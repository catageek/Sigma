#include "composites/NetworkNode.h"

namespace Sigma {
	std::unordered_map<id_t, network::TCPConnection> NetworkNode::connection_map;
	VectorMap<id_t, int> NetworkNode::fd_map;
}
