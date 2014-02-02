#include "systems/EntitySystem.h"

namespace Sigma {
	// IEntity implementation
	EntitySystem* IEntity::entitySystem = 0;

	EntitySystem::EntitySystem(FactorySystem* factory) : componentFactory(factory) { IEntity::entitySystem = this; }

	void EntitySystem::addComposite(IEntity* e, const CompositeID& fid, const std::vector<Property>& properties) {
		// create feature in ECS
		componentFactory->createECS(fid, e->GetEntityID(), properties);
	}

	void EntitySystem::RemoveComposite(IEntity *e, const CompositeID& cid) {
		// TODO : add method to remove component in factory ?
		//componentFactory.remove(FeatureID, e->GetEntityID());
		// get a list of components to remove in entity map
	}
}
