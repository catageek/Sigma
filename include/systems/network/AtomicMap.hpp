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

		void Insert(const K& key, const T& element) {
			mtx.lock();
			q->insert(std::make_pair(key, element));
			mtx.unlock();
		}

		bool Erase(const K& key) {
			mtx.lock();
			q->erase(key);
			mtx.unlock();
		}

		bool Pop(const K& key, T& element) {
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

	private:
		atomic_map<K,T> q;
		std::mutex mtx;
	};
}

#endif // ATOMICMAP_HPP_INCLUDED
