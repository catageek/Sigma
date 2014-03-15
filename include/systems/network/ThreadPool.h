#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#define MAX_CONCURRENT_THREAD 4

#define 	NEXT	0
#define		STOP	1

#include <chrono>
#include <functional>
#include <condition_variable>
#include <list>
#include <forward_list>
#include <iterator>

namespace Sigma {

	typedef std::function<int(void)> block_t;
	typedef std::forward_list<block_t> chain_t;
	typedef std::forward_list<chain_t> workflow_tree_t;
	struct TaskReq;
	typedef std::function<TaskReq*(void)> worker_t;

	struct TaskReq {
		TaskReq(const chain_t& chain, size_t index) : block_end(chain.cend()), timestamp(std::chrono::steady_clock::now()) {
			block = chain.cbegin();
			std::advance(block, index);
		};
		virtual ~TaskReq() {};
		chain_t::const_iterator block;
		chain_t::const_iterator block_end;
		std::chrono::steady_clock::time_point timestamp;
	};


	class ThreadPool {
	public:
		ThreadPool(unsigned int nr_thread);
		virtual ~ThreadPool() {};

		void Queue(TaskReq* worker);
	private:
		void Poll();
		std::list<TaskReq*> taskqueue;
		int counter;
		std::mutex m_queue;
		std::mutex m_count;
		std::mutex m_print;
		std::condition_variable queuecheck;
	};
}

#endif // THREADPOOL_H_INCLUDED
