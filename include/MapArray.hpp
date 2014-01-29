#ifndef MAPARRAY_HPP_INCLUDED
#define MAPARRAY_HPP_INCLUDED

#include <map>
#include <iterator>
#include "Sigma.h"

namespace Sigma {
	typedef id_t chunk_id;

	template<class T>
	class MapArrayIterator;

	template<class T>
	struct Chunk {
		T data[CONTAINER_CHUNK_SIZE];
	};

	template<class T>
	class MapArray {
	public:
		MapArray() {};
		virtual ~MapArray() {};

		T& at(const id_t id) { return (DataChunk(id)).data[Index(id)]; };
		const T& at(const id_t id) const { return at(id); };

		T& operator[](const id_t id) { return (map_array[ChunkId(id)]).data[Index(id)]; };

		void clear(const id_t id) {}; // TODO

		MapArrayIterator<T> begin() { return MapArrayIterator<T>(map_array.cbegin(), map_array.cend()); };

		MapArrayIterator<T> end() { return MapArrayIterator<T>(map_array.cend(), map_array.cend()); };

		Chunk<T>& DataChunk(const id_t id) { return map_array.at(ChunkId(id)); };

		const Chunk<T>& DataChunk(const id_t id) const { return DataChunk(id); };

		static const chunk_id ChunkId(const id_t id) { return id >> CONTAINER_CHUNK_SHIFT; };

		static const size_t Index(const id_t id) { return id & CONTAINER_CHUNK_MASK; };

	private:
		std::map<chunk_id, Chunk<T>> map_array;
	};

	template<class T>
	class MapArrayIterator : public std::iterator<std::bidirectional_iterator_tag, T> {
	public:
		MapArrayIterator(const typename std::map<id_t, Chunk<T>>::const_iterator& chunk_iterator, const typename std::map<id_t, Chunk<T>>::const_iterator& chunk_end)
			: chunk_iterator(chunk_iterator), chunk_end(chunk_end) {
			if (chunk_iterator != chunk_end) {
				p = chunk_iterator->first << CONTAINER_CHUNK_SHIFT;
			}
		};

		MapArrayIterator(const typename std::map<id_t, Chunk<T>>::const_iterator& chunk_end) : chunk_iterator(chunk_end), chunk_end(chunk_end) {};

		virtual ~MapArrayIterator() {};

		MapArrayIterator(const MapArrayIterator<T>& it) : chunk_iterator(it.chunk_iterator), chunk_end(it.chunk_end) {
			if (chunk_iterator != chunk_end) {
				p = it.p;
			}
		};	// Copy constructor

		MapArrayIterator& operator++() {
			if (chunk_iterator !=  chunk_end && 0 == MapArray<T>::Index(++p)) {
				++chunk_iterator;
				p = chunk_iterator->first << CONTAINER_CHUNK_SHIFT;
			}
			return *this;
		};

		bool operator==(const MapArrayIterator<T>& mai) const {return (chunk_iterator == mai.chunk_iterator) && ((chunk_iterator == chunk_end) ||(p == mai.p)); };
		bool operator!=(const MapArrayIterator<T>& mai) const { return ! (*this == mai); };
		std::pair<id_t,T>& operator*() { return std::make_pair(p, chunk_iterator->second.data[MapArray<T>::Index(p)]); };
		std::pair<id_t,T>* operator->() { tmp_pair = std::make_pair(p, chunk_iterator->second.data[MapArray<T>::Index(p)]); return &tmp_pair; };

	private:
		MapArrayIterator operator++(int) {};
		id_t p;
		typename std::map<id_t, Chunk<T>>::const_iterator chunk_iterator;
		typename std::map<id_t, Chunk<T>>::const_iterator chunk_end;
		// placeholder for returned value of dereference operator
		std::pair<id_t,T> tmp_pair;
	};
}

#endif // MAPARRAY_HPP_INCLUDED
