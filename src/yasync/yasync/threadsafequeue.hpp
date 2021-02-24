#pragma once

// A simple thread-safe queue implementation based on std::list<>::splice
// after a tip in a talk by Sean Parent of Adobe.
//
// Uses standard library threading and synchronization primitives together
// with a std::list<> container for implementing a thread-safe queue.  The
// only special thing is that the queue uses std::list<>::splice in the
// locked region to minimize locked time.
//
// Also implements a maximal size and can thus be used as a buffer between
// the elements in a pipeline with limited buffer bloat.
//
// Author: Manuel Holtgrewe <manuel.holtgrewe@fu-berlin.de>
// License: 3-clause BSD

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <list>
#include <optional>

// Simple thread-safe queue with maximal size based on std::list<>::splice().
//
// Implemented after a tip by Sean Parent at Adobe.
// https://gist.github.com/holtgrewe/8728757

template <typename T>
class ThreadSafeQueue
{
private:
    // The current size.
    size_t currentSize = 0, popw = 0;
	bool closed = false;
    // The condition variables to use for pushing/popping.
    std::condition_variable cvPop;
    // The mutex for locking the queue.
    std::mutex mutex;
    // The list that the queue is implemented with.
    std::list<T> list;
public:
	std::condition_variable* cvIdle = nullptr;
	unsigned thresIdle = 0;

    // Initialize the queue with a maximal size.
    explicit ThreadSafeQueue(){}

	unsigned currentIdle(){
		std::unique_lock<std::mutex> lock(mutex);
		return popw;
	}

    // Push v to queue.  Blocks if queue is full.
    void push(T const & v)
    {
        // Create temporary queue.
        decltype(list) tmpList;
        tmpList.push_back(v);

        // Pushing with lock, only using list<>::splice().
        {
            std::unique_lock<std::mutex> lock(mutex);

            // Push to queue.
            currentSize += 1;
            list.splice(list.end(), tmpList, tmpList.begin());

            // Wake up one popping thread.
            if (currentSize == 1u)
                cvPop.notify_one();
        }
    }

    // Push to queue with rvalue reference.
    void push(T && v)
    {
       // Create temporary queue.
        decltype(list) tmpList;
        tmpList.push_back(v);

        // Pushing with lock, only using list<>::splice().
        {
            std::unique_lock<std::mutex> lock(mutex);

            // Push to queue.
            currentSize += 1;
            list.splice(list.end(), tmpList, tmpList.begin());

            // Wake up one popping thread.
            if (currentSize == 1u)
                cvPop.notify_one();
        }
    }

    // Pop value from queue and write to v.
    //
    // If this succeeds, OK is returned.  CLOSED is returned if the queue is empty and was closed.
    std::optional<T> pop()
    {
        decltype(list) tmpList;

        // Pop into tmpList which is finally written out.
        {
            std::unique_lock<std::mutex> lock(mutex);
			popw++;

            // If there is no item then we wait until there is one.
            while(list.empty() && !closed){
				if(cvIdle && thresIdle && popw >= thresIdle) cvIdle->notify_all();
                cvPop.wait(lock);
			}
			if(closed){
				popw--;
				return std::nullopt;
			}

            // If we reach here then there is an item, get it.
            currentSize -= 1;
            tmpList.splice(tmpList.begin(), list, list.begin());
			popw--;
        }

        return tmpList.front();
    }

	void close(){
		if(closed) return;
		std::unique_lock<std::mutex> lock(mutex);
		closed = true;
		cvPop.notify_all();
	}

};
