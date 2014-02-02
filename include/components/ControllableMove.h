#ifndef CONTROLLABLEMOVE_H_INCLUDED
#define CONTROLLABLEMOVE_H_INCLUDED

#include <list>
#include <unordered_map>
#include <memory>
#include <vector>

#include "glm/glm.hpp"
#include "Sigma.h"
#include "Property.h"
#include "IECSComponent.h"
#include "IComponent.h"
#include "GLTransform.h"
#include "MapArray.hpp"
#include "systems/CompositeSubscribers.h"

namespace Sigma {
	typedef std::list<glm::vec3> forces_list; // The list of forces for each entity
	typedef std::list<glm::vec3> rotationForces_list;

	/** \brief A component for entities with movements under external control
	 *
	 * It stores a list of forces to apply.
	 *
	 * A function provides a way to apply the forces to entities
	 *
	 * NB: Currently movements are constrained in the horizontal plan
	 */
	class ControllableMove {
	public:
		SET_STATIC_COMPONENT_TYPENAME("ControllableMove");

		/* All functions are static for convenience purpose (be accesssible from everywhere).
		 * A component must be able to be pure static except special cases.
		 * Making the component pure static is a good practice (I think...)
		 */

		virtual ~ControllableMove() {}; // default dtor
	private:
		ControllableMove(); // default ctor
		ControllableMove(ControllableMove& cm); // copy constructor
		ControllableMove(ControllableMove&& cm); // move constructor
		ControllableMove& operator=(ControllableMove& cm); // copy assignment
		ControllableMove& operator=(ControllableMove&& cm); // move assignment
	public:
		// Note that all data have the same lifecycle : created at once, deleted at once
		// If this is not the case, split the component
		static void AddEntity(const id_t id) {
			// TODO: check that the composite does not exist for this entity
			forces_map[id] = forces_list();
			rotationForces_map.emplace(id, rotationForces_list());
			cumulatedForces_map[id] = glm::vec3();
		}

		static void RemoveEntity(const id_t id) {
			forces_map.clear(id);
			rotationForces_map.erase(id);
			cumulatedForces_map.clear(id);
		}

		/** \brief Add a force to the list for a specific entity.
		 *
		 * Adds the force to the list. Adding of a duplicate force is checked for and ignored.
		 * \param id const id_t the id of the entity
		 * \param force glm::vec3 The force to add to the list.
		 *
		 */
		static void AddForce(const id_t id, glm::vec3 force) {
			auto forces = getForces(id);
			for (auto forceitr = forces.begin(); forceitr != forces.end(); ++forceitr) {
				if ((*forceitr) == force) {
					return;
				}
			}
			forces.push_back(force);
		}

		/**
		 * \brief Removes a force from the list for a specific id.
		 *
		 * \param id const id_t the id of the entity
		 * \param glm::vec3 force The force to remove. Is is evaluated based on glm's comparisson.
		 */
		static void RemoveForce(const id_t id, const glm::vec3& force) {
			auto forces = getForces(id);
			forces.remove(force);
		}

		static void AddRotationForce(const id_t id, const glm::vec3 rotation) {
			auto rotationForces = getRotationForces(id);
			for (auto rotitr = rotationForces->begin(); rotitr != rotationForces->end(); ++rotitr) {
				if ((*rotitr) == rotation) {
					return;
				}
			}
			rotationForces->push_back(rotation);
		}

		static void RemoveRotationForce(const id_t id, glm::vec3 rotation) {
			auto rotationForces = getRotationForces(id);
			rotationForces->remove(rotation);
		}

		/**
		 * \brief Removes all forces from the list.
		 *
		 * \param id const id_t the id of the entity
		 */
		static void ClearForces(const id_t id) {
			auto forces = getForces(id);
			forces.clear();

			auto rotationForces = getRotationForces(id);
			rotationForces->clear();
		}

		/**
		 * \brief Apply all forces to the body of all entities that have this component.
		 *
		 * Sets the rigid body's linear force.
		 *
		 * NB: You must call CumulateForces() before to refresh forces
		 */
		static void ApplyForces(const double delta);

		/** \brief Compute the cumulated forces for all entities that have this component
		 *
		 * The cumulated force is stored in a map to be retrieved in order to be applied
		 * to a body, or to be broadcasted on the network
		 *
		 */
		static void CumulateForces();

		static forces_list& getForces(const id_t id) {
			return forces_map.at(id);
		}

		static rotationForces_list* getRotationForces(const id_t id) {
			auto rotationForces = rotationForces_map.find(id);
			if (rotationForces != rotationForces_map.end()) {
				return &rotationForces->second;
			}
			return nullptr;
		}

	private:
		static MapArray<forces_list> forces_map; // The list of forces to apply each update loop.
		static std::unordered_map<id_t, rotationForces_list> rotationForces_map;
		static MapArray<glm::vec3> cumulatedForces_map;
	};
}

#endif // CONTROLLABLEMOVE_H_INCLUDED
