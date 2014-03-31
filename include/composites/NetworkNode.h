#ifndef NETWORKNODE_H_INCLUDED
#define NETWORKNODE_H_INCLUDED

#include <unordered_map>
#include "VectorMap.hpp"
#include "netport/include/net/network.h"
#include "IECSComponent.h"

namespace Sigma {
	class NetworkNode {
	public:
		NetworkNode() {};
		virtual ~NetworkNode() {};

		static std::vector<std::unique_ptr<IECSComponent>> AddEntity(const id_t id, network::TCPConnection&& cnx) {
			if(! connection_map.count(id)) {
				fd_map[id] = cnx.Handle();
				connection_map[id] = std::move(cnx);
			}
			return std::vector<std::unique_ptr<IECSComponent>>();
		}

		static void RemoveEntity(const id_t id) {
			if(connection_map.count(id)) {
				connection_map.at(id).Close();
				fd_map.clear(id);
				connection_map.erase(id);
			}
		}

		static int GetFileDescriptor(const id_t id) {
			if(fd_map.count(id)) {
				return fd_map.at(id);
			}
			return -1;
		}

	private:
		static std::unordered_map<id_t, network::TCPConnection> connection_map;
		static VectorMap<id_t, int> fd_map;
	};
}


#endif // NETWORKNODE_H_INCLUDED
