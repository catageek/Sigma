#include "ThreadPool.h"
#include <thread>
#include <iostream>
#include <utmpx.h>

namespace Sigma {
	std::function<void(std::shared_ptr<TaskReq<chain_t>>)> TaskReq<chain_t>::queue_task;

	std::list<std::shared_ptr<TaskQueueElement>> ThreadPool::taskqueue;
	int ThreadPool::counter = 0;
	std::mutex ThreadPool::m_queue;
	std::mutex ThreadPool::m_count;
	std::condition_variable ThreadPool::queuecheck;

	void ThreadPool::Initialize(unsigned int nr_thread) {
		LOG_DEBUG << "hardware concurrency " << std::thread::hardware_concurrency();
		for (auto i = 0; i < nr_thread; ++i) {
			std::thread(&ThreadPool::Poll).detach();
		}
		TaskReq<chain_t>::Initialize([&](std::shared_ptr<TaskReq<chain_t>> c) { Queue(c); });
	}

	void ThreadPool::Poll() {
		while(1) {
			std::shared_ptr<TaskQueueElement> task;
			{
				// Wait for a task to do
				std::unique_lock<std::mutex> locker(m_queue);
				while (taskqueue.empty()){
					queuecheck.wait(locker, [&](){return ! taskqueue.empty();});
				}
				// Get the task to do
				task = taskqueue.front();
				taskqueue.pop_front();
			}
			{
				std::unique_lock<std::mutex> locker(m_count);
				while(counter >= MAX_CONCURRENT_THREAD) {
					// Wait the counter to be under MAX_CONCURRENT_THREAD
					queuecheck.wait(locker, [&](){return counter < MAX_CONCURRENT_THREAD;});
				}
				// increment counter when possible
				++counter;
			}

			task->RunTask();

			{
				std::unique_lock<std::mutex>(m_count);
				// decrement counter
				--counter;
				queuecheck.notify_one();
			}
		}
	}


}
