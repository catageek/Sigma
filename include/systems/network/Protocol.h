#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include <cstdint>
#include <memory>
#include <vector>
#include "systems/network/NetworkPacketHandler.hpp"
#include "systems/network/NetworkClient.h"
#include "composites/VMAC_Checker.h"

#define VMAC_SIZE			8

// flags
#define	HAS_VMAC_TAG		0

// We are little endian !!!

// Version MAJOR codes
// unauthenticated messages
#define TEST			0
#define NET_MSG 		1		// low level messages for authentication and disconnect
#define SERVER_MSG		2		// Server public information
#define PUBPLAYER_MSG	3		// Player public profile
#define	DLBINARY_MSG	4		// for binary download
#define	ASSETS_MSG		5		// for public assets download
#define	RESERVED_6		6
#define	RESERVED_7		7
// authenticated messages
#define	PROFILE_MSG		8		// player private profile
#define WORLD_MSG		9		// world data
#define GAME_MSG		10		// game data
#define SOCIAL_MSG		11		// chat/mail
#define PASSETS_MSG		11		// private assets
#define CPU_MSG			12		// for CPU...

#define IS_RESTRICTED(x)	((x >> 3) != 0)

namespace Sigma {
	struct msg_hdr {
		uint32_t flags;
		unsigned char type_major;
		unsigned char type_minor;
		char padding[2];
	};

	struct Frame_hdr {
		uint32_t length;
	};

	struct Frame {
		Frame_hdr fheader;
		msg_hdr mheader;
	};

	class FrameObject {
	public:
		friend class Authentication;
		template<int Major,int Minor> friend void network_packet_handler::INetworkPacketHandler::Process();
		friend void NetworkClient::SendMessage(unsigned char, unsigned char, FrameObject&);
		friend void NetworkClient::SendUnauthenticatedMessage(unsigned char, unsigned char, FrameObject&);

		template<class T=Frame_hdr>
		FrameObject(int fd = -1, vmac_pair* vmac_ptr = nullptr) : fd(fd), packet_size(sizeof(T)), data(std::vector<char>(sizeof(T) + VMAC_SIZE)),
																	vmac_verifier(vmac_ptr) {};

		// Copy constructor and assignment are deleted to remain zero-copy
		FrameObject(FrameObject&) = delete;
		FrameObject& operator=(FrameObject&) = delete;

		void SendMessage(id_t id, unsigned char major, unsigned char minor);

		template<bool WITH_VMAC=true>
		void Resize(size_t new_size) {
			packet_size = new_size + sizeof(Frame);
			data.resize(packet_size + VMAC_SIZE);
		};


		void Set_VMAC_Flag();

		bool Verify_VMAC_tag();

		template<class T, class U=T>
		U* Content() {
			if(packet_size < sizeof(T) + sizeof(Frame)) {
				Resize(sizeof(T));
			}
			return reinterpret_cast<U*>(Body());
		}

		uint32_t PacketSize() const { return packet_size; };

		id_t GetId() const;

		msg_hdr* Header() { return reinterpret_cast<msg_hdr*>(data.data() + sizeof(Frame_hdr)); };
		Frame_hdr* FullFrame() { return reinterpret_cast<Frame_hdr*>(data.data()); };

		int fd;
	private:
		char* Body() { return ( data.data() + sizeof(msg_hdr) + sizeof(Frame_hdr)); };
		unsigned char* VMAC_tag() { return reinterpret_cast<unsigned char*>(data.data() + packet_size); };
		const unsigned char* VMAC_tag() const { return reinterpret_cast<const unsigned char*>(data.data() + packet_size); };
		void SendMessage(int fd, unsigned char major, unsigned char minor);

		std::vector<char> data;
		uint32_t packet_size;
		vmac_pair* const vmac_verifier;
	};
}

#endif // PROTOCOL_H_INCLUDED
