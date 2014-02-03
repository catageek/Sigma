#include "systems/BulletPhysics.h"
#include "components/BulletShapeMesh.h"
#include "resources/Mesh.h"
#include "components/BulletShapeSphere.h"
#include "composites/PhysicalWorldLocation.h"

namespace Sigma {
	// We need ctor and dstor to be exported to a dll even if they don't do anything
	BulletPhysics::BulletPhysics() {}
	BulletPhysics::~BulletPhysics() {
		if (this->mover != nullptr) {
			delete this->mover;
		}
		if (this->dynamicsWorld != nullptr) {
			delete this->dynamicsWorld;
		}
		if (this->solver != nullptr) {
			delete this->solver;
		}
		if (this->dispatcher != nullptr) {
			delete this->dispatcher;
		}
		if (this->collisionConfiguration != nullptr) {
			delete this->collisionConfiguration;
		}
		if (this->broadphase != nullptr) {
			delete this->broadphase;
		}
	}

	bool BulletPhysics::Start() {
		this->broadphase = new btDbvtBroadphase();
		this->collisionConfiguration = new btDefaultCollisionConfiguration();
		this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
		this->solver = new btSequentialImpulseConstraintSolver();
		this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher, this->broadphase, this->solver, this->collisionConfiguration);
		this->dynamicsWorld->setGravity(btVector3(0,-10,0));
		RigidBody::setWorld(dynamicsWorld);
		return true;
	}

	void BulletPhysics::initViewMover(GLTransform& t) {
		this->moverSphere = new BulletShapeCapsule(1);
		this->moverSphere->SetCapsuleSize(0.3f, 1.3f);
		this->moverSphere->InitializeRigidBody(
			t.GetPosition().x,
			t.GetPosition().y,
			t.GetPosition().z,
			t.GetPitch(),
			t.GetYaw(),
			t.GetRoll());
		this->mover = new PhysicsController(*moverSphere, t);
		this->dynamicsWorld->addRigidBody(this->moverSphere->GetRigidBody());
	}

	std::map<std::string,Sigma::IFactory::FactoryFunction>
    BulletPhysics::getFactoryFunctions() {
		using namespace std::placeholders;
		std::map<std::string,Sigma::IFactory::FactoryFunction> retval;
		return retval;
	}

	std::map<std::string,Sigma::IECSFactory::FactoryFunction>
    BulletPhysics::getECSFactoryFunctions() {
		using namespace std::placeholders;
		std::map<std::string,Sigma::IECSFactory::FactoryFunction> retval;
		retval[ControllableMove::getComponentTypeName()] = std::bind(&ControllableMove::AddEntity,_1);
		retval[InterpolatedMovement::getComponentTypeName()] = std::bind(&InterpolatedMovement::AddEntity,_1);
		retval[PhysicalWorldLocation::getComponentTypeName()] = std::bind(&PhysicalWorldLocation::AddEntity,_1,_2);
		retval[RigidBody::getComponentTypeName()] = std::bind(&RigidBody::AddEntity,_1,_2);
		return retval;
	}

	bool BulletPhysics::Update(const double delta) {
		this->mover->UpdateForces(delta);
		// Make entities with target move a little
		InterpolatedMovement::ComputeInterpolatedForces(delta);
		// It's time to sum all the forces
		ControllableMove::CumulateForces();
		// We inject the movement in the simulation or directly
		ControllableMove::ApplyForces(delta);
		// We step the simulation
		dynamicsWorld->stepSimulation(delta, 10);
		// We update the transform component with updated data of the PhysicalWorldLocation component
		PhysicalWorldLocation::UpdateTransform();
		PhysicalWorldLocation::ClearUpdatedSet();
		this->mover->UpdateTransform();

		return true;
	}
}
