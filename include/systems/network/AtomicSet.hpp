#ifndef ATOMICSET_HPP_INCLUDED
#define ATOMICSET_HPP_INCLUDED

#include <mutex>
#include <atomic>
#include <set>
#include <memory>

namespace Sigma {
	template<class T>
	using atomic_set = std::unique_ptr<std::set<T>>;

	template<class T>
	class AtomicSet {
	public:
		AtomicSet() : q(atomic_set<T>(new std::set<T>)) {};

		void Insert(T element) {
			mtx.lock();
			q->insert(element);
			mtx.unlock();
		}

		bool Erase(const T& element) {
			mtx.lock();
			q->erase(element);
			mtx.unlock();
		}

		size_t Count(const T& element) const {
			return q->count(element);
		}

	private:
		atomic_set<T> q;
		std::mutex mtx;
	};
}


#endif // ATOMICSET_HPP_INCLUDED
