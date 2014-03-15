#include "systems/network/ThreadPool.h"
#include <thread>
#include <iostream>

namespace Sigma {
	ThreadPool::ThreadPool(unsigned int nr_thread) : counter(0) {
		for (auto i = 0; i < nr_thread; ++i) {
			std::thread(&ThreadPool::Poll, this).detach();
		}
	}

	void ThreadPool::Poll() {
		{
			std::unique_lock<std::mutex> locker(m_print);
			std::cout << "running" << std::endl;
		}
		while(1) {
			TaskReq* task;
			{
				// Wait for a task to do
				std::unique_lock<std::mutex> locker(m_queue);
				if (taskqueue.empty()){
					queuecheck.wait(locker, [&](){return ! taskqueue.empty();});
				}
				// Get the task to do
				task = taskqueue.front();
				taskqueue.pop_front();
			}
			{
				std::unique_lock<std::mutex> locker(m_print);
				std::cout << "get a task" << std::endl;
			}
			{
				std::unique_lock<std::mutex> locker(m_count);
				if(counter >= MAX_CONCURRENT_THREAD) {
					{
						std::unique_lock<std::mutex> locker(m_print);
						std::cout << "waiting counter" << std::endl;
					}
					{
						// Wait the counter to be under MAX_CONCURRENT_THREAD
						queuecheck.wait(locker, [&](){return counter < MAX_CONCURRENT_THREAD;});
					}
				}
				// increment counter when possible
				counter++;
				{
					std::unique_lock<std::mutex> locker(m_print);
					std::cout << "increment counter" << std::endl;
				}
			}

			for(auto block = task->block, b_end = task->block_end; block != b_end; ++block) {
				if((*block)()) {
					break;
				}
			}

			delete task;

			{
				std::unique_lock<std::mutex>(m_count);
				// decrement counter
				{
					std::unique_lock<std::mutex> locker(m_print);
					std::cout << "decrement counter" << std::endl;
				}
				counter--;
				queuecheck.notify_one();
			}
		}
	}

	void ThreadPool::Queue(TaskReq* task) {
		std::unique_lock<std::mutex> locker(m_queue);
		for (auto t : taskqueue) {
			if (t->block == task->block) {
				// No duplicate entry
				return;
			}
		}
		taskqueue.push_back(task);
		queuecheck.notify_one();
	}
}
