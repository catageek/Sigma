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
			mtx.lock();
			auto ret = std::move(q);
			q.reset(new std::list<T>());
			mtx.unlock();
			return ret;
		}

		void Add(T element) {
			mtx.lock();
			q->push_back(element);
			mtx.unlock();
		}

		bool Exist(const T& element) const {
			for (auto e = q->cbegin(), qend = q->cend(); e != qend; e++) {
				if (*e == element) {
					return true;
				}
			}
			return false;
		}

	private:
		atomic_queue<T> q;
		std::mutex mtx;
	};
}


#endif // WORKERQUEUE_HPP_INCLUDED
