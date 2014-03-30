#include "systems/network/Protocol.h"
#include "Sigma.h"
#include "netport/include/net/network.h"
#include "composites/NetworkNode.h"
#include "composites/VMAC_Checker.h"

using namespace network;

namespace Sigma {
	void FrameObject::SendMessage(int fd, unsigned char major, unsigned char minor) {
		if(fd < 0) {
			return;
		}
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = packet_size - sizeof(Frame_hdr);

		if(Header()->flags & (1 << HAS_VMAC_TAG)) {
			FullFrame()->length = packet_size - sizeof(Frame_hdr) + VMAC_SIZE;
			// The VMAC Hasher has been put under id #1
			VMAC_Checker::Digest(1, VMAC_tag(), reinterpret_cast<const unsigned char*>(data.data()), packet_size);
			LOG_DEBUG << "Bytes to send (with VMAC): " << packet_size + VMAC_SIZE;
			if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size + VMAC_SIZE) <= 0) {
				LOG_ERROR << "could not send authenticated frame" ;
			};
		}
		else {
			FullFrame()->length = packet_size - sizeof(Frame_hdr);
			LOG_DEBUG << "Bytes to send (no VMAC): " << packet_size;
			if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size) <= 0) {
				LOG_ERROR << "could not send frame" ;
			};
		}

	}

	void FrameObject::SendMessage(id_t id, unsigned char major, unsigned char minor) {
		SendMessage(NetworkNode::GetFileDescriptor(id), major, minor);
	}

	template<>
	void FrameObject::Resize<false>(size_t new_size) {
		packet_size = new_size + sizeof(Frame);
		data.resize(packet_size);
	}

	void FrameObject::Set_VMAC_Flag() {
		Header()->flags |= 1 << HAS_VMAC_TAG;
	}

	bool FrameObject::Verify_VMAC_tag(const id_t id) {
		if(Header()->flags & (1 << HAS_VMAC_TAG)) {
			packet_size -= VMAC_SIZE;
			return VMAC_Checker::Verify(id, VMAC_tag(), reinterpret_cast<const unsigned char*>(data.data()), packet_size);
		}
		return false;
	}
}
