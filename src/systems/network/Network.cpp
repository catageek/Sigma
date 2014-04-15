#include "systems/network/Network.h"
#include "systems/network/NetworkSystem.h"

namespace Sigma {
	template<>
	void NetworkSystem::CloseConnection<true>(const FrameObject* frame) const {
		delete frame->CxData();
		RemoveClientConnection();
	}

	template<>
	void NetworkSystem::CloseConnection<false>(const FrameObject* frame) const {
		RemoveConnection(frame->fd);
		delete frame->CxData();
	}
}
