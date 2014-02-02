#ifndef COMPOSITESUBSCRIBERS_H_INCLUDED
#define COMPOSITESUBSCRIBERS_H_INCLUDED

#include <unordered_map>
#include "BitArray.hpp"
#include "IFactory.h"
#include "IECSFactory.h"
#include "systems/FactorySystem.h"

namespace Sigma {
	struct EntityProperty {
		EntityProperty(id_t id, const std::vector<Property>& properties) : id(id), properties(properties) {};
		~EntityProperty() {};
		id_t id;
		std::vector<Property> properties;
	};

	class CompositeSubscribers {
	public:
		CompositeSubscribers() {};
		virtual ~CompositeSubscribers() {};

        /** \brief Add a composite removal to the queue
         *
         * \param eid id_t the entity id
         * \param cid CompositeID the composite to remove
         *
         */
		static void QueueAddEntity(id_t eid, CompositeID cid, const std::vector<Property>& properties) {
			composite_entity_add_map.emplace(std::make_pair(CompositeID(cid), EntityProperty(eid, properties)));
		}

        /** \brief Add a composite addition to the queue
         *
         * \param eid id_t the entity id
         * \param cid CompositeID the composite to add
         *
         */
		static void QueueRemoveEntity(id_t eid, CompositeID cid) {
			(composite_entity_rmqueue_map[cid])[eid] = true;
		}

        /** \brief Tell if an entity has a composite
         *
         * \param eid id_t the entity id
         * \param cid CompositeID the composite id
         * \return bool true if the entity has the composite
         *
         */
		static bool HasComposite(id_t eid, CompositeID cid) {
			return composite_entity_map.count(cid) && (composite_entity_map.at(cid))[eid];
		}

        /** \brief Return the BitArray of the entities having a specific composite
         *
         * \param cid CompositeID the composite id
         * \return BitArray<uint32_t>& the BitArray returned
         *
         */
		static BitArray<uint32_t>& GetBitArray(CompositeID cid) {
			return composite_entity_map.at(cid);
		}

        /** \brief Register a factory
         *
         * \param factory IFactory& the factory to register
         *
         */
		static void register_Factory(IFactory& f) {
			factory.register_Factory(f);
		}

        /** \brief Register an ECS factory
         *
         * \param factory IFactory& the factory to register
         *
         */
		static void register_ECSFactory(IECSFactory& f) {
			factory.register_ECSFactory(f);
		}

		static FactorySystem& GetFactory() {
			return factory;
		}


        /** \brief Process the removal queue. Actually remove the composites
         *
         */
		static void ProcessRemoval() {
			// remove composites
			for (auto itc = composite_entity_rmqueue_map.begin(), endc = composite_entity_rmqueue_map.end(); itc != endc; ++itc) {
				for (auto ite = itc->second.begin(), endc = itc->second.end(); ite != endc; ++ite) {
					// TODO: call delete function of composite
					(composite_entity_map[itc->first])[*ite] = false;
				}
			}
			composite_entity_rmqueue_map.clear();
		}

        /** \brief Process the addition queue. Actually add the composites
         *
         */
		static void ProcessAddition() {
			// add composites
			for (auto it = composite_entity_add_map.cbegin(), end = composite_entity_add_map.cend(); it != end; ++it) {
				if (! factory.create(it->first, it->second.id, it->second.properties)) {
					factory.createECS(it->first, it->second.id, it->second.properties);
				};
				(composite_entity_map[it->first])[it->second.id] = true;
			}
			composite_entity_add_map.clear();
		}

	private:
		// A map of the id's of entities for each composite
		// the id's are used as key in a bitset
		static std::unordered_map<CompositeID, BitArray<uint32_t>> composite_entity_map;
		// The map used as queue before actually remove a composite from an entity
		static std::unordered_map<CompositeID, BitArray<uint32_t>> composite_entity_rmqueue_map;
		// the map of properties queued to add
		static std::multimap<CompositeID, EntityProperty> composite_entity_add_map;
		// the reference to a factory
		static FactorySystem& factory;

	};
}


#endif // COMPOSITESUBSCRIBERS_H_INCLUDED
