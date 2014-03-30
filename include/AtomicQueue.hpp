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

		atomic_queue<T> Poll() {
			if (! q->size()) {
				return atomic_queue<T>();
			}
			mtx.lock();
			auto ret = std::move(q);
			q.reset(new std::list<T>());
			mtx.unlock();
			return ret;
		}

		void Push(T element) {
			mtx.lock();
			q->push_back(element);
			mtx.unlock();
		}

		bool Pop(T& element) {
			std::unique_lock<std::mutex> locker(mtx);
			if(q->empty()) {
				return false;
			}
			element = std::move(q->front());
			q->pop_front();
			return true;
		}

		bool Empty() const {
			return q->empty();
		}

	private:
		atomic_queue<T> q;
		std::mutex mtx;
	};
}


#endif // WORKERQUEUE_HPP_INCLUDED
