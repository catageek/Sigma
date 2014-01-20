#ifndef COMPOSITESYSTEM_H_INCLUDED
#define COMPOSITESYSTEM_H_INCLUDED

#include <unordered_map>
#include "BitArray.hpp"
#include "systems/CompositeSuscribers.h"
#include "systems/CompositeDependency.h"
#include "systems/FactorySystem.h"

namespace Sigma {
	class CompositeSystem {
	public:
		DLL_EXPORT CompositeSystem() : factory(FactorySystem::getInstance(this)) {};
		DLL_EXPORT virtual ~CompositeSystem() {};

		DLL_EXPORT void AddEntity(CompositeID cid, id_t eid, const std::vector<Property> &properties) {
			if (! factory.create(cid, eid, properties)) {
				factory.createECS(cid, eid, properties);
			};
			suscribers.AddEntity(eid, cid);
		}

		DLL_EXPORT void RemoveEntity(id_t eid, CompositeID cid) {
			suscribers.RemoveEntity(eid, cid);
		}

		DLL_EXPORT bool HasComposite(id_t eid, CompositeID cid) {
			return suscribers.HasComposite(eid, cid);
		}

		DLL_EXPORT FactorySystem* GetFactory() {
			return &factory;
		}

	private:
		// The container for entity ID's that suscribe to each component
		CompositeSuscribers suscribers;
		CompositeDependency dependencies;
		FactorySystem& factory;
	};
}

#endif // COMPOSITESYSTEM_H_INCLUDED
