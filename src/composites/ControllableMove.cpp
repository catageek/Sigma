#include "composites/ControllableMove.h"
#include "composites/PhysicalWorldLocation.h"
#include "composites/RigidBody.h"

namespace Sigma {
    MapArray<forces_list> ControllableMove::forces_map;
    std::unordered_map<id_t, rotationForces_list> ControllableMove::rotationForces_map;
    MapArray<glm::vec3> ControllableMove::cumulatedForces_map;

    void ControllableMove::CumulateForces() {
        // TODO: vectorize this
        auto cf_it = cumulatedForces_map.begin();
        auto f_it = forces_map.begin();
        for (; f_it != forces_map.end(); ++cf_it, ++f_it) {
			auto id = cf_it->first;
            auto transform = PhysicalWorldLocation::GetTransform(id);
            if (transform != nullptr) {
                glm::vec3 t;
                for (auto forceitr = f_it->second.begin(); forceitr != f_it->second.end(); ++forceitr) {
                    t += *forceitr;
                }
                cf_it->second = (t.z * transform->GetForward()) +
                           (t.y * transform->GetUp()) +
                           (t.x * transform->GetRight());
            }
        }
    }

	// TODO: Optimize the creation of the btVector3
    void ControllableMove::ApplyForces(const double delta) {
        for (auto cf_it = cumulatedForces_map.begin(); cf_it != cumulatedForces_map.end(); ++cf_it) {
			auto id = cf_it->first;
            auto body = RigidBody::getBody(id);
            auto finalForce = cf_it->second;
            if (CompositeSubscribers::HasComposite(id, "RigidBody")) {
                body->setLinearVelocity(btVector3(finalForce.x, body->getLinearVelocity().y() + 0.000000001f, finalForce.z));
            }
            else {
				auto position = PhysicalWorldLocation::getPosition(id);
				// TODO: check that the position is valid
				position.x += finalForce.x * delta;
				position.z += finalForce.z * delta;
				PhysicalWorldLocation::setPosition(id, position);
            }
        }
    }

}
