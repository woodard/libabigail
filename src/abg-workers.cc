// -*- Mode: C++ -*-
//
// Copyright (C) 2013-2016 Red Hat, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this program; see the file COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.

// Author: Dodji Seketeli

/// @file
///
/// This file implements the worker threads (or thread pool) design
/// pattern.  It aims at performing a set of tasks in parallel, using
/// the multi-threading capabilities of the underlying processor(s).

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <iostream>

#include "abg-internal.h"
// <headers defining libabigail's API go under here>
ABG_BEGIN_EXPORT_DECLARATIONS

#include "abg-workers.h"

ABG_END_EXPORT_DECLARATIONS
// </headers defining libabigail's API>

namespace abigail
{

namespace workers
{

/// @defgroup thread_pool Worker Threads
/// @{
///
/// \brief Libabigail's implementation of Thread Pools.
///
/// The main interface of this pattern is a @ref queue of @ref tasks
/// to be performed.  Associated to that queue are a set of worker
/// threads (these are native posix threads) that sits there, idle,
/// until at least one @ref task is added to the queue.
///
/// When a @ref task is added to the @ref queue, one thread is woken
/// up, picks the @ref task, removes it from the @ref queue, and
/// executes the instructions it carries.  We say the worker thread
/// performs the @ref task.
///
/// When the worker thread is done performing the @ref task, the
/// performed @ref task is added to another queue, named as the "done
/// queue".  Then the thread looks at the @ref queue of tasks to be
/// performed again, and if there is at least one task in that queue,
/// the same process as above is done.  Otherwise, the thread blocks,
/// waiting for a new task to be added to the queue.
///
/// By default, the number of worker threads is equal to the number of
/// execution threads advertised by the underlying processor.
///
/// Note that the user of the queue can either wait for all the tasks
/// to be performed by the pool of threads,and them stop them, get the
/// vector of done tasks and proceed to whatever computation she may
/// need next.
///
/// Or she can choose to be asynchronously notified whenever a task is
/// performed and added to the "done queue".
///
///@}

/// @return The number of hardware threads of executions advertised by
/// the underlying processor.
size_t
get_number_of_threads()
{return sysconf(_SC_NPROCESSORS_ONLN);}

// <task stuff>

/// The default constructor of the @ref task type
task::task()
{}

/// Destructor of the @ref task type.
task::~task()
{}
// </task stuff>

// <worker declarations>
struct worker;

///  A convenience typedef for a shared_ptr to the @ref worker type.
typedef shared_ptr<worker> worker_sptr;

/// The abstraction of a worker thread.
///
/// This is an implementation detail of the @ref queue public
/// interface type of this worker thread design pattern.
struct worker
{
  pthread_t tid;

  worker()
    : tid()
  {}

  static queue::priv*
  wait_to_execute_a_task(queue::priv*);
}; // end struct worker

// </worker declarations>

// <queue stuff>

/// The private data structure of the task queue.
struct queue::priv
{
  // A boolean to say if the user wants to shutdown the worker
  // threads.
  bool				bring_workers_down;
  // The number of worker threads.
  size_t			num_workers;
  // The mutex associated to the queue condition variable below.
  pthread_mutex_t		queue_cond_mutex;
  // The queue condition variable.  This condition is used to make the
  // worker threads sleep until a new task is added to the queue of
  // todo tasks.  Whenever a new task is added to that queue, a signal
  // is sent to all the threads sleeping on this condition variable,
  // and only one of them wakes up and takes the mutex
  // queue_cond_mutex above.
  pthread_cond_t		queue_cond;
  // A mutex that protects the todo tasks queue from being accessed in
  // read/write by two threads at the same time.
  pthread_mutex_t		tasks_todo_mutex;
  // A mutex that protects the done tasks queue from being accessed in
  // read/write by two threads at the same time.
  pthread_mutex_t		tasks_done_mutex;
  // The todo task queue itself.
  std::queue<task_sptr>	tasks_todo;
  // The done task queue itself.
  std::vector<task_sptr>	tasks_done;
  // This functor is invoked to notify the user of this queue that a
  // task has been completed and has been added to the done tasks
  // vector.  We call it a notifier.  This notifier is the default
  // notifier of the work queue; the one that is used when the user
  // has specified no notifier.  It basically does nothing.
  task_done_notify	default_notify;
  // This is a reference to the the notifier that is actually used in
  // the queue.  It's either the one specified by the user or the
  // default one.
  task_done_notify&	notify;
  // A vector of the worker threads.
  std::vector<worker>		workers;

  /// The default constructor of @ref queue::priv.
  priv()
    : bring_workers_down(),
      num_workers(get_number_of_threads()),
      queue_cond_mutex(),
      queue_cond(),
      tasks_todo_mutex(),
      tasks_done_mutex(),
      notify(default_notify)
  {create_workers();}

