// -*- Mode: C++ -*-
//
// Copyright (C) 2013 Red Hat, Inc.
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

/// @file
///
/// This file declares types and operations implementing the "O(ND)
/// Difference Algorithm" (aka diff2) from Eugene W. Myers, to compute
/// the difference between two sequences.
///
/// To understand what is going on here, one must read the paper at
/// http://www.xmailserver.org/diff2.pdf.  Throughout this file, that
/// paper is referred to as "the paper".
///
/// The implementations goes as far as calculating the shortest edit
/// script (the set of insertions and deletions) for transforming a
/// sequence into another.  The main entry point for that is the
/// compute_diff() function.

#include <stdexcept>
#include <cassert>
#include <cstdlib>
#include <ostream>
#include <string>
#include <vector>
#include <sstream>

namespace abigail
{

namespace diff_utils
{

// Inject the names from std:: below into this namespace
using std::string;
using std::ostream;
using std::vector;
using std::abs;
using std::ostringstream;

/// A class representing a vertex in an edit graph, as explained in
/// the paper.  A vertex is a basically a pair of coordinates
/// (abscissa and ordinate).
class point
{
  int x_;
  int y_;
  bool empty_;

public:

  point()
    : x_(-1), y_(-1),empty_(true)
  {}

  point(int x, int y)
    : x_(x), y_(y), empty_(false)
  {}

  point(const point& p)
    : x_(p.x()), y_(p.y()), empty_(p.is_empty())
  {}

  int
  x() const
  {return x_;}

  void
  x(int x)
  {
    x_ = x;
    empty_ = false;
  }

  int
  y() const
  {return y_;}

  void
  y(int y)
  {
    y_ = y;
    empty_ = false;
  }

  void
  set(int x, int y)
  {
    x_ = x;
    y_ = y;
    empty_ = false;
  }

  bool
  operator!=(const point& o)
  {return (x() != o.x() || y() != o.y() || is_empty() != o.is_empty());}

  bool
  operator==(const point& o)
  {return !(*this != o);}

  point
  operator+(int val) const
  {return point(x() + val, y() + val);}

  point
  operator-(int val) const
  {return point(x() - val, y() - val);}

  point&
  operator+= (int val)
  {
    set(x_ + val, y_ + val);
    return *this;
  }

  point&
  operator-= (int val)
  {return (*this) += (-val);}

  point&
  operator--()
  {return (*this) -= 1;}

  point&
  operator++()
  {return (*this) += 1;}

  point
  operator--(int)
  {
    point tmp(*this);
    (*this)--;
    return tmp;
  }

  point
  operator++(int)
  {
    point tmp(*this);
    (*this)++;
    return tmp;
  }

  point&
  operator=(int val)
  {
    set(val, val);
    return *this;
  }

  point&
  operator=(const point& p)
  {
    set(p.x(), p.y());
    return *this;
  }

  bool
  is_empty() const
  {return empty_;}

  operator bool () const
  {return !is_empty();}

  bool
  operator!() const
  {return is_empty();}

  void
  clear()
  {
    x_ = -1;
    y_ = -1;
    empty_ = true;
  }
};// end point

/// The array containing the furthest D-path end-points, for each value
/// of K.  MAX_D is the maximum value of the D-Path.  That is, M+N if
/// M is the size of the first input string, and N is the size of the
/// second.
class d_path_vec : public std::vector<int>
{
private:

  unsigned a_size_;
  unsigned b_size_;

  /// Forbid vector size modifications
  void
  push_back(const typename vector<int>::value_type&);

  /// Forbid default constructor.
  d_path_vec();

  void
  check_index_against_bound(int index, int bound) const
  {
    if (std::abs(index) > bound)
      {
	ostringstream o;
	o << "index '" << index
	  << "' out of range [-" << bound << ", " << bound << "]";
	throw std::out_of_range(o.str());
      }
  }

public:

  /// Constructor of the d_path_vec.
  ///
  /// The underlying vector allocates 2 * MAX_D - 1 space, so that one
  /// can address elements in the index range [-MAX_D, MAX_D].
  /// And MAX_D is the sum of the (1 + size_of_the_sequence).
  ///
  /// @params size1 the size of the first sequence we are interested
  /// in.
  ///
  /// @param size2 the size of the second sequence we are interested
  /// in.
  d_path_vec(unsigned size1, unsigned size2)
    : vector<int>(2 * (size1 + 1 + size2 + 1) - 1, 0),
      a_size_(size1), b_size_(size2)
  {
  }

  typename std::vector<int>::const_reference
  operator[](int index) const
  {
    int i = max_d() + index;
    return (*static_cast<const vector<int>* >(this))[i];
  }

  typename std::vector<int>::reference
  operator[](int index)
  {
    int i = max_d() + index;
    return (*static_cast<vector<int>* >(this))[i];
  }

  typename std::vector<int>::reference
  at(int index)
  {
    check_index_against_bound(index, max_d());
    int i = max_d() + index;
    return static_cast<vector<int>* >(this)->at(i);
  }

  typename std::vector<int>::const_reference
  at(int index) const
  {
    check_index_against_bound(index, max_d());
    int i = max_d() + index;
    return static_cast<const vector<int>* >(this)->at(i);
  }

  unsigned
  a_size() const
  {return a_size_;}

  unsigned
  b_size() const
  {return b_size_;}

