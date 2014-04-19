#include "systems/network/Protocol.h"

namespace Sigma {
    template<>
    void FrameObject::Resize<CLIENT>(size_t new_size) {
        packet_size = new_size + sizeof(Frame);
        data.resize(packet_size + VMAC_SIZE);
    };

	template<>
	void FrameObject::Resize<SERVER>(size_t new_size) {
		packet_size = new_size + sizeof(Frame);
		data.resize(packet_size + ESIGN_SIZE);
	}

	template<>
	void FrameObject::Resize<NONE>(size_t new_size) {
		packet_size = new_size + sizeof(Frame);
		data.resize(packet_size);
	}
}