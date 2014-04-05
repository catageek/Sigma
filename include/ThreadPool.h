#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#define MAX_CONCURRENT_THREAD 4

#define		STOP		0
#define 	SPLIT		1
#define		CONTINUE	2
#define		REQUEUE		3
#define		REPEAT		4

#include <chrono>
#include <functional>
#include <condition_variable>
#include <list>
#include <forward_list>
#include <iterator>
#include "Sigma.h"

namespace Sigma {

	typedef std::function<int(void)> block_t;
	typedef std::list<block_t> chain_t;
	typedef std::forward_list<chain_t> workflow_tree_t;
	template<class T> struct TaskReq;
//	typedef std::function<TaskReq*(void)> worker_t;

	template<typename T>
	struct identity { typedef T type; };

	class ThreadPool;

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
		TaskReq(chain_t& chain) : block(chain.cbegin()), block_end(chain.cend()), TaskQueueElement(std::chrono::steady_clock::now()) {};

		TaskReq(std::shared_ptr<chain_t>&& chain) : chain(std::move(chain)), block(this->chain->cbegin()), block_end(this->chain->cend()), TaskQueueElement(std::chrono::steady_clock::now()) {};

		virtual ~TaskReq() {};

		bool operator==(const TaskQueueElement& tqe) const {
			auto rhs = dynamic_cast<const TaskReq<chain_t>*>(&tqe);
			return rhs && rhs->block == this->block;
		}

		void RunTask() override {
			for(auto& b = block, b_end = block_end; b != b_end; ++b) {
				auto s = (*b)();
				switch(s) {
				case REQUEUE:
					LOG_DEBUG << "Requeuing...";
					// "*this" is now undefined
					queue_task(std::make_shared<TaskReq<chain_t>>(std::move(*this)));
				case STOP:
					return;
				case SPLIT:
					// Queue a thread to execute this block again, and go to the next block
					LOG_DEBUG << "Splitting...";
					queue_task(std::make_shared<TaskReq<chain_t>>(*this));
				case REPEAT:
					LOG_DEBUG << "Rewinding...";
					--b;
				case CONTINUE:
					LOG_DEBUG << "Jumping to next block...";
				default:
					break;
				}
			}
		}

		static void Initialize(std::function<void(std::shared_ptr<TaskReq<chain_t>>)>&& f) { queue_task = f; };

	private:
		static std::function<void(std::shared_ptr<TaskReq<chain_t>>)> queue_task;
		static std::function<void(std::shared_ptr<TaskReq<chain_t>>)> execute_task;
		const std::shared_ptr<chain_t> chain;
		chain_t::const_iterator block;
		chain_t::const_iterator block_end;
	};

	class ThreadPool {
	public:
		static void Initialize(unsigned int nr_thread);

		template<class T>
		static void Execute(const std::shared_ptr<TaskReq<T>>& task) {
			task->RunTask();
		}

		template<class T>
		static void Queue(std::shared_ptr<TaskReq<T>>& task) {
			return Queue(task, identity<T>());
		}

		template<class T>
		static void Queue(std::shared_ptr<TaskReq<T>>&& task) {
			return Queue(std::move(task), identity<T>());
		}

	private:

		template<class T,class U>
		static void Queue(U&& task, identity<T>) {
			std::unique_lock<std::mutex> locker(m_queue);
			taskqueue.push_back(std::forward<U>(task));
			queuecheck.notify_one();
		}

		template<class U>
		static void Queue(U&& task, identity<chain_t>) {
			std::unique_lock<std::mutex> locker(m_queue);
/*			for (auto t : taskqueue) {
				if (t == task) {
					// No duplicate entry
					return;
				}
			}
*/			taskqueue.push_back(std::forward<U>(task));
			LOG_DEBUG << "notify";
			queuecheck.notify_one();
		}

		static void Poll();

		static std::list<std::shared_ptr<TaskQueueElement>> taskqueue;
		static int counter;
		static std::mutex m_queue;
		static std::mutex m_count;
		static std::mutex m_print;
		static std::condition_variable queuecheck;
	};
}

#endif // THREADPOOL_H_INCLUDED
