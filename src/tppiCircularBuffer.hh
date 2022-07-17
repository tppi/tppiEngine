/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
Reference: https://github.com/embeddedartistry/embedded-resources
HanXiChangLong, PanYu, GuangZhou, MA 510000 China
**********/

#ifndef TPPI_CIRCULAR_BUFFER_HH_
#define TPPI_CIRCULAR_BUFFER_HH_

#include <array>
#include <mutex>
#include <optional>

template<class T>
class CircularBuffer
{
public:
    explicit CircularBuffer(const size_t size = 0) :
      TElemCount{size}
    {      
        buf_ = new T[TElemCount];
    }
    
    ~CircularBuffer()
    {
        delete []buf_;
    }
    
	void put(T item) noexcept
	{
		std::lock_guard<std::recursive_mutex> lock(mutex_);

		buf_[head_] = item;

		if(full_)
		{
			tail_ = (tail_ + 1) % TElemCount;
		}

		head_ = (head_ + 1) % TElemCount;

		full_ = head_ == tail_;
	}

	bool get(T * data) const noexcept
	{
		std::lock_guard<std::recursive_mutex> lock(mutex_);

		if(empty())
		{
			return false;
		}

		// Read data and advance the tail (we now have a free space)
		*data = buf_[tail_];
		full_ = false;
		tail_ = (tail_ + 1) % TElemCount;

		return true;
	}

	void reset() noexcept
	{
		std::lock_guard<std::recursive_mutex> lock(mutex_);
		head_ = tail_;
		full_ = false;
	}

	bool empty() const noexcept
	{
		// Can have a race condition in a multi-threaded application
		std::lock_guard<std::recursive_mutex> lock(mutex_);
		// if head and tail are equal, we are empty
		return (!full_ && (head_ == tail_));
	}

	bool full() const noexcept
	{
		// If tail is ahead the head by 1, we are full
		return full_;
	}

	size_t capacity() const noexcept
	{
		return TElemCount;
	}

	size_t size() const noexcept
	{
		// A lock is needed in size ot prevent a race condition, because head_, tail_, and full_
		// can be updated between executing lines within this function in a multi-threaded
		// application
		std::lock_guard<std::recursive_mutex> lock(mutex_);

		size_t size = TElemCount;

		if(!full_)
		{
			if(head_ >= tail_)
			{
				size = head_ - tail_;
			}
			else
			{
				size = TElemCount + head_ - tail_;
			}
		}

		return size;
	}

  private:
  T *buf_;
	mutable std::recursive_mutex mutex_; 
	mutable size_t head_ = 0;
	mutable size_t tail_ = 0;
	mutable size_t TElemCount = 0;
	mutable bool full_ = 0;
    
};

#endif
