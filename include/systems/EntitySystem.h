#ifndef ENTITYSYSTEM_H_INCLUDED
#define ENTITYSYSTEM__H_INCLUDED

#include <vector>
#include <memory>
#include "Sigma.h"
#include "Property.h"
#include "IECSComponent.h"
#include "systems/FactorySystem.h"

namespace Sigma {
	class FactorySystem;
	class EntitySystem;

	/** \brief An interface for entities
     */
	class IEntity {
	public:
		IEntity(const id_t entityID) : entityID(entityID) {};
		virtual ~IEntity() {};

		const id_t GetEntityID() const { return entityID; };

		static EntitySystem* entitySystem;

	protected:
		const id_t entityID;
	};


    /** \brief A system that manage the components of entities
     */
	class EntitySystem {
		typedef std::pair<ComponentID, std::unique_ptr<IECSComponent>> element_t;
	public:
		DLL_EXPORT EntitySystem(FactorySystem* factory);
		virtual ~EntitySystem() {};

        /** \brief Add a composite, i.e a set of predefined components, to an entity
         *
         * \param e IEntity* the entity
         * \param fid const CompositeID& the composite ID
         * \param properties const Property& the property object to initialize the composite
         *
         */
		void addComposite(IEntity* e, const CompositeID& fid, const std::vector<Property>& properties);

		void RemoveComposite(IEntity *e, const CompositeID& cid);
			// TODO : add method to remove component in factory ?
			//componentFactory.remove(FeatureID, e->GetEntityID());
			// get a list of components to remove in entity map
	private:
		FactorySystem* componentFactory;
	};
}

#endif // ENTITYSYSTEM_H_INCLUDED
