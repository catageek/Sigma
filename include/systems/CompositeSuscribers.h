#ifndef COMPOSITESUSCRIBERS_H_INCLUDED
#define COMPOSITESUSCRIBERS_H_INCLUDED

#include <unordered_map>
#include "BitArray.hpp"

namespace Sigma {
	class CompositeSuscribers {
	public:
		CompositeSuscribers() {};
		virtual ~CompositeSuscribers() {};

		void AddEntity(id_t eid, CompositeID cid) {
			(composite_entity_map[cid])[eid] = true;
		}

		void RemoveEntity(id_t eid, CompositeID cid) {
			(composite_entity_map[cid])[eid] = false;
		}

		bool HasComposite(id_t eid, CompositeID cid) {
			return composite_entity_map.count(cid) && (composite_entity_map.at(cid))[eid];
		}

	private:
		// A map of the id's of entities for each composite
		// the id's are used as key in a bitset
		std::unordered_map<CompositeID, BitArray<uint32_t>> composite_entity_map;
	};
}


#endif // COMPOSITESUSCRIBERS_H_INCLUDED
