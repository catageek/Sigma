#include "systems/network/Network.h"
#include "systems/network/NetworkSystem.h"

namespace Sigma {
	template<>
	void NetworkSystem::CloseConnection<true>(const FrameObject* frame) {
		delete frame->CxData();
		CloseClientConnection();
	}

	template<>
	void NetworkSystem::CloseConnection<false>(const FrameObject* frame) {
		CloseConnection(frame->fd);
		delete frame->CxData();
	}
}