  unsigned
  max_d() const
  {return a_size() + b_size();}
}; // end class d_path_vec

/// The abstration of an insertion of elements of a sequence B into a
/// sequence A.  This is used to represent the edit script for
/// transforming a sequence A into a sequence B.
///
/// And insertion mainly encapsulates two components:
///
///   - An insertion point: this is the index (starting at 0) of the
///     element of the sequence A after which the insertion occurs.
///
///   - Inserted elements: this is a vector of indexes of elements of
///     sequence B (starting at 0) that got inserted into sequence A,
///     after the insertion point.
class insertion
{
  int		insertion_point_;
  vector<int>	inserted_;

public:

  insertion(int insertion_point,
	    const vector<int>& inserted_indexes)
    : insertion_point_(insertion_point),
      inserted_(inserted_indexes)
  {}

    insertion(int insertion_point = 0)
      : insertion_point_(insertion_point)
  {}

  int
  insertion_point_index() const
  {return insertion_point_;}

  void
  insertion_point_index(int i)
  {insertion_point_ = i;}

  const vector<int>&
  inserted_indexes() const
  {return inserted_;}

  vector<int>&
  inserted_indexes()
  {return inserted_;}
};// end class insertion

/// The abstraction of the deletion of one element of a sequence A.
///
/// This encapsulates the index of the element A that got deleted.
class deletion
{
  int index_;

public:

  deletion(int i)
    : index_(i)
  {}

  int
  index() const
  {return index_;}

  void
  index(int i)
  {index_ = i;}
};// end class deletion

/// The abstraction of an edit script for transforming a sequence A
/// into a sequence B.
///
/// It encapsulates the insertions and deletions for transforming A
/// into B.
class edit_script
{
  vector<insertion>	insertions_;
  vector<deletion>	deletions_;

public:

  edit_script()
  {}

  const vector<insertion>&
  insertions() const
  {return insertions_;}

  vector<insertion>&
  insertions()
  {return insertions_;}

  const vector<deletion>&
  deletions() const
  {return deletions_;}

  vector<deletion>&
  deletions()
  {return deletions_;}

  void
  append(const edit_script& es)
  {
    insertions().insert(insertions().end(),
			es.insertions().begin(),
			es.insertions().end());
    deletions().insert(deletions().end(),
		       es.deletions().begin(),
		       es.deletions().end());
  }

  void
  prepend(const edit_script& es)
  {
    insertions().insert(insertions().begin(),
			es.insertions().begin(),
			es.insertions().end());
    deletions().insert(deletions().begin(),
		       es.deletions().begin(),
		       es.deletions().end());
  }

  void
  clear()
  {
    insertions().clear();
    deletions().clear();
  }

  bool
  is_empty() const
  {return insertions().empty() && deletions().empty();}

  operator bool() const
  {return !is_empty();}

  int
  num_insertions() const
  {
    int l = 0;
    for (vector<insertion>::const_iterator i = insertions().begin();
	 i != insertions().end();
	 ++i)
      l += i->inserted_indexes().size();
    return l;
  }

  int
  num_deletions() const
  {return deletions().size();}

