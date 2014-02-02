#include "systems/CompositeSubscribers.h"

namespace Sigma {
		std::unordered_map<CompositeID, BitArray<uint32_t>> CompositeSubscribers::composite_entity_map;
		std::unordered_map<CompositeID, BitArray<uint32_t>> CompositeSubscribers::composite_entity_rmqueue_map;
		std::multimap<CompositeID, EntityProperty> CompositeSubscribers::composite_entity_add_map;
		FactorySystem& CompositeSubscribers::factory = FactorySystem::getInstance();
}
