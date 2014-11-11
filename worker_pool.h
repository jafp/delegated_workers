/*
 * Copyright 2013 jafp.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef WORKER_POOL_H_
#define WORKER_POOL_H_

#include <cassert>
#include <iostream>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>

using namespace std;

namespace raw {
	
enum WorkerState {
	Ready,
	Done, 
	Running,
	Stopped
};

template<class T>
struct Worker {
	unsigned int number_;
	T* data_;
	WorkerState state_;
	std::mutex lock_;
	std::thread thread_;
};

template<class T, int S = 4>
class WorkerPool {

public:

	typedef std::function<void(const T&)> proc_fn;

	typedef Worker<T> worker_type;

	typedef std::shared_ptr<worker_type> worker_pointer;

	typedef std::vector<worker_pointer> worker_list;

	// Construction/Destruction

	/**
	 * 
	 */
	explicit WorkerPool(proc_fn fn) : proc_fn_(fn), work_no_(0) {};

	/**
	 *
	 */
	~WorkerPool() {};

	// Public interface

	/**
	 *
	 */
	void prepare();

	/**
	 *
	 */
	void join();

	/**
	 *
	 */
	void queue(T& value);	

private:

	// Processing function for each portion of data
	proc_fn proc_fn_;

	// List of workers
	worker_list worker_list_;

	unsigned int work_no_;

	// Method called for each worker
	void process(worker_pointer worker);
};

template<class T, int S>
void WorkerPool<T, S>::prepare() {
	for (int i = 0; i < S; i++) {
		worker_pointer ptr(new Worker<T>);
		ptr->number_ = i;
		ptr->state_ = Ready;

		std::function<void(worker_pointer)> fn = std::bind(
			&WorkerPool<T, S>::process, this, ptr);
		ptr->thread_ = std::thread(fn, ptr);

		worker_list_.push_back(ptr);
	}
}

template<class T, int S>
void WorkerPool<T, S>::join() {
	assert(worker_list_.size() == S);
	for (int i = 0; i < S; i++) {
		worker_pointer ptr = worker_list_[i];
		//cout << "Marking as stopped and joined" << endl;
		ptr->lock_.lock();
		ptr->state_ = Stopped;
		ptr->lock_.unlock();
		ptr->thread_.join();
	}
}

template<class T, int S>
void WorkerPool<T, S>::queue(T& value) {
	bool ok = false;
	unsigned int no = work_no_++;

	do {
		for (int i = 0; i < S; i++) {
			worker_pointer worker = worker_list_[i];

			if (worker->lock_.try_lock()) {
				if (worker->state_ == Ready) {
					worker->data_ = &value;
					worker->state_ = Running;
					worker->number_ = no;
					ok = true;
				}
				worker->lock_.unlock();	
				if (ok) {
					return;
				}
			}
		}
	} while(1);
}

template<class T, int S>
void WorkerPool<T, S>::process(worker_pointer worker) {
	while (worker->state_ != Stopped) {
		if (worker->state_ == Running) {
			worker->lock_.lock();
			proc_fn_(*worker->data_);
			worker->state_ = Ready;
			worker->lock_.unlock();
		}
	}
}

}

#endif