  int
  length() const
  {return num_insertions() + num_deletions();}
};//end class edit_script

bool
point_is_valid_in_graph(point& p,
			unsigned a_size,
			unsigned b_size);

bool
ends_of_furthest_d_paths_overlap(point& forward_d_path_end,
				 point& reverse_d_path_end);

/// Find the end of the furthest reaching d-path on diagonal k, for
/// two sequences.  In the paper This is referred to as "the basic
/// algorithm".
///
/// Unlike in the paper, the coordinates of the edit graph start at
/// (-1,-1), rather than (0,0), and they end at (M-1, N-1), rather
/// than (M,N).
///
/// @param k the number of the diagonal on which we want to find the
/// end of the furthest reaching D-path.
///
/// @param d the D in D-Path.  That's the number of insertions/deletions
/// (the number of changes, in other words) in the changeset.  That is
/// also the number of non-diagonals in the D-Path.
///
/// @param a_begin an iterator to the beginning of the first sequence
///
/// @param a_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param b_begin an iterator to the beginning of the second sequence.
///
/// @param b_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param v the vector of furthest end points of d_paths, at (d-1).
/// It contains the abscissas of the furthest end points for different
/// values of k, at (d-1).  That is, for k in [-D + 1, -D + 3, -D + 5,
/// ..., D - 1], v[k] is the abscissa of the end of the furthest
/// reaching (D-1)-path on diagonal k.
///
/// @param end abscissa and ordinate of the computed abscissa of the
/// end of the furthest reaching (d-1) paths.
///
/// @return true if the end of the furthest reaching path that was
/// found was inside the boundaries of the edit graph, false
/// otherwise.
template<typename RandomAccessOutputIterator>
bool
end_of_fr_d_path_in_k(int k, int d,
		      RandomAccessOutputIterator a_begin,
		      RandomAccessOutputIterator a_end,
		      RandomAccessOutputIterator b_start,
		      RandomAccessOutputIterator b_end,
		      d_path_vec& v, point& end)
{
  int x = -1, y = -1;

  // Let's pick the end point of the furthest reaching
  // (D-1)-path.  It's either v[k-1] or v[k+1]; the word
  // "furthest" means we choose the one which abscissa is the
  // greatest (that is, furthest from abscissa zero).
  if (k == -d || ((k != d) && (v[k-1] < v[k + 1])))
    // So, the abscissa of the end point of the furthest
    // reaching (D-1)-path is v[k+1].  That is a diagonal that
    // is above the current (k) diagonal, and on the right.
    // To move to the current k diagonal, one has to move
    // "down" from the diagonal k+1.  So the abscissa won't
    // change.  Only the ordinate will.  It will be given by y
    // = x - k (a bit below); as k has changed from k - 1 (it
    // has increased), y is going to be the new y that is
    // 'down' from the previous y in k - 1.
    x = v[k+1];
  else
    // So the abscissa of the end point of the furthest
    // (D-1)-path is v[k-1].  That is on the left of the
    // current k diagonal.  To move to the current k diagonal,
    // one has to move "right" from diagonal k - 1.  That is,
    // the y stays constant and x is incremented.
    x = v[k-1] + 1;

  // Now get the value of y from the equation k = x -y.
  // This is the point where we first touch K, when we move
  // from the end of the furthest reaching (D-1)-path.
  y = x - k;

  int last_x_index = a_end - a_begin - 1;
  int last_y_index = b_end - b_start - 1;
  // Now, follow the snake (aka, zero or more consecutive
  // diagonals).  Note that we stay on the k diagonal when we
  // do this.
  while ((x < last_x_index) && (y < last_y_index))
    if (a_begin[x + 1] == b_start[y + 1])
      {
	x = x + 1;
	y = y + 1;
      }
    else
      break;

  // Note the point that we store in v here might be outside the
  // bounds of the edit graph.  But we store it at this step (for a
  // given D) anyway, because out of bound or not, we need this value
  // at this step to be able to compute the value of the point on the
  // "next" diagonal for the next D.
  v[k] = x;

  if (x >= (int) v.a_size()
      || y >= (int) v.b_size()
      || x < -1 || y < -1)
    return false;

  end.x(x);
  end.y(y);

  return true;
}

/// Find the end of the furthest reaching reverse d-path on diagonal k
/// + delta.  Delta is abs(M - N), with M being the size of a and N
/// being the size of b.  This is the "basic algorithm", run backward.
/// That is, starting from the point (M,N) of the edit graph.
///
/// Unlike in the paper, the coordinates of the edit graph start at
/// (-1,-1), rather than (0,0), and they end at (M-1, N-1), rather
/// than (M,N).
///
/// @param k the number of the diagonal on which we want to find the
/// end of the furthest reaching reverse D-path.  Actually, we want to
/// find the end of the furthest reaching reverse D-path on diagonal (k
/// - delta).
///
/// @param d the D in D-path.  That's the number of insertions/deletions
/// (the number of changes, in other words) in the changeset.  That is
/// also the number of non-diagonals in the D-Path.
///
/// @param a_begin an iterator to the beginning of the first sequence
///
/// @param a_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param b_begin an iterator to the beginning of the second sequence.
///
/// @param b_end an iterator that points right after the last element
/// of the second sequence to consider.
///
/// @param v the vector of furthest end points of d_paths, at (d-1).
/// It contains the abscissae of the furthest end points for different
/// values of k - delta, at (d-1).  That is, for k in [-D + 1, -D + 3,
/// -D + 5, ..., D - 1], v[k - delta] is the abscissa of the end of the
/// furthest reaching (D-1)-path on diagonal k - delta.
///
/// @param point the computed abscissa and ordinate of the end point
/// of the furthest reaching d-path on line k - delta.
///
/// @return true iff the end of the furthest reaching path that was
/// found was inside the boundaries of the edit graph, false
/// otherwise.
template<typename RandomAccessOutputIterator>
bool
end_of_frr_d_path_in_k_plus_delta (int k, int d,
				   RandomAccessOutputIterator a_begin,
				   RandomAccessOutputIterator a_end,
				   RandomAccessOutputIterator b_begin,
				   RandomAccessOutputIterator b_end,
				   d_path_vec& v, point& end)
{
  int a_size = a_end - a_begin;
  int b_size = b_end - b_begin;
  int delta = a_size - b_size;
  int k_plus_delta = k + delta;
  int x = -1, y = -1;

  // Let's pick the end point of the furthest reaching (D-1)-path and
  // move from there to reach the current k_plus_delta-line.  That end
  // point of the furthest reaching (D-1)-path is either on
  // v[k_plus_delta-1] or on v[k_plus_delta+1]; the word "furthest"
  // means we choose the one which abscissa is the lowest (that is,
  // furthest from abscissa M).
  if (k_plus_delta == -d + delta
      || ((k_plus_delta != d + delta)
	  && (v[k_plus_delta + 1] < v[k_plus_delta - 1])))
    {
      // We move left, that means ordinate won't change ...
      x = v[k_plus_delta + 1];
      y = x - (k_plus_delta + 1);
      // ... and abscissa decreases.
      x = x - 1;
    }
  else
    {
      // So the furthest end point is on the k_plus_delta - 1
      // diagonal.  That is a diagonal that is 'below' the
      // k_plus_delta current diagonal.  So to join the current
      // diagonal from the k_plus_delta - 1 one, we need to move up.

      // So moving up means abscissa won't change ...
      x = v[k_plus_delta - 1];
      // ... and that ordinate decreases.
      y = x - (k_plus_delta - 1) - 1;
    }

  // Now, follow the snake.  Note that we stay on the k_plus_delta
  // diagonal when we do this.
  while (x >= 0 && y >= 0)
    if (a_begin[x] == b_begin[y])
      {
	x = x - 1;
	y = y - 1;
      }
    else
      break;

  // Note the point that we store in v here might be outside the
  // bounds of the edit graph.  But we store it at this step (for a
  // given D) anyway, because out of bound or not, we need this value
  // at this step to be able to compute the value of the point on the
  // "next" diagonal for the next D.
  v[k_plus_delta] = x;

  if (x == -1 && y == -1)
    ;
  else if (x < -1 || y < -1)
    return false;

  end.x(x);
  end.y(y);

  return true;
}

/// Tests if a given point is a match point in an edit graph.
///
/// @param a_begin the begin iterator of the first input sequence of
/// the edit graph.
///
/// @param a_end the end iterator of the first input sequence of the
/// edit graph.  This points to one element passed the end of the
/// sequence.
///
/// @param b_begin the begin iterator of the second input sequence of
/// the edit graph.
///
/// @param b_end the end iterator of the second input sequence of the
/// edit graph.  This points the one element passed the end of the
/// sequence.
///
/// @param point the point to test for being a match point.
///
/// @return true iff \a point is a match point.
template<typename RandomAccessOutputIterator>
bool
is_match_point(RandomAccessOutputIterator a_begin,
	       RandomAccessOutputIterator a_end,
	       RandomAccessOutputIterator b_begin,
	       RandomAccessOutputIterator b_end,
	       const point& point)
{
  int a_size = a_end - a_begin, b_size = b_end - b_begin;

  if (point.x() < 0
      || point.x () >= a_size
      || point.y() < 0
      || point.y() >= b_size)
    return false;

  return (a_begin[point.x()] == b_begin[point.y()]);
}

/// Given two points A and B, if A is a match point, assign A to B (B
/// = A).
///
/// @param a_begin the begin iterator of the first input sequence of
/// the edit graph.
///
/// @param a_end the end iterator of the first input sequence of the
/// edit graph.  This points to one element after the last element of
/// the sequence.
///
/// @param b_begin the begin iterator of the second input sequence of
/// the edit graph.
///
/// @param b_end the end iterator of the second input sequence of the
/// edit graph.  This points the one element after the last element of
/// the sequence.
///
/// @param p the first point to consider.  This is the one to test for
/// being a match point.
///
/// @param snake_end_point the second point to consider.  This the one
/// to be assigned to, if \a p is a match point.
template<typename RandomAccessOutputIterator>
void
maybe_record_match_point(const RandomAccessOutputIterator a_begin,
			 const RandomAccessOutputIterator a_end,
			 const RandomAccessOutputIterator b_begin,
			 const RandomAccessOutputIterator b_end,
			 const point& p,
			 point& snake_end_point)
{
  if (is_match_point(a_begin, a_end,
		     b_begin, b_end,
		     p))
    snake_end_point = p;
}

/// Given the end point of a snake, find its starting point.
///
/// @param a_begin the begin iterator of the first input sequence of
/// the edit graph.
///
/// @param a_end the end iterator of the first input sequence of the
/// edit graph.  This points to one element after the last element of
/// the sequence.
///
/// @param b_begin the begin iterator of the second input sequence of
/// the edit graph.
///
/// @param b_end the end iterator of the second input sequence of the
/// edit graph.  This points to one element after the last element of
/// the sequence.
///
/// @param snake_end_point the end point of the snake.
///
/// @param snake_start_point this out parameter is set to the starting
/// point of the snake.
template<typename RandomAccessOutputIterator>
void
find_snake_start_point(const RandomAccessOutputIterator a_begin,
		       const RandomAccessOutputIterator a_end,
		       const RandomAccessOutputIterator b_begin,
		       const RandomAccessOutputIterator b_end,
		       const point& snake_end_point,
		       bool forward,
		       point& snake_start_point)
{
  assert(is_match_point(a_begin, a_end,
			b_begin, b_end,
			snake_end_point));

  int incr = forward ? -1 : 1;

  for (point p  = snake_end_point;
       is_match_point(a_begin, a_end,
		      b_begin, b_end,
		      p);
       p += incr)
      snake_start_point = p;
}

/// Returns the middle snake of two sequences A and B, as well as the
/// length of their shortest editing script.
///
///  This uses the "linear space refinement" algorithm presented in
/// section 4b in the paper.  As the paper says, "The idea for doing
/// so is to simultaneously run the basic algorithm in both the
/// forward and reverse directions until furthest reaching forward and
/// reverse paths starting at opposing corners ‘‘overlap’’."
///
/// @param a_begin an iterator pointing to the begining of sequence A.
///
/// @param a_end an iterator pointing to the end of sequence A.  Note
/// that this points right /after/ the end of vector A.
///
/// @param b_begin an iterator pointing to the begining of sequence B.
///
/// @param b_end an iterator pointing to the end of sequence B.  Note
/// that this points right /after/ the end of vector B
///
/// @param snake_start this is set by the function iff it returns
/// true.  It's the coordinates (starting from 1) of the beginning of
/// the snake using @a a_begin as the base for the abscissa and
/// b_begin as the base for the ordinate.
///
/// @param snake_end this is set by the function iff it returns true.
/// It's the coordinates (starting from 1) of the end of the snake
/// using @a a_begin as the base for the abscissa and @a b_begin as
/// the base for the ordinate.  It points to the last point of the
/// snake.
///
/// @return true is the snake was found, false otherwise.
template<typename RandomAccessOutputIterator>
bool
compute_middle_snake(RandomAccessOutputIterator a_begin,
		     RandomAccessOutputIterator a_end,
		     RandomAccessOutputIterator b_begin,
		     RandomAccessOutputIterator b_end,
		     point& snake_begin,
		     point& snake_end,
		     int& ses_len)
{
  int a_size = a_end - a_begin;
  int N = a_size;
  int b_size = b_end - b_begin;
  int M = b_size;
  int delta = N - M;
  d_path_vec forward_d_paths(a_size, b_size);
  d_path_vec reverse_d_paths(a_size, b_size);
  // These two points below record the end point of the last non-empty
  // snake that we've seen while building the path.
  point last_forward_snake_point, last_reverse_snake_point;
  // These points below are the top leftmost point and bottom
  // right-most points of the edit graph.
  point first_point(-1, -1), last_point(a_size -1, b_size -1);

  // We want the initial step (D = 0, k = 0 in the paper) to find a
  // furthest reaching point on diagonal k == 0; For that, we need the
  // value of x for k == 1; So let's set that value to -1; that is for
  // k == 1 (diagonal 1), the point in the edit graph is (-1,-2).
  // That way, to get the furthest reaching point on diagonal 0 (k ==
  // 0), we go down from (-1,-2) on diagonal 1 and we hit diagonal 0
  // on (-1,-1); that is the starting value that the algorithm expects
  // for k == 0.
  forward_d_paths[1] = -1;

  // Similarly for the reverse paths, for diagonal delta + 1 (note
  // that diagonals are centered on delta, unlike for forward paths
  // where they are centered on zero), we set the initial point to
  // (a_size, b_size - 1).  That way, at step D == 0 and k == delta,
  // to reach diagonal delta from the point (a_size, b_size - 1) on
  // diagonal delta + 1, we just have to move left, and we hit
  // diagonal delta on (a_size - 1, b_size -1); that is the starting
  // point value the algorithm expects for k == 0 in the reverse case.
  reverse_d_paths[delta + 1] = a_size;

  int d_max = (M + N) / 2;
  for (int d = 0; d <= d_max; ++d)
    {
      // First build forward paths.
      for (int k = -d; k <= d; k += 2)
	{
	  point forward_end, reverse_end;
	  bool found = end_of_fr_d_path_in_k(k, d,
					     a_begin, a_end,
					     b_begin, b_end,
					     forward_d_paths,
					     forward_end);
	  if (!found)
	    continue;

	  maybe_record_match_point(a_begin, a_end,
				   b_begin, b_end,
				   forward_end,
				   last_forward_snake_point);

	  // As the paper says criptically in 4b while explaining the
	  // middle snake algorithm:
	  //
	  // "Thus when delta is odd, check for overlap only while
	  //  extending forward paths ..."
	  if ((delta % 2)
	      && (k >= (delta - (d - 1))) && (k <= (delta + (d - 1))))
	    {
	      reverse_end.x(reverse_d_paths[k]);
	      reverse_end.y(reverse_end.x() - k);
	      if (ends_of_furthest_d_paths_overlap(forward_end, reverse_end))
		{
		  // So the forward path overlaps the reverse path.
		  // The paper says in the linear space refinement in
		  // 4b that:
		  //
		  //	As soon as a pair of opposing and furthest
		  //	reaching paths overlap, stop and report the
		  //	overlapping snake as the middle snake of an
		  //	optimal path.
		  //
		  // I guess now the crux of the mater is to find that
		  // pesky "overlapping snake".  And unfortunately the
		  // paper is not extremely precise about how to find
		  // it.
		  //
		  // In the algorithm one can read:
		  //
		  //	The last snake of the forward path is the
		  //	middle snake
		  //
		  //	So here is how I think we can pull this off:
		  //
		  //	Once we detected the overlap, if there is
		  //	already a non-empty snake encountered in the
		  //	current path, stop and report it as the middle
		  //	snake.  Otherwise if there is no non-empty
		  //	snake seen yet, keep building the forward path
		  //	until the end of the path.  If we found a
		  //	non-empty snake, report it.  Otherwise, there
		  //	is no non-empty middle snake and the two
		  //	sequences have no longest common sub-sequence.
		  if (!last_forward_snake_point
		      && forward_end != last_point)
		    {
		      // So we didn't see any non-empty snake before
		      // the paths overlap.  Let's keep building the
		      // path and record the non-empty snakes we see
		      // on the way.
		      int k_prime = k + 2;
		      for (; k_prime <= d; k_prime += 2)
			{
			  forward_end.clear();
			  end_of_fr_d_path_in_k(k_prime, d,
						a_begin, a_end,
						b_begin, b_end,
						forward_d_paths,
						forward_end);
			  maybe_record_match_point(a_begin, a_end,
						   b_begin, b_end,
						   forward_end,
						   last_forward_snake_point);
			  if (forward_end == last_point)
			    break;
			}
		      for (int remaining_of_d = d + 1;
			   remaining_of_d <= d_max;
			   ++remaining_of_d)
			{
			  for (k_prime = -remaining_of_d;
			       k_prime <= remaining_of_d;
			       k_prime += 2)
			    {
			      forward_end.clear();
			      end_of_fr_d_path_in_k(k_prime, remaining_of_d,
						    a_begin, a_end,
						    b_begin, b_end,
						    forward_d_paths,
						    forward_end);
			      maybe_record_match_point(a_begin, a_end,
						       b_begin, b_end,
						       forward_end,
						       last_forward_snake_point);
			      if (forward_end == last_point)
				break;
			    }
			}
		    }

		  // Now report the "last snake" of this path that
		  // overlaped.
		  if (last_forward_snake_point)
		    {
		      ses_len = 2 * d - 1;
		      snake_end = last_forward_snake_point;
		      find_snake_start_point(a_begin, a_end,
					     b_begin, b_end,
					     last_forward_snake_point,
					     /*forward=*/true,
					     snake_begin);
		      return true;
		    }
		}
	    }
	}

      // Now build reverse paths.
      for (int k = -d; k <= d; k += 2)
	{
	  point forward_end, reverse_end;
	  bool found = end_of_frr_d_path_in_k_plus_delta(k, d,
							 a_begin, a_end,
							 b_begin, b_end,
							 reverse_d_paths,
							 reverse_end);

	  if (!found)
	    continue;

	  maybe_record_match_point(a_begin, a_end,
				   b_begin, b_end,
				   reverse_end + 1,
				   last_reverse_snake_point);

	  // And the paper continues by saying:
	  //
	  // "... and when delta is even, check for overlap only while
	  // extending reverse paths."
	  int k_plus_delta = k + delta;
	  if (!(delta % 2)
	      && (k_plus_delta >= -d) && (k_plus_delta <= d))
	    {
	      forward_end.x(forward_d_paths[k_plus_delta]);
	      forward_end.y(forward_end.x() - k_plus_delta);
	      // To detect the overlapping paths and report the middle
	      // snake, we do things similar to the foward path
	      // version above.  Please go read the comments there to
	      // understand what we are doing here.
	      if (ends_of_furthest_d_paths_overlap(forward_end, reverse_end))
		{
		  if (!last_reverse_snake_point
		      && reverse_end != first_point)
		    {
		      int k_prime = k + 2;
		      for (; k_prime <= d; k_prime += 2)
			{
			  end_of_frr_d_path_in_k_plus_delta(k_prime, d,
							    a_begin, a_end,
							    b_begin, b_end,
							    reverse_d_paths,
							    reverse_end);
			  maybe_record_match_point(a_begin, a_end,
						   b_begin, b_end,
						   reverse_end + 1,
						   last_reverse_snake_point);
			  if (reverse_end == first_point)
			    break;
			}
		      for (int remaining_of_d = d + 1;
			   remaining_of_d <= d_max;
			   ++remaining_of_d)
			{
			  for (k_prime = -remaining_of_d;
			       k_prime <= remaining_of_d;
			       k_prime += 2)
			    {
			      end_of_frr_d_path_in_k_plus_delta(k_prime,
								remaining_of_d,
								a_begin, a_end,
								b_begin, b_end,
								reverse_d_paths,
								reverse_end);
			      maybe_record_match_point(a_begin, a_end,
						       b_begin, b_end,
						       reverse_end + 1,
						       last_reverse_snake_point);
			      if (reverse_end == first_point)
				break;
			    }
			}
		    }

		  if (last_reverse_snake_point)
		    {
		      ses_len = 2 * d;
		      snake_end = last_reverse_snake_point;
		      find_snake_start_point(a_begin, a_end,
					     b_begin, b_end,
					     last_reverse_snake_point,
					     /*forward=*/false,
					     snake_begin);
		      return true;
		    }
		}
	    }
	}
    }
  return false;
}

bool
compute_middle_snake(const char* str1, const char* str2,
		     point& snake_begin, point& snake_end,
		     int& ses_len);

/// This prints the middle snake of two strings.
///
/// @param a_begin the beginning of the first string.
///
/// @param b_begin the beginning of the second string.
///
/// @param snake_begin the beginning point of the snake.
///
/// @param snake_end the end point of the snake.  Note that this point
/// is one offset past the end of the snake.
template<typename RandomAccessOutputIterator>
void
print_snake(RandomAccessOutputIterator a_begin,
	    RandomAccessOutputIterator b_begin,
	    const point& snake_begin,
	    const point& snake_end,
	    ostream& out)
{
   if (!(snake_begin && snake_end))
    return;

  out << "middle snake points: ";
  for (int x = snake_begin.x(), y = snake_begin.y();
       x <= snake_end.x() && y <= snake_end.y();
       ++x, ++y)
    {
      assert(a_begin[x] == b_begin[y]);
      out << "(" << x << "," << y << ") ";
    }
  out << "\n";

  out << "middle snake string: ";
  for (int x = snake_begin.x(), y = snake_begin.y();
       x <= snake_end.x() && y <= snake_end.y();
       ++x, ++y)
    out << a_begin[x];

  out << "\n";
}

/// Compute the length of the shortest edit script for two sequences a
/// and b.  This is done using the "Greedy LCS/SES" of figure 2 in the
/// paper.  It can walk the edit graph either foward (when reverse is
/// false) or backward starting from the end (when reverse is true).
///
/// Here, note that the real content of a and b should start at index
/// 1, for this implementatikon algorithm to match the paper's
/// algorithm in a straightforward manner.  So pleast make sure that
/// at index 0, we just get some non-used value.
///
/// @param a the first sequence we care about.
///
/// @param b the second sequence we care about.
///
/// @param v the vector that contains the end points of the furthest
/// reaching d-path and (d-1)-path.
template<typename RandomAccessOutputIterator>
int
ses_len(RandomAccessOutputIterator a_begin,
	RandomAccessOutputIterator a_end,
	RandomAccessOutputIterator b_begin,
	RandomAccessOutputIterator b_end,
	d_path_vec& v, bool reverse)
{
  unsigned a_size = a_end - a_begin;
  unsigned b_size = b_end - b_begin;

  assert(v.max_d() == a_size + b_size);

  int delta = a_size - b_size;

  if (reverse)
    // Set a fictitious (M, N-1) into v[1], to find the furthest
    // reaching reverse 0-path (i.e, when we are at d == 0 and k == 0).
    v[delta + 1] = a_size - 1;
  else
    // Set a fictitious (-1,-2) point into v[1], to find the furthest
    // reaching forward 0-path (i.e, when we are at d == 0 and k == 0).
    v[1] = -1;

  for (unsigned d = 0; d <= v.max_d(); ++d)
    {
      for (int k = -d; k <= (int) d; k += 2)
	{
	  point end;
	  if (reverse)
	    {
	      end_of_frr_d_path_in_k_plus_delta(k, d,
						a_begin, a_end,
						b_begin, b_end,
						v, end);
	      // If we reached the upper left corner of the edit graph then
	      // we are done.
	      if (end.x() == -1 && end.y() == -1)
		return d;
	    }
	  else
	    {
	      end_of_fr_d_path_in_k(k, d,
				    a_begin, a_end,
				    b_begin, b_end,
				    v, end);
	      // If we reached the lower right corner of the edit
	      // graph then we are done.
	      if ((end.x() == (int) a_size - 1)
		  && (end.y() == (int) b_size - 1))
		return d;
	    }
	}
    }
  return 0;
}

int
ses_len(const char* str1,
	const char* str2,
	bool reverse = false);

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
///@param b_base an iterator to the base of the second sequence to
///consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @param ses_len the length of the ses above.  Normally this can be
/// retrived from ses.length(), but this parameter is here for sanity
/// check purposes.  The function computes the length of the ses in two
/// redundant redundant ways and ensures that both methods lead to the
/// same result.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses,
	     int& ses_len)
{
  int a_size = a_end - a_begin;
  int b_size = b_end - b_begin;

  if (a_size == 0 || b_size == 0)
    {
      if (a_size > 0 && b_size == 0)
	// All elements of the first sequences have been deleted.  So add
	// the relevant deletions to the edit script.
	for (RandomAccessOutputIterator i = a_begin; i < a_end; ++i)
	  ses.deletions().push_back(deletion(i - a_base));

      if (b_size > 0 && a_size == 0)
	{
	  // All elements present in the second sequence are part of
	  // an insertion into the first sequence at a_end.  So add
	  // that insertion to the edit script.
	  int a_full_size = a_end - a_base;
	  int insertion_index = a_full_size ? a_full_size - 1 : 0;
	  insertion ins(insertion_index);
	  for (RandomAccessOutputIterator i = b_begin; i < b_end; ++i)
	    ins.inserted_indexes().push_back(i - b_base);

	  ses.insertions().push_back(ins);
	}
      return;
    }

  int d = 0;
  point middle_begin, middle_end; // end points of the middle snake.
  vector<point> middle; // the  middle snake itself.
  bool has_snake = compute_middle_snake(a_begin, a_end,
					b_begin, b_end,
					middle_begin,
					middle_end, d);
  if (has_snake)
    {
      // So middle_{begin,end} are expressed wrt a_begin and b_begin.
      // Let's express them wrt a_base and b_base.
      unsigned a_offset = a_begin - a_base, b_offset = b_begin - b_base;
      middle_begin.x(middle_begin.x() + a_offset);
      middle_begin.y(middle_begin.y() + b_offset);
      middle_end.x(middle_end.x() + a_offset);
      middle_end.y(middle_end.y() + b_offset);

      for (int x = middle_begin.x(), y = middle_begin.y();
	   x <= middle_end.x() && y <= middle_end.y();
	   ++x, ++y)
	middle.push_back(point(x, y));

      ses_len = d;
    }
  else
    {
      // So there is no middle snake.  That means there is no lcs, so
      // the two sequences are different.

      // In other words, all the elements of the first sequence have
      // been delete ...
      for (RandomAccessOutputIterator i = a_begin; i < a_end; ++i)
	ses.deletions().push_back(deletion(i - a_base));

      // ... and all the elements of the second sequence are insertions
      // that happen at the beginning of the first sequence.
      insertion ins(a_begin - a_base);
      for (RandomAccessOutputIterator i = b_begin; i < b_end; ++i)
	ins.inserted_indexes().push_back(i - b_base);
      ses.insertions().push_back(ins);

      ses_len = a_size + b_size;
      assert(ses_len == ses.length());
      return;
    }

  if (d > 1)
    {
      int tmp_ses_len = 0;
      compute_diff(a_base, a_begin, a_base + middle_begin.x(),
		   b_base, b_begin, b_base + middle_begin.y(),
		   lcs, ses, tmp_ses_len);

      lcs.insert(lcs.end(), middle.begin(), middle.end());

      tmp_ses_len = 0;
      edit_script tmp_ses;
      compute_diff(a_base, a_base + middle_end.x() + 1, a_end,
		   b_base, b_base + middle_end.y() + 1, b_end,
		   lcs, tmp_ses, tmp_ses_len);
      ses.append(tmp_ses);
    }
  else if (d == 1)
    {
      // So we found a non-empty middle snake in an optimal path that
      // is 1-length.  That means there is exactly one insertion or
      // deletion.
      //
      // The general pattern of the optimal path is this one: A
      // possibly empty snake, followed by a non-diagonal edge,
      // followed by a possibly empty snake.  Note that only one of
      // the snakes can be empty; in general the two snakes can be
      // non-empty.
      //
      // As d == 1, the path ends either on k equals to either -1 or
      // 1.  That is, delta is odd.  So we found the middle_snake
      // while building a forward path.
      //
      // If the (non-empty) middle snake reported comes *after* the
      // non-diagonal edge, we should not forget to add the first
      // snake (that starts at point (0,0)) to the LCS.
      //
      // Also, we need to find the non-diagonal edge (that comes
      // *after* the first (possibly empty) snake that starts at point
      // (0,0) and add it to the list of insertions/deletions.
      int x = 0, y = 0;
      while (x < middle_begin.x() && y < middle_begin.y())
	{
	  if (a_base[x] == b_base[y])
	    {
	      lcs.push_back(point(x, y));
	      ++x, ++y;
	    }
	  else
	    break;
	}

      // Now if there is an element of the first sequence which index
      // is not an abscissa of a point in the non-empty snake, that
      // means that element is deleted; i.e, we found the non-diagonal
      // edge we are looking for.
      for (; x < a_size; ++x)
	{
	  if (x < middle_begin.x() || x > middle_end.x())
	    {
	      deletion del(x);
	      ses.deletions().push_back(deletion(x));
	      break;
	    }
	}
      // Now if there is an element of the second sequence which index
      // is not an ordinate of a point in the non-empty snake, that
      // means that element got inserted; i.e, we found the
      // non-diagonal edge we are looking for.
      for (; y < b_size; ++y)
	{
	  if (y > middle_end.y() || y < middle_begin.y())
	    {
	      insertion ins(middle_end.x());
	      ins.inserted_indexes().push_back(y);
	      ses.insertions().push_back(ins);
	      break;
	    }
	}

      // ... and append the middle snake to the longest common
      // subsequence.
      lcs.insert(lcs.end(), middle.begin(), middle.end());
      ses_len = 1;
    }
  else if (d == 0)
    {
      // Obviously on the middle snake is part of the solution, as
      // there is no edit script; iow, the two sequences are
      // identical.
      lcs.insert(lcs.end(), middle.begin(), middle.end());
      ses_len = 0;
    }

  assert(ses_len == ses.length());
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
///@param b_base an iterator to the base of the second sequence to
///consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param lcs the resulting lcs.  This is set iff the function
/// returns true.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     vector<point>& lcs,
	     edit_script& ses)
{
  int ses_len = 0;

  compute_diff(a_base, a_begin, a_end,
	       b_base, b_begin, b_end,
	       lcs, ses, ses_len);
}

/// Compute the longest common subsequence of two (sub-regions of)
/// sequences as well as the shortest edit script from transforming
/// the first (sub-region of) sequence into the second (sub-region of)
/// sequence.
///
/// A sequence is determined by a base, a beginning offset and an end
/// offset.  The base always points to the container that contains the
/// sequence to consider.  The beginning offset is an iterator that
/// points the beginning of the sub-region of the sequence that we
/// actually want to consider.  The end offset is an iterator that
/// points to the end of the sub-region of the sequence that we
/// actually want to consider.
///
/// This uses the LCS algorithm of the paper at section 4b.
///
/// @param a_base the iterator to the base of the first sequence.
///
/// @param a_start an iterator to the beginning of the sub-region
/// of the first sequence to actually consider.
///
/// @param a_end an iterator to the end of the sub-region of the first
/// sequence to consider.
///
///@param b_base an iterator to the base of the second sequence to
///consider.
///
/// @param b_start an iterator to the beginning of the sub-region
/// of the second sequence to actually consider.
///
/// @param b_end an iterator to the end of the sub-region of the
/// second sequence to actually consider.
///
/// @param ses the resulting shortest editing script.
///
/// @return true upon successful completion, false otherwise.
template<typename RandomAccessOutputIterator>
void
compute_diff(RandomAccessOutputIterator a_base,
	     RandomAccessOutputIterator a_begin,
	     RandomAccessOutputIterator a_end,
	     RandomAccessOutputIterator b_base,
	     RandomAccessOutputIterator b_begin,
	     RandomAccessOutputIterator b_end,
	     edit_script& ses)
{
  vector<point> lcs;

  compute_diff(a_base, a_begin, a_end,
	       b_base, b_begin, b_end,
	       lcs, ses);
}

void
compute_lcs(const char* str1, const char* str2, int &ses_len, string& lcs);

void
compute_ses(const char* str1, const char* str2, edit_script& ses);

/// Display an edit script on standard output.
///
/// @param es the edit script to display
///
/// @param str1_base the first string the edit script is about.
///
/// @pram str2_base the second string the edit script is about.
template<typename RandomAccessOutputIterator>
void
display_edit_script(const edit_script& es,
		    const RandomAccessOutputIterator str1_base,
		    const RandomAccessOutputIterator str2_base,
		    ostream& out)
{
  if (es.num_deletions() == 0)
    out << "no deletion:\n";
  else if (es.num_deletions() == 1)
    {
      out << "1 deletion:\n"
	  << "\t happened at index: ";;
    }
  else
    {
      out << es.num_deletions() << " deletions:\n"
	   << "\t happened at indexes: ";
    }

  for (vector<deletion>::const_iterator i = es.deletions().begin();
       i != es.deletions().end();
       ++i)
    {
      if (i != es.deletions().begin())
	out << ", ";
      out << i->index() << " (" << str1_base[i->index()] << ")";
    }
  out << "\n\n";

  if (es.num_insertions() == 0)
    out << "no insertion\n";
  else if (es.num_insertions() == 1)
    out << "1 insertion\n";
  else
      out << es.num_insertions() << " insertions:\n";
  for (vector<insertion>::const_iterator i = es.insertions().begin();
       i != es.insertions().end();
       ++i)
    {
      out << "\t after index of first sequence: " << i->insertion_point_index()
	   << " (" << str1_base[i->insertion_point_index()] << ")\n";

      if (!i->inserted_indexes().empty())
	out << "\t\t inserted indexes from second sequence: ";

      for (vector<int>::const_iterator j = i->inserted_indexes().begin();
	   j != i->inserted_indexes().end();
	   ++j)
	{
	  if (j != i->inserted_indexes().begin())
	    out << ", ";
	  out << *j << " (" << str2_base[*j] << ")";
	}
      out << "\n";
    }
  out << "\n\n";
}

}//end namespace diff_utils

}//end namespace abigail