  /// A constructor of @ref queue::priv.
  ///
  /// @param nb_workers the number of worker threads to have in the
  /// thread pool.
  priv(size_t nb_workers)
    : bring_workers_down(),
      num_workers(nb_workers),
      queue_cond_mutex(),
      queue_cond(),
      tasks_todo_mutex(),
      tasks_done_mutex(),
      notify(default_notify)
  {create_workers();}

  /// A constructor of @ref queue::priv.
  ///
  /// @param nb_workers the number of worker threads to have in the
  /// thread pool.
  ///
  /// @param task_done_notify a functor object that is invoked by the
  /// worker thread which has performed the task, right after it's
  /// added that task to the vector of the done tasks.
  priv(size_t nb_workers, task_done_notify& n)
    : bring_workers_down(),
      num_workers(nb_workers),
      queue_cond_mutex(),
      queue_cond(),
      tasks_todo_mutex(),
      tasks_done_mutex(),
      notify(n)
  {create_workers();}

  /// Create the worker threads pool and have all threads sit idle,
  /// waiting for a task to be added to the todo queue.
  void
  create_workers()
  {
    for (unsigned i = 0; i < num_workers; ++i)
      {
	worker w;
	assert(pthread_create(&w.tid,
			      /*attr=*/0,
			      (void*(*)(void*))&worker::wait_to_execute_a_task,
			      this) == 0);
	workers.push_back(w);
      }
  }

  /// Submit a task to the queue of tasks to be performed.
  ///
  /// This wakes up one thread from the pool which immediatly starts
  /// performing the task.  When it's done with the task, it goes back
  /// to be suspended, waiting for a new task to be scheduled.
  ///
  /// @param t the task to schedule.  Note that a nil task won't be
  /// scheduled.  If the queue is empty, the task @p t won't be
  /// scheduled either.
  ///
  /// @return true iff the task @p t was successfully scheduled.
  bool
  schedule_task(const task_sptr& t)
  {
    if (workers.empty() || !t)
      return false;

    pthread_mutex_lock(&tasks_todo_mutex);
    tasks_todo.push(t);
    pthread_mutex_unlock(&tasks_todo_mutex);

    pthread_mutex_lock(&queue_cond_mutex);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_cond_mutex);
    return true;
  }

  /// Submit a vector of task to the queue of tasks to be performed.
  ///
  /// This wakes up threads of the pool which immediatly start
  /// performing the tasks.  When they are done with the task, they go
  /// back to be suspended, waiting for new tasks to be scheduled.
  ///
  /// @param tasks the tasks to schedule.
  bool
  schedule_tasks(const tasks_type& tasks)
  {
    bool is_ok= true;
    for (tasks_type::const_iterator t = tasks.begin(); t != tasks.end(); ++t)
      is_ok &= schedule_task(*t);
    return is_ok;
  }

  /// Signal all the threads (of the pool) which are suspended and
  /// waiting to perform a task, so that they wake up and end up their
  /// execution.  If there is no task to perform, they just end their
  /// execution.  If there are tasks to perform, they finish them and
  /// then end their execution.
  ///
  /// This function then joins all the tasks of the pool, waiting for
  /// them to finish, and then it returns.  In other words, this
  /// function suspends the thread of the caller, waiting for the
  /// worker threads to finish their tasks, and end their execution.
  ///
  /// If the user code wants to work with the thread pool again,
  /// she'll need to create them again, using the member function
  /// create_workers().
  void
  do_bring_workers_down()
  {
    if (workers.empty())
      return;

    pthread_mutex_lock(&tasks_todo_mutex);
    bring_workers_down = true;
    pthread_mutex_unlock(&tasks_todo_mutex);

    // Acquire the mutex that protects the queue condition variable
    // (queue_cond) and wake up all the workers that are sleeping on
    // the condition.
    pthread_mutex_lock(&queue_cond_mutex);
    assert(pthread_cond_broadcast(&queue_cond) == 0);
    pthread_mutex_unlock(&queue_cond_mutex);

    for (std::vector<worker>::const_iterator i = workers.begin();
	 i != workers.end();
	 ++i)
      assert(pthread_join(i->tid, /*thread_return=*/0) == 0);
    workers.clear();
  }

