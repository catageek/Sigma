#include "composites/RigidBody.h"
#include "composites/PhysicalWorldLocation.h"

namespace Sigma {
	MapArray<btRigidBody*> RigidBody::body_map;
	btDiscreteDynamicsWorld* RigidBody::dynamicsWorld;

	void RigidBody::AddEntity(const id_t id, const std::vector<Property> &properties) {
		btCollisionShape* shape = nullptr;
		btScalar mass = 0;
		std::string shape_type;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;

			if (p->GetName() == "shape") {
				shape_type = p->Get<std::string>();
				break;
			}
			if (p->GetName() == "mass") {
				mass = p->Get<btScalar>();
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
			std::string meshFilename = "";

			for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
				const Property*  p = &*propitr;
				if (p->GetName() == "scale") {
					scale = p->Get<float>();
				}
				else if (p->GetName() == "meshFile") {
					std::cerr << "Loading mesh: " << p->Get<std::string>() << std::endl;
					meshFilename = p->Get<std::string>();
				}
			}

			std::shared_ptr<resource::Mesh> mesh = resource::ResourceSystem::GetInstace()->Get<resource::Mesh>(meshFilename);

			auto btmesh = new btTriangleMesh();
			for (unsigned int i = 0; i < mesh->FaceCount(); ++i) {
				const resource::Face* f = mesh->GetFace(i);
				const resource::Vertex* v1 = mesh->GetVertex(f->v1);
				const resource::Vertex* v2 = mesh->GetVertex(f->v2);
				const resource::Vertex* v3 = mesh->GetVertex(f->v3);
				btmesh->addTriangle(btVector3(v1->x, v1->y, v1->z), btVector3(v2->x, v2->y, v2->z), btVector3(v3->x, v3->y, v3->z));
			}

			shape = new btScaledBvhTriangleMeshShape(new btBvhTriangleMeshShape(btmesh, false), btVector3(scale, scale, scale));

		}

		if (! shape) {
			assert(0 && "Invalid shape type");
		}

		btVector3 fallInertia(0,0,0);
		auto motionState =	PhysicalWorldLocation::GetMotionState(id);
		btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, motionState, shape,fallInertia);
		shape->calculateLocalInertia(mass,fallInertia);
		auto b = new btRigidBody(fallRigidBodyCI);
		b->setContactProcessingThreshold(BT_LARGE_FLOAT);
		b->setCcdMotionThreshold(.5);
		b->setCcdSweptSphereRadius(0);
		body_map[id] = b;
		// add the body to the world
		dynamicsWorld->addRigidBody(body_map.at(id));
	}
}
