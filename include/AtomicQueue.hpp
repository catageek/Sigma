#ifndef WORKERQUEUE_HPP_INCLUDED
#define WORKERQUEUE_HPP_INCLUDED

#include <mutex>
#include <atomic>
#include <list>
#include <memory>

namespace Sigma {
	template<class T>
	using atomic_queue = std::unique_ptr<std::list<T>>;

	template<class T>
	class AtomicQueue {
	public:
		AtomicQueue() : q(atomic_queue<T>(new std::list<T>)) {};

		atomic_queue<T> Poll() const {
			mtx.lock();
			if (! q->size()) {
				return atomic_queue<T>();
			}
			auto ret = std::move(q);
			q.reset(new std::list<T>());
			mtx.unlock();
			return ret;
		}

		template<class U>
		void Push(U&& element) const {
			mtx.lock();
			q->push_back(std::forward<U>(element));
			mtx.unlock();
		}

		template<class U>
		void PushList(U&& list) const {
			mtx.lock();
			q->splice(q->end(), std::forward<U>(list));
			mtx.unlock();
		}

		bool Pop(T& element) const {
			std::unique_lock<std::mutex> locker(mtx);
			if(q->empty()) {
				return false;
			}
			element = std::move(q->front());
			q->pop_front();
			return true;
		}

		bool Empty() const {
			std::unique_lock<std::mutex> locker(mtx);
			return q->empty();
		}

	private:
		mutable atomic_queue<T> q;
		mutable std::mutex mtx;
	};
}


#endif // WORKERQUEUE_HPP_INCLUDED
