#ifndef COMPOSITESYSTEM_H_INCLUDED
#define COMPOSITESYSTEM_H_INCLUDED

#include <unordered_map>
#include "BitArray.hpp"
#include "systems/CompositeSuscribers.h"
#include "systems/CompositeDependency.h"

namespace Sigma {
	class CompositeSystem {
	public:
		DLL_EXPORT CompositeSystem() {};
		DLL_EXPORT virtual ~CompositeSystem() {};

		DLL_EXPORT void AddEntity(id_t eid, CompositeID cid) {
			suscribers.AddEntity(eid, cid);
		}

		DLL_EXPORT void RemoveEntity(id_t eid, CompositeID cid) {
			suscribers.RemoveEntity(eid, cid);
		}

		DLL_EXPORT bool HasComposite(id_t eid, CompositeID cid) {
			return suscribers.HasComposite(eid, cid);
		}

	private:
		// The container for entity ID's that suscribe to each component
		CompositeSuscribers suscribers;
		CompositeDependency dependencies;
	};
}

#endif // COMPOSITESYSTEM_H_INCLUDED
