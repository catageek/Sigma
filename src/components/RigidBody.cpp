#include "components/RigidBody.h"
#include "components/PhysicalWorldLocation.h"

namespace Sigma {
	MapArray<btRigidBody*> RigidBody::body_map;
	btDiscreteDynamicsWorld* RigidBody::dynamicsWorld;

	void RigidBody::AddEntity(const id_t id, const std::vector<Property> &properties) {
		btCollisionShape* shape = nullptr;
		std::string shape_type;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;

			if (p->GetName() == "shape") {
				shape_type = p->Get<std::string>();
				break;
			}
		}

		if (shape_type == "capsule") {
			float radius = 0.0f;
			float height = 0.0f;
			for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
				const Property  p = *propitr;
				if (p.GetName() == "radius") {
					radius = p.Get<float>();
				}
				else if (p.GetName() == "height") {
					height = p.Get<float>();
				}
			}
			shape = new btCapsuleShape(radius, height);
		}
		else if (shape_type == "sphere") {
			float radius = 0.0f;
			for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
				const Property*  p = &*propitr;
				if (p->GetName() == "radius") {
					radius = p->Get<float>();
				}
			}
			shape = new btSphereShape(radius);
		}
		else if (shape_type == "mesh") {
			float scale = 1.0f;
			Mesh glmesh;

			for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
				const Property*  p = &*propitr;
				if (p->GetName() == "scale") {
					scale = p->Get<float>();
				}
				else if (p->GetName() == "meshFile") {
					std::cerr << "Loading mesh: " << p->Get<std::string>() << std::endl;
					glmesh.LoadObjMesh(p->Get<std::string>());
				}
			}

			auto btmesh = new btTriangleMesh();
			for (unsigned int i = 0; i < glmesh.FaceCount(); ++i) {
				const Sigma::Face* f = glmesh.GetFace(i);
				const Sigma::Vertex* v1 = glmesh.GetVertex(f->v1);
				const Sigma::Vertex* v2 = glmesh.GetVertex(f->v2);
				const Sigma::Vertex* v3 = glmesh.GetVertex(f->v3);
				btmesh->addTriangle(btVector3(v1->x, v1->y, v1->z), btVector3(v2->x, v2->y, v2->z), btVector3(v3->x, v3->y, v3->z));
			}

			shape = new btScaledBvhTriangleMeshShape(new btBvhTriangleMeshShape(btmesh, false), btVector3(scale, scale, scale));

		}

		if (! shape) {
			assert(0 && "Invalid shape type");
		}
		btScalar mass = 1;
		btVector3 fallInertia(0,0,0);
		auto motionState =	PhysicalWorldLocation::GetMotionState(id);
		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, motionState, shape,fallInertia);
		shape->calculateLocalInertia(mass,fallInertia);
		body_map[id] = new btRigidBody(fallRigidBodyCI);
		// add the body to the world
		dynamicsWorld->addRigidBody(body_map.at(id));
	}
}