  /// Destructors of @ref queue::priv type.
  ~priv()
  {do_bring_workers_down();}

}; //end struct queue::priv

/// Default constructor of the @ref queue type.
///
/// By default the queue is created with a number of worker threaders
/// which is equals to the number of simultaneous execution threads
/// supported by the underlying processor.
queue::queue()
  : p_(new priv())
{}

/// Constructor of the @ref queue type.
///
/// @param number_of_workers the number of worker threads to have in
/// the pool.
queue::queue(unsigned number_of_workers)
  : p_(new priv(number_of_workers))
{}

/// Constructor of the @ref queue type.
///
/// @param number_of_workers the number of worker threads to have in
/// the pool.
///
/// @param the notifier to invoke when a task is done doing its job.
/// Users should create a type that inherit this @ref task_done_notify
/// class and overload its virtual task_done_notify::operator()
/// operator function.  Note that the code of that
/// task_done_notify::operator() is assured to run in *sequence*, with
/// respect to the code of other task_done_notify::operator() from
/// other tasks.
queue::queue(unsigned number_of_workers,
	     task_done_notify& notifier)
  : p_(new priv(number_of_workers, notifier))
{}

/// Getter of the size of the queue.  This gives the number of task
/// still present in the queue.
///
/// @return the number of task still present in the queue.
size_t
queue::get_size() const
{return p_->tasks_todo.size();}

/// Submit a task to the queue of tasks to be performed.
///
/// This wakes up one thread from the pool which immediatly starts
/// performing the task.  When it's done with the task, it goes back
/// to be suspended, waiting for a new task to be scheduled.
///
/// @param t the task to schedule.  Note that if the queue is empty or
/// if the task is nil, the task is not scheduled.
///
/// @return true iff the task was successfully scheduled.
bool
queue::schedule_task(const task_sptr& t)
{return p_->schedule_task(t);}

/// Submit a vector of tasks to the queue of tasks to be performed.
///
/// This wakes up one or more threads from the pool which immediatly
/// start performing the tasks.  When the threads are done with the
/// tasks, they goes back to be suspended, waiting for a new task to
/// be scheduled.
///
/// @param tasks the tasks to schedule.
bool
queue::schedule_tasks(const tasks_type& tasks)
{return p_->schedule_tasks(tasks);}

/// Suspends the current thread until all worker threads finish
/// performing the tasks they are executing.
///
/// If the worker threads were suspended waiting for a new task to
/// perform, they are woken up and their execution ends.
///
/// The execution of the current thread is resumed when all the
/// threads of the pool have finished their execution and are
/// terminated.
void
queue::wait_for_workers_to_complete()
{p_->do_bring_workers_down();}

/// Getter of the vector of tasks that got performed.
///
/// @return the vector of tasks that got performed.
std::vector<task_sptr>&
queue::get_completed_tasks() const
{return p_->tasks_done;}

/// Destructor for the @ref queue type.
queue::~queue()
{}

/// The default function invocation operator of the @ref queue type.
///
/// This does nothing.
void
queue::task_done_notify::operator()(const task_sptr&/*task_done*/)
{
}

// </queue stuff>

// <worker definitions>

/// Wait to be woken up by a thread condition signal, then look if
/// there is a task to be executed.  If there is, then pick one (in a
/// FIFO manner), execute it, and put the executed task into the set
/// of done tasks.
///
/// @param t the private data of the "task queue" type to consider.
///
/// @param return the same private data of the task queue type we got
/// in argument.
queue::priv*
worker::wait_to_execute_a_task(queue::priv* p)
{
  pthread_mutex_lock(&p->tasks_todo_mutex);
  bool more_tasks = !p->tasks_todo.empty();
  bool bring_workers_down = p->bring_workers_down;
  pthread_mutex_unlock(&p->tasks_todo_mutex);

  do
    {
      // If there is no more tasks to perform and the queue is not to
      // be brought down then wait (sleep) for new tasks to come up.
      while (!more_tasks && !bring_workers_down)
	{
	  pthread_mutex_lock(&p->queue_cond_mutex);
	  pthread_cond_wait(&p->queue_cond, &p->queue_cond_mutex);
	  pthread_mutex_unlock(&p->queue_cond_mutex);

	  pthread_mutex_lock(&p->tasks_todo_mutex);
	  more_tasks = !p->tasks_todo.empty();
	  bring_workers_down = p->bring_workers_down;
	  pthread_mutex_unlock(&p->tasks_todo_mutex);
	}


      // We were woken up.  So maybe there are tasks to perform?  If
      // so, get a task from the queue ...
      task_sptr t;
      pthread_mutex_lock(&p->tasks_todo_mutex);
      if (!p->tasks_todo.empty())
	{
	  t = p->tasks_todo.front();
	  p->tasks_todo.pop();
	}
      pthread_mutex_unlock(&p->tasks_todo_mutex);

      // If we've got a task to perform then perform it and when it's
      // done then add to the set of tasks that are done.
      if (t)
	{
	  t->perform();

	  // Add the task to the vector of tasks that are done and
	  // notify listeners about the fact that the task is done.
	  //
	  // Note that this (including the notification) is not
	  // happening in parallel.  So the code performed by the
	  // notifier during the notification is running sequentially,
	  // not in parallel with any other task that was just done
	  // and that is notifying its listeners.
	  pthread_mutex_lock(&p->tasks_done_mutex);
	  p->tasks_done.push_back(t);
	  p->notify(t);
	  pthread_mutex_unlock(&p->tasks_done_mutex);
	}

      pthread_mutex_lock(&p->tasks_todo_mutex);
      more_tasks = !p->tasks_todo.empty();
      bring_workers_down = p->bring_workers_down;
      pthread_mutex_unlock(&p->tasks_todo_mutex);
    }
    while (!p->bring_workers_down || more_tasks);

  return p;
}
// </worker definitions>
} //end namespace workers
} //end namespace abigail
