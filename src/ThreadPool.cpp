#include "ThreadPool.h"
#include <thread>
#include <iostream>

namespace Sigma {
	std::function<void(std::shared_ptr<TaskReq<chain_t>>)> TaskReq<chain_t>::queue_task;

	std::list<std::shared_ptr<TaskQueueElement>> ThreadPool::taskqueue;
	int ThreadPool::counter = 0;
	std::mutex ThreadPool::m_queue;
	std::mutex ThreadPool::m_count;
	std::mutex ThreadPool::m_print;
	std::condition_variable ThreadPool::queuecheck;

	void ThreadPool::Initialize(unsigned int nr_thread) {
		for (auto i = 0; i < nr_thread; ++i) {
			std::thread(&ThreadPool::Poll).detach();
		}
		TaskReq<chain_t>::Initialize([&](std::shared_ptr<TaskReq<chain_t>> c) { Queue(c); });
	}

	void ThreadPool::Poll() {
		{
			std::unique_lock<std::mutex> locker(m_print);
		}
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
				std::unique_lock<std::mutex> locker(m_print);
			}
			{
				std::unique_lock<std::mutex> locker(m_count);
				if(counter >= MAX_CONCURRENT_THREAD) {
					{
						std::unique_lock<std::mutex> locker(m_print);
						LOG_DEBUG << "Too many workers. Waiting...";
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
					LOG_DEBUG << "Launching new worker. We have " << counter << " worker(s)";
				}
			}

			task->RunTask();

			{
				std::unique_lock<std::mutex>(m_count);
				// decrement counter
				counter--;
				queuecheck.notify_one();
				{
					std::unique_lock<std::mutex> locker(m_print);
					LOG_DEBUG << "Stopping worker. We have " << counter << " worker(s)";
				}
			}
		}
	}


}
