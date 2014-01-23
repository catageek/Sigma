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
		DLL_EXPORT CompositeSystem() {};
		DLL_EXPORT virtual ~CompositeSystem() {};

        /** \brief Add a composite to an entity
         *
         * The addition order will be actually put in a queue
         *
         * \param cid CompositeID the composite id
         * \param eid id_t the entity id
         * \param properties const std::vector<Property>& the properties to initialize the composite
         *
         */
		DLL_EXPORT void AddEntity(CompositeID cid, id_t eid, const std::vector<Property> &properties) {
			suscribers.QueueAddEntity(eid, cid, properties);
		}

        /** \brief Remove a composite to an entity
         *
         * The removal order will be actually put in a queue
         *
         * \param cid CompositeID the composite id
         * \param eid id_t the entity id
         *
         */
		DLL_EXPORT void RemoveEntity(id_t eid, CompositeID cid) {
			suscribers.QueueRemoveEntity(eid, cid);
		}

        /** \brief Tell if an entity has a composite
         *
         * \param eid id_t the entity id
         * \param cid CompositeID the composite id
         * \return bool true if the entity has the composite
         *
         */
		bool HasComposite(id_t eid, CompositeID cid) {
			return suscribers.HasComposite(eid, cid);
		}

        /** \brief Declare a dependency for a composite
         *
         * \param child ComponentID the id of the child composite
         * \param parent ComponentID the id of the parent composite
         *
         */
		void AddDependency(ComponentID child, ComponentID parent) {
			dependencies.AddDependency(child, parent);
		};

        /** \brief Return the instance of the factory
         *
         * \return FactorySystem& the instance
         *
         */
		DLL_EXPORT FactorySystem& GetFactory() {
			return suscribers.GetFactory();
		}

        /** \brief Process the orders in the queues and remove orphans
         *
         * First removal orders are processed, then the composites with
         * no parents are removed, and finally the addition queue is processed
         *
         */
		DLL_EXPORT void Update() {
			suscribers.ProcessRemoval();
			RemoveOrphans();
			suscribers.ProcessAddition();
		}

	private:
        /** \brief Remove the orphans composites
         *
         */
		void RemoveOrphans() {
			auto dmap = dependencies.map();
			bool not_finished = true;
			while (not_finished) {
				// we only iteratate once (if we can)
				not_finished = false;
				for(auto itp = dmap.begin(), end_p = dmap.end(); itp != end_p; itp = dmap.upper_bound(itp->first)) {
					// for each unique key of parent
					auto parent = suscribers.GetBitArray(itp->first);
					// get all children
					auto range = dependencies.map().equal_range(itp->first);
					for(auto itc = range.first; itc != range.second; ++itc) {
						// for each child
						auto child = suscribers.GetBitArray(itc->second);
						// looking for entities having child and not parent
						auto result = child & (~parent);
						for (auto ite = result.begin(), end_e = result.end(); ite != end_e; ++ite) {
							// for each entity, remove the child component
							RemoveEntity(*ite, itc->second);
							// Request another iteration
							not_finished = true;
						}
					}
				}
			}
		};

		// The container for entity ID's that suscribe to each component
		CompositeSuscribers suscribers;
		CompositeDependency dependencies;
	};
}

#endif // COMPOSITESYSTEM_H_INCLUDED
