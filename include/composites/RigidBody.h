#ifndef RIGIDBODY_H_INCLUDED
#define RIGIDBODY_H_INCLUDED

#include <vector>

#include <bullet/btBulletDynamicsCommon.h>
#include "IComponent.h"
#include "Property.h"
#include "components/BulletShapeMesh.h"
#include "resources/Mesh.h"
#include "MapArray.hpp"
#include "systems/OpenGLSystem.h"

namespace Sigma {
	/** \brief A component for entities that have a rigid body
	 *
	 * It stores the body instance of each entity
	 *
	 * Entities having this component must also have the PhysicalWorldLocation component
	 *
	 * NB: The btCollisionShape is a btCapsuleShape.
	 */
	class RigidBody {
	public:
		SET_STATIC_COMPONENT_TYPENAME("RigidBody");

		RigidBody() {};

		virtual ~RigidBody() {};

		static void AddEntity(const id_t id, const std::vector<Property> &properties);

		// TODO : read properties
//		static std::vector<std::unique_ptr<IECSComponent>> AddEntity(const id_t id, const std::vector<Property> &properties) { return AddEntity(id); };

		static void RemoveEntity(const id_t id) {
			body_map.clear(id);
		};

		static btRigidBody* getBody(const id_t id) {
			return body_map.at(id);
		};

		static void setWorld(btDiscreteDynamicsWorld* world) {
			dynamicsWorld = world;
		}

	private:
		static MapArray<btRigidBody*> body_map;
		static btDiscreteDynamicsWorld* dynamicsWorld;
	};
}

#endif // RIGIDBODY_H_INCLUDED
