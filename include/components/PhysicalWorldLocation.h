#ifndef PHYSICALWORLDLOCATION_H_INCLUDED
#define PHYSICALWORLDLOCATION_H_INCLUDED

#include "BitArray.hpp"
#include "components/WorldPosition.h"
#include "components/WorldOrientation.h"
#include "components/SigmaMotionState.h"
#include "SharedPointerMap.hpp"
#include "IComponent.h"
#include <map>
#include <memory>

namespace Sigma {
	/** \brief A component to store the location of an entity
	 *
	 * It provides a btMotionState instance for a specific entity upon request, to use the component
	 * as a storage for the Bullet simulation.
	 */
	class PhysicalWorldLocation : IComponent {
	public:
		SET_COMPONENT_TYPENAME("PhysicalWorldLocation");

		PhysicalWorldLocation() {};

		virtual ~PhysicalWorldLocation() {};

		static void AddEntity(const id_t id, const coordinate_type x, const coordinate_type y,
					   const coordinate_type z, const coordinate_type rx, const coordinate_type ry, const coordinate_type rz);

		static void RemoveEntity(const id_t id) {
			pphysical.RemoveEntityPosition(id);
			ophysical.RemoveEntityOrientation(id);
		};

		static std::unique_ptr<position_type> getPosition(const id_t id) {
			return std::unique_ptr<position_type>(new position_type(pphysical.x(id), pphysical.y(id), pphysical.z(id)));
		};

		/** \brief Helper function to set an entity position
		 *
		 * \param id const id_t the entity id
		 * \param position const position_type& the position to set
		 *
		 */
		static void setPosition(const id_t id, const position_type& position);

		static std::unique_ptr<orientation_type> getOrientation(const id_t id) {
			return std::unique_ptr<orientation_type>(new orientation_type(ophysical.euler(id)));
		};

		static inline SigmaMotionState* GetMotionState(const id_t id) {
			return new SigmaMotionState(id, pphysical, ophysical);
		};

		/** \brief Fore the position of an entity to be marked as updated
		 *
		 * Note that you do not need to call this function each time you update a position,
		 * this is already done internally.
		 *
		 * \param id const id_t the entity
		 *
		 */
		static inline void MarkUpdated(const id_t id) { (*updated_set)[id] = true; };

		/** \brief Clear the set of updated entities
		 *
		 * \return ClearUpdatedSet() { updated_set = BitArray<unsigned
		 *
		 */
		static inline void ClearUpdatedSet() { updated_set = BitArray<unsigned int>::Create(); };

		static inline std::unique_ptr<BitArrayIterator<unsigned int>> GetIteratorUpdatedID() {
			return updated_set->iterator();
		};

		static inline id_t GetUpdatedSize() { return updated_set->size(); }

		// TODO : move to UserView component
		/** \brief A function adding a viewer, i.e an entity that will render the world
		 *
		 * \param id const id_t the id of the entity
		 * \param view_limit coordinate_type the distance view for this viewer
		 *
		 */
		void AddViewer(const id_t id, coordinate_type view_limit) {
			viewDistanceMap.insert({{id, view_limit}});
			viewBitsetMap.insert({{id, BitArray<unsigned short>::Create()}});
		};

		// TODO : move to UserView component
		/** \brief A function providing all the cartesian positions with a translation
		 * and a conversion in single precision
		 *
		 * This function may be used to bootstrap the world for a new logged in player
		 *
		 * This supposes that the player run the bullet simulation in single precision
		 *
		 * \param id const id_t the id of the entity that will be the origin
		 * \return std::unique_ptr<position_array> the entity positions translated
		 *
		 */
		inline std::unique_ptr<position_array> TransformedPositions(const id_t id) {
			return pphysical.RelativeTo(pphysical.x(id), pphysical.y(id), pphysical.z(id));
		};

		/** \brief Get a bitmap of the entities that are in a cube
		 *
		 * The center of the cube is the entity provided, and the sides have a length
		 * of 2 * the distance view
		 *
		 * \param id const id_t the id of the entity at the center
		 * \return std::shared_ptr<BitArray<unsigned short>> the bitset
		 *
		 */
		inline std::shared_ptr<BitArray<unsigned short>> InViewPositions(const id_t id) {
			auto distance = viewDistanceMap.find(id);
			if (distance != viewDistanceMap.end()) {
				return pphysical.InViewPositions(pphysical.x(id), pphysical.y(id), pphysical.z(id), distance->second);
			}
		};

	private:
		static WorldPosition pphysical;
		static WorldOrientation ophysical;
		static std::shared_ptr<BitArray<unsigned int>> updated_set;

		// TODO : move to UserViewSystem
		std::map<id_t, std::shared_ptr<BitArray<unsigned short>>> viewBitsetMap;
		std::map<id_t, coordinate_type> viewDistanceMap;
	};
}
#endif // PHYSICALWORLDLOCATION_H_INCLUDED
