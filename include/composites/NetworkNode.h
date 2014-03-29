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
				auto handle = cnx.Handle();
				fd_map[id] = handle;
				fd2id_map[handle] = id;
				connection_map[id] = std::move(cnx);
			}
			return std::vector<std::unique_ptr<IECSComponent>>();
		}

		static void RemoveEntity(const id_t id) {
			if(connection_map.count(id)) {
				connection_map.at(id).Close();
				fd2id_map.erase(fd_map.at(id));
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

		static id_t getEntityID(const int fd) {
			if(fd2id_map.count(fd)) {
				return fd2id_map.at(fd);
			}
			return 0;
		}

	private:
		static std::unordered_map<id_t, network::TCPConnection> connection_map;
		static std::unordered_map<int, id_t> fd2id_map;
		static VectorMap<id_t, int> fd_map;
	};
}


#endif // NETWORKNODE_H_INCLUDED
