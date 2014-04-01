#include "systems/network/Protocol.h"
#include "Sigma.h"
#include "netport/include/net/network.h"
#include "composites/NetworkNode.h"
#include "systems/network/VMAC_StreamHasher.h"

using namespace network;

namespace Sigma {
	void FrameObject::SendMessage(int fd, unsigned char major, unsigned char minor, cryptography::VMAC_StreamHasher* hasher) {
		if(fd < 0) {
			return;
		}
		Header()->flags |= 1 << HAS_VMAC_TAG;
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = packet_size - sizeof(Frame_hdr) + VMAC_SIZE;
		// The VMAC Hasher has been put under id #1 for client
		hasher->CalculateDigest(VMAC_tag(), reinterpret_cast<const unsigned char*>(data.data()), packet_size);
		LOG_DEBUG << "Bytes to send (with VMAC): " << packet_size + VMAC_SIZE;
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size + VMAC_SIZE) <= 0) {
			LOG_ERROR << "could not send authenticated frame" ;
		};
	}

	void FrameObject::SendMessage(id_t id, unsigned char major, unsigned char minor) {
		auto fd = NetworkNode::GetFileDescriptor(id);
		if(fd < 0) {
			return;
		}
		SendMessageNoVMAC(fd, major, minor);
	}

	void FrameObject::SendMessageNoVMAC(int fd, unsigned char major, unsigned char minor) {
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = packet_size - sizeof(Frame_hdr);

		FullFrame()->length = packet_size - sizeof(Frame_hdr);
		LOG_DEBUG << "Bytes to send (no VMAC): " << packet_size;
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size) <= 0) {
			LOG_ERROR << "could not send frame" ;
		};
	}


	template<>
	void FrameObject::Resize<false>(size_t new_size) {
		packet_size = new_size + sizeof(Frame);
		data.resize(packet_size);
	}

	bool FrameObject::Verify_VMAC_tag() {
		if(Header()->flags & (1 << HAS_VMAC_TAG)) {
			packet_size -= VMAC_SIZE;
			return vmac_verifier->second.Verify(VMAC_tag(), reinterpret_cast<const unsigned char*>(data.data()), packet_size);
		}
		return false;
	}

	id_t FrameObject::GetId() const { return vmac_verifier->first; }

}
