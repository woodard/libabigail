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

/// @defgroup Worker Threads
/// @{
///
/// The main interface of this pattern is a queue of tasks to be
/// performed.  Associated to that queue are a set of worker threads,
/// that sit there, idle, until at least one task is added to the
/// queue.
///
/// When a task is added to the queue, one thread is woken up, picks
/// the task, removes it from the queue, and executes the instructions
/// carried by the task.  We say the worker thread performs the task.
///
/// When the worker thread is done performing the task, the performed
/// task is added to another queue, named as the "done queue".  Then
/// the thread looks at the queue of tasks to be performed again, and
/// if there is at least one task in that queue, the same process as
/// above is done.  Otherwise, the thread blocks, waiting for a new
/// task to be added to the queue.
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

#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <vector>
#include <iostream>
#include "abg-workers.h"
namespace abigail
{
namespace workers
{

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
  // vector.
  task_done_notify		notify;
  // A vector of the worker threads.
  std::vector<worker>		workers;

  /// The default constructor of @ref queue::priv.
  priv()
    : bring_workers_down(),
      num_workers(get_number_of_threads()),
      queue_cond_mutex(),
      queue_cond(),
      tasks_todo_mutex(),
      tasks_done_mutex()
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
      tasks_done_mutex()
  {create_workers();}

  /// A constructor of @ref queue::priv.
  ///
  /// @param nb_workers the number of worker threads to have in the
  /// thread pool.
  ///
  /// @param task_done_notify a functor object that is invoked by the
  /// worker thread which has performed the task, right after it's
  /// added that task to the vector of the done tasks.
  priv(size_t nb_workers, const task_done_notify& n)
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
  /// @param t the task to schedule.
  bool
  schedule_task(const task_sptr& t)
  {
    if (workers.empty())
      return false;

    pthread_mutex_lock(&tasks_todo_mutex);
    tasks_todo.push(t);
    pthread_mutex_unlock(&tasks_todo_mutex);

    pthread_mutex_lock(&queue_cond_mutex);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_cond_mutex);
    return true;
  }

  /// Signal all the threads (of the pool) which are suspended, so
  /// that they wakes up.  If there is no task to perform, they just
  /// end their execution.  If there are tasks to perform, they finish
  /// them and then end their execution.
  ///
  /// This function then joins all the tasks of the pool, waiting for
  /// them to finish, and then it returns.  In other words, this
  /// function suspends the thread of the caller, waiting for the
  /// worker threads to finish their tasks, and end their execution.
  ///
  /// If the user wants to work with the thread pool again, she'll
  /// need to create them again, using the member function
  /// create_workers().
  void
  do_bring_workers_down()
  {
    if (workers.empty())
      return;

    bring_workers_down = true;

    pthread_mutex_lock(&tasks_todo_mutex);
    if (tasks_todo.empty())
      assert(pthread_cond_broadcast(&queue_cond) == 0);
    pthread_mutex_unlock(&tasks_todo_mutex);

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
/// @param the notifier to invoker when a task is performed.  Users
/// should create a type that inherit this @ref task_done_notify class
/// and overload its virtual task_done_notify::operator() operator
/// function.
queue::queue(unsigned number_of_workers,
	     const task_done_notify& notifier)
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
/// @param t the task to schedule.
bool
queue::schedule_task(const task_sptr& t)
{return p_->schedule_task(t);}

/// Suspends the current thread until all worker threads finish
/// performing the tasks they are executing.
///
/// If the worker threads were suspended waiting for a new task to
/// perform, they are woken up and their execution ends.
///
/// The execution of the current thread is resume when all the threads
/// of the pool have finished their execution and are terminated.
void
queue::wait_for_workers_to_complete()
{p_->do_bring_workers_down();}

/// Getter of the vector of tasks that got performed.
///
/// @retun the vector of tasks that got performed.
const std::vector<task_sptr>&
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
  pthread_mutex_unlock(&p->tasks_todo_mutex);

  do
    {
      pthread_mutex_lock(&p->queue_cond_mutex);
      while (!more_tasks && !p->bring_workers_down)
	{
	  pthread_cond_wait(&p->queue_cond, &p->queue_cond_mutex);

	  pthread_mutex_lock(&p->tasks_todo_mutex);
	  more_tasks = !p->tasks_todo.empty();
	  pthread_mutex_unlock(&p->tasks_todo_mutex);
	}
      pthread_mutex_unlock(&p->queue_cond_mutex);

      task_sptr t;
      pthread_mutex_lock(&p->tasks_todo_mutex);
      if (!p->tasks_todo.empty())
	{
	  t = p->tasks_todo.front();
	  p->tasks_todo.pop();
	}
      pthread_mutex_unlock(&p->tasks_todo_mutex);

      if (t)
	{
	  t->perform();
	  pthread_mutex_lock(&p->tasks_done_mutex);
	  p->tasks_done.push_back(t);
	  pthread_mutex_unlock(&p->tasks_done_mutex);
	  p->notify(t);
	}

      pthread_mutex_lock(&p->tasks_todo_mutex);
      more_tasks = !p->tasks_todo.empty();
      pthread_mutex_unlock(&p->tasks_todo_mutex);
    }
    while (!p->bring_workers_down || more_tasks);

  return p;
}
// </worker definitions>
} //end namespace workers
} //end namespace abigail
