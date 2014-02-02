#ifndef COMPOSITEDEPENDENCY_H_INCLUDED
#define COMPOSITEDEPENDENCY_H_INCLUDED

namespace Sigma {
	class CompositeDependency {
	public:
		CompositeDependency() {};
		virtual ~CompositeDependency() {};

		void AddDependency(ComponentID child, ComponentID parent) {
			parent_child_map.insert(std::make_pair(parent, child));
		};

		std::multimap<ComponentID, ComponentID>& map() {
			return parent_child_map;
		}

	private:
		std::multimap<ComponentID, ComponentID> parent_child_map;
	};
}

#endif // COMPOSITEDEPENDENCY_H_INCLUDED
