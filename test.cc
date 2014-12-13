/*
 * Copyright 2014 jafp.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <mutex>

#include "worker_pool.h"

using namespace std;

struct MyData {
	int value;
	double another_value;
};

std::vector<unsigned int> value_list;
std::mutex mtx;

// C+11 Random generator stuff
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 20);

static void procFn(const MyData& value) {
	int t = dis(gen);
	std::this_thread::sleep_for(chrono::milliseconds(t));

	mtx.lock();
	value_list.push_back(value.value);
	std::this_thread::sleep_for(chrono::milliseconds(5));
	mtx.unlock();

	delete &value;
}

int main() 
{
	raw::WorkerPool<MyData, 2> pool(procFn);
	pool.prepare();

	for (int i = 0; i < 100; i++) {
		MyData* data = new MyData;
		data->value = i;
		data->another_value = pow(i, i);
		pool.queue(*data);
	}

 	pool.join();

	for (unsigned int i = 0; i < value_list.size(); i++) {
		cout << value_list[i] << ", ";
	}
	cout << endl;

	return 0;
}