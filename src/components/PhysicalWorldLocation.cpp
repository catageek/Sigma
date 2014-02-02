#include "components/PhysicalWorldLocation.h"
#include "components/SigmaMotionState.h"

namespace Sigma {
	MapArray<position_type> PhysicalWorldLocation::pphysical;
	MapArray<orientation_type> PhysicalWorldLocation::ophysical;
	std::unique_ptr<BitArray<unsigned int>> PhysicalWorldLocation::updated_set = std::unique_ptr<BitArray<unsigned int>>(new BitArray<unsigned int>());
    std::unordered_map<id_t, GLTransform> PhysicalWorldLocation::transform_map;
    std::unordered_map<id_t, std::shared_ptr<GLTransform>> PhysicalWorldLocation::transform_ptr_map;

    void PhysicalWorldLocation::UpdateTransform() {
        for (auto it = begin_UpdatedID(); it != end_UpdatedID(); ++it) {
            auto position = getPosition(*it);
            auto transform = GetTransform(*it);
            if (transform) {
				transform->TranslateTo(position.x, position.y, position.z);
            }
        }
    }

	void PhysicalWorldLocation::AddEntityPosition(const id_t id, coordinate_type x, coordinate_type y,
				   coordinate_type z, const coordinate_type rx, const coordinate_type ry, const coordinate_type rz) {
		pphysical[id] = position_type(x, y, z);
		ophysical[id] = orientation_type(rx, ry, rz);
		GLTransform transform;
		// Set the view mover's view pointer.
		transform.SetEuler(true);
		transform.SetMaxRotation(glm::vec3(45.0f,0,0));
		transform_map.insert(std::make_pair(id, transform));
		transform_ptr_map.emplace(id, std::shared_ptr<GLTransform>(&transform_map.at(id)));
		(*updated_set)[id] = true;
		std::cout << "adding entity " << id << " (" << x << ", " << y << ", " << z << ")" << std::endl;
	}

	void PhysicalWorldLocation::AddEntity(const id_t id, const std::vector<Property> &properties) {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;

		for (auto propitr = properties.begin(); propitr != properties.end(); ++propitr) {
			const Property*  p = &*propitr;

			if (p->GetName() == "x") {
				x = p->Get<float>();
			}
			else if (p->GetName() == "y") {
				y = p->Get<float>();
			}
			else if (p->GetName() == "z") {
				z = p->Get<float>();
			}
			else if (p->GetName() == "rx") {
				rx = p->Get<float>();
			}
			else if (p->GetName() == "ry") {
				ry = p->Get<float>();
			}
			else if (p->GetName() == "rz") {
				rz = p->Get<float>();
			}
		}
		AddEntityPosition(id, x, y, z, rx, ry, rz);
	}
}
