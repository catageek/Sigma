#ifndef UNIQUEIDPROVIDER_H_INCLUDED
#define UNIQUEIDPROVIDER_H_INCLUDED

#if defined(_MSC_VER)
#include "intrin.h"
#endif

#include "BitArray.hpp"

namespace Sigma {
	class UniqueIDProvider {
	public:
		UniqueIDProvider() {};

		virtual ~UniqueIDProvider() {};

		static size_t GetID() {
			auto ret = GetFirstFreeID();
			ba_ptr[ret] = false;
			last_free = ret;
			return ret;
		};

		static bool Reserve(size_t id) {
			if (ba_ptr[id]) {
				ba_ptr[id] = false;
				return true;
			}
			return false;
		}

		static void Release(size_t id) {
			ba_ptr[id] = true;
			if (last_free > id) {
					last_free = id;
			}
		};

		static bool IsInUse(size_t id) {
			return (id < ba_ptr.size()) && (! ba_ptr[id]);
		};

	private:
		static size_t GetFirstFreeID() {
			if (ba_ptr.size() == 0) {
				return 0;
			}
			const size_t max_block = (ba_ptr.size() / 32) + 1;
			const unsigned int* data = ba_ptr.data();
			auto current_block = last_free >> 5;
			for(; current_block < max_block; current_block++) {
				if (data[current_block] != 0) {
					#if defined(__GNUG__)
					return (current_block << 5) + __builtin_ctzl(data[current_block]);
					#elif defined(_MSC_VER)
					return (current_block << 5) + (31 - __lzcnt(data[current_block]));
					#else
					unsigned int t = 1;
					unsigned short r = 0;
					while ((data[current_block] & t) == 0) {
						t = t << 1;
						r++;
					}
					return (current_block << 5) + r;
					#endif
				}
			}
			return current_block << 5;
		};

		static BitArray<unsigned int> ba_ptr;
		static size_t last_free;
	};
}


#endif // UNIQUEIDPROVIDER_H_INCLUDED
