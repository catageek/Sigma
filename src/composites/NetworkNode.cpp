#include "composites/NetworkNode.h"

namespace Sigma {
	std::unordered_map<id_t, network::TCPConnection> NetworkNode::connection_map;
	std::unordered_map<int, id_t> NetworkNode::fd2id_map;
	VectorMap<id_t, int> NetworkNode::fd_map;
}
