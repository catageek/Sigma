#include "systems/network/Protocol.h"

#include "composites/NetworkNode.h"
#include "systems/network/NetworkSystem.h"

using namespace network;

namespace Sigma {

	void FrameObject::SendMessage(int fd, unsigned char major, unsigned char minor, const std::function<void(unsigned char*,const unsigned char*,size_t)>* hasher) {
		if(fd < 0) {
			return;
		}
		Header()->flags |= 1 << HAS_VMAC_TAG;
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = packet_size - sizeof(Frame_hdr) + VMAC_SIZE;
		// The VMAC Hasher has been put under id #1 for client
		(*hasher)(VMAC_tag(), reinterpret_cast<const unsigned char*>(data.data()), packet_size);
//		LOG_DEBUG << "Bytes to send (with VMAC): " << packet_size + VMAC_SIZE;
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size + VMAC_SIZE) <= 0) {
			LOG_ERROR << "could not send authenticated frame" ;
		};
	}


	void FrameObject::SendMessage(id_t id, unsigned char major, unsigned char minor) {
		auto fd = NetworkNode::GetFileDescriptor(id);
		if(fd < 0) {
			return;
		}
//		SendMessage(fd, major, minor, CxData()->Hasher());
		SendMessageNoVMAC(fd, major, minor);
	}

	void FrameObject::SendMessage(unsigned char major, unsigned char minor) {
		auto fd = Authentication::GetNetworkSystem<CLIENT>()->cnx.Handle();
		if(fd < 0) {
			return;
		}
		SendMessage(fd, major, minor, Authentication::GetNetworkSystem<CLIENT>()->Hasher());
	}

	void FrameObject::SendMessageNoVMAC(int fd, unsigned char major, unsigned char minor) {
		if(fd < 0) {
			return;
		}
		Header()->flags |= 1 << HAS_VMAC_TAG;
		auto header = Header();
		header->type_major = major;
		header->type_minor = minor;
		FullFrame()->length = packet_size - sizeof(Frame_hdr);
		if (TCPConnection(fd, NETA_IPv4, SCS_CONNECTED).Send(reinterpret_cast<char*>(FullFrame()), packet_size) <= 0) {
			LOG_ERROR << "could not send frame with no tag" ;
	    }
	}

	void FrameObject::append(const void* in, std::size_t sizeBytes) {
		Resize(BodySize() + sizeBytes);
		std::memcpy(VMAC_tag() - sizeBytes, in, sizeBytes);
	}

	id_t FrameObject::GetId() const { return cx_data->Id(); }

	template<>
	FrameObject& FrameObject::operator<<(const std::string& in) {
		append(in.c_str(), in.size() + 1);
		return *this;
	}

	template<>
	FrameObject& FrameObject::operator<<(const std::vector<unsigned char>& in) {
		append(in.data(), in.size());
		return *this;
	}
}
