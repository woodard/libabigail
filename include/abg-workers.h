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
/// This file declares an interface for the worker threads (or thread
/// pool) design pattern.  It aims at performing a set of tasks in
/// parallel, using the multi-threading capabilities of the underlying
/// processor(s).
///

#ifndef __ABG_WORKERS_H__
#define __ABG_WORKERS_H__

#include <tr1/memory>
#include <vector>

using std::tr1::shared_ptr;

namespace abigail
{
namespace workers
{

class task;
typedef shared_ptr<task> task_sptr;

size_t get_number_of_threads();

class task
{
public:
  task();
  virtual void
  perform() = 0;

  virtual ~task();
}; // end class task.

class queue
{
public:
  struct priv;
  typedef shared_ptr<priv> priv_sptr;

private:
  priv_sptr p_;

public:
  struct task_done_notify;
  queue();
  queue(unsigned number_of_workers);
  queue(unsigned number_of_workers,
	const task_done_notify& notifier);
  size_t get_size() const;
  bool schedule_task(const task_sptr&);
  void wait_for_workers_to_complete();
  const std::vector<task_sptr>& get_completed_tasks() const;
  ~queue();
}; // end class queue

struct queue::task_done_notify
{
  virtual void
  operator()(const task_sptr& task_done);
};
} // end namespace workers
} // end namespace abigail
#endif // __ABG_WORKERS_H__
