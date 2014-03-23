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
	template<class T> struct TaskReq;
//	typedef std::function<TaskReq*(void)> worker_t;

	template<typename T>
	struct identity { typedef T type; };

	struct TaskQueueElement {
		TaskQueueElement(std::chrono::steady_clock::time_point&& timestamp) : timestamp(std::move(timestamp)) {};
		virtual ~TaskQueueElement() {};

		virtual bool operator==(const TaskQueueElement& tqe) const = 0;

		virtual void RunTask() = 0;
		std::chrono::steady_clock::time_point timestamp;
	};

	template<class T>
	struct TaskReq : TaskQueueElement {
		TaskReq(T&& funct) : funct(std::forward<T>(funct)), TaskQueueElement(std::chrono::steady_clock::now()) {};
		TaskReq(T& funct) : funct(std::forward<T>(funct)), TaskQueueElement(std::chrono::steady_clock::now()) {};
		virtual ~TaskReq() {};

		bool operator==(const TaskQueueElement& tqe) const {
			return false;
		}

		void RunTask() override {
			funct();
		}
	private:
		T funct;
	};

	template<>
	struct TaskReq<chain_t> : TaskQueueElement {
		TaskReq(const chain_t& chain, size_t index) : block_end(chain.cend()), TaskQueueElement(std::chrono::steady_clock::now()) {
			block = chain.cbegin();
			std::advance(block, index);
		};
		virtual ~TaskReq() {};

		bool operator==(const TaskQueueElement& tqe) const {
			auto rhs = dynamic_cast<const TaskReq<chain_t>*>(&tqe);
			return rhs && rhs->block == this->block;
		}

		void RunTask() override {
			for(auto& b = block, b_end = block_end; b != b_end; ++b) {
				if((*b)()) {
					return;
				}
			}
		}
	private:
		chain_t::const_iterator block;
		chain_t::const_iterator block_end;
	};

	class ThreadPool {
	public:
		ThreadPool(unsigned int nr_thread);
		virtual ~ThreadPool() {};

		template<class T>
		void Queue(std::shared_ptr<TaskReq<T>>& task) {
			return Queue(task, identity<T>());
		}

		template<class T>
		void Queue(std::shared_ptr<TaskReq<T>>&& task) {
			return Queue(std::move(task), identity<T>());
		}

	private:

		template<class T>
		void Queue(std::shared_ptr<TaskReq<T>>& task, identity<T>) {
			std::unique_lock<std::mutex> locker(m_queue);
			taskqueue.push_back(task);
			queuecheck.notify_one();
		}

		template<class T>
		void Queue(std::shared_ptr<TaskReq<T>>&& task, identity<T>) {
			std::unique_lock<std::mutex> locker(m_queue);
			taskqueue.push_back(std::move(task));
			queuecheck.notify_one();
		}

		void Queue(std::shared_ptr<TaskReq<chain_t>>& task, identity<chain_t>) {
			std::unique_lock<std::mutex> locker(m_queue);
			for (auto t : taskqueue) {
				if (t == task) {
					// No duplicate entry
					return;
				}
			}
			taskqueue.push_back(task);
			queuecheck.notify_one();
		}

		void Queue(std::shared_ptr<TaskReq<chain_t>>&& task, identity<chain_t>) {
			std::unique_lock<std::mutex> locker(m_queue);
			for (auto t : taskqueue) {
				if (t == task) {
					// No duplicate entry
					return;
				}
			}
			taskqueue.push_back(std::move(task));
			queuecheck.notify_one();
		}

		void Poll();

		std::list<std::shared_ptr<TaskQueueElement>> taskqueue;
		int counter;
		std::mutex m_queue;
		std::mutex m_count;
		std::mutex m_print;
		std::condition_variable queuecheck;
	};
}

#endif // THREADPOOL_H_INCLUDED
