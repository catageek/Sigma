#ifndef ATOMICMAP_HPP_INCLUDED
#define ATOMICMAP_HPP_INCLUDED

#include <mutex>
#include <atomic>
#include <map>
#include <memory>

namespace Sigma {
	template<class K, class T>
	using atomic_map = std::unique_ptr<std::map<K,T>>;

	template<class K,class T>
	class AtomicMap {
	public:
		AtomicMap() : q(atomic_map<K,T>(new std::map<K,T>)) {};

		void Insert(const K& key, const T& element) const {
			mtx.lock();
			(*q)[key] = element;
			mtx.unlock();
		}

		bool Erase(const K& key) const {
			mtx.lock();
			q->erase(key);
			mtx.unlock();
		}

		bool Pop(const K& key, T& element) const {
			mtx.lock();
			if(q->count(key)) {
				element = std::move(q->at(key));
				q->erase(key);
				mtx.unlock();
				return true;
			}
			mtx.unlock();
			return false;
		}

		T At(const K& key) const {
			mtx.lock();
			auto ret = q->at(key);
			mtx.unlock();
			return ret;
		}

		size_t Count(const K& key) const {
			mtx.lock();
			auto ret = q->count(key);
			mtx.unlock();
			return ret;
		}

		bool Compare(const K& key, const T& element) const {
			std::lock_guard<std::mutex> locker(mtx);
			return q->count(key) && (q->at(key) == element);
		}

	private:
		const atomic_map<K,T> q;
		mutable std::mutex mtx;
	};
}

#endif // ATOMICMAP_HPP_INCLUDED
