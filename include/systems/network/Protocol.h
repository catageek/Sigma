#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include <cstdint>
#include <memory>
#include <vector>
#include <cstring>
#include "systems/network/NetworkPacketHandler.hpp"

// size of the VMAc tag
#define VMAC_SIZE			8

// flags
#define	HAS_VMAC_TAG		0

// WARNING : We are little endian even on network !!!

// Version MAJOR codes
// unauthenticated messages: everyone can send them
#define TEST			0		// Reserved
#define NET_MSG 		1		// low level messages for authentication and disconnect
#define SERVER_MSG		2		// Server public information
#define PUBPLAYER_MSG	3		// Player public profile
#define	DLBINARY_MSG	4		// for binary download
#define	ASSETS_MSG		5		// for public assets download
#define	RESERVED_6		6
#define	RESERVED_7		7
// authenticated messages: only authenticated users can send and receive this
#define	PROFILE_MSG		8		// player private profile
#define WORLD_MSG		9		// world data
#define GAME_MSG		10		// game data
#define SOCIAL_MSG		11		// chat/mail
#define PASSETS_MSG		11		// private assets
#define CPU_MSG			12		// for CPU...
// Feel free to add or modify this list

#define IS_RESTRICTED(x)	((x >> 3) != 0)

namespace Sigma {
	class ConnectionData;

	namespace cryptography {
		class VMAC_StreamHasher;
	}

    /** \brief The header of the message, without the preceding length
     */
	struct msg_hdr {
		uint32_t flags;
		unsigned char type_major;
		unsigned char type_minor;
		char padding[2];
	};

    /** \brief The header of the frame, i.e the length
     */
	struct Frame_hdr {
		uint32_t length;
	};

    /** \brief The header including length
     */
	struct Frame {
		Frame_hdr fheader;
		msg_hdr mheader;
	};

	class FrameObject {
	public:
		friend class Authentication;
		template<int Major,int Minor,bool isClient> friend void network_packet_handler::INetworkPacketHandler::Process();
		friend class NetworkSystem;
		friend class Frame_req;

        /** \brief Constructor
         *
         * \param T the type of packet to construct
         * \param fd int the file descriptor of the socket (optional)
         * \param cxdata_ptr const ConnectionData* const the object storing data about the connection
         *
         */
		template<class T=Frame>
		FrameObject(const int fd = -1, const ConnectionData* const cx_data = nullptr) : cx_data(cx_data), fd(fd), packet_size(sizeof(T)), data(std::vector<char>(sizeof(T) + VMAC_SIZE)){};

		virtual ~FrameObject() {};

		// Copy constructor and assignment are deleted to remain zero-copy
		FrameObject(FrameObject&) = delete;
		FrameObject& operator=(FrameObject&) = delete;

		template<class T>
		FrameObject& operator<<(const T& in) {
			append(&in, sizeof(T));
			return *this;
		}

        /** \brief Send a message to a client using TCP
         *
         * \param id id_t the id of the entity to which the message must be sent
         * \param major unsigned char the major code of the message
         * \param minor unsigned char the minor code of the message
         *
         */
		void SendMessage(id_t id, unsigned char major, unsigned char minor);

        /** \brief Send a message to a server using TCP
         *
         * \param major unsigned char the major code of the message
         * \param minor unsigned char the minor code of the message
         *
         */
		void SendMessage(unsigned char major, unsigned char minor);

        /** \brief Resize the internal buffer.
         *
         * This function is specialized for the case where there is no VMAC tag to include
         *
         * \param new_size size_t the new size
         *
         */
		template<bool WITH_VMAC=true>
		void Resize(size_t new_size) {
			packet_size = new_size + sizeof(Frame);
			data.resize(packet_size + VMAC_SIZE);
		};

        /** \brief Maps the body of the message to the structure of the packet of type T
         *
		 * The internal buffer is dynamically resized.
         *
         * \return U* pointer to the packet T
         *
         */
		template<class T, class U=T>
		U* Content() {
			if(packet_size < sizeof(T) + sizeof(Frame)) {
				Resize(sizeof(T));
			}
			return reinterpret_cast<U*>(Body());
		}

        /** \brief Return the size of the frame including header, but without VMAC tag
         *
         * \return uint32_t the size
         *
         */
		uint32_t PacketSize() const { return packet_size; };

        /** \brief Return the entity id attached to this message
         *
         * \return id_t the id
         *
         */
		id_t GetId() const;

        /** \brief Return a pointer on the header of the message
         *
         * \return msg_hdr* the pointer on msg_hdr
         *
         */
		msg_hdr* Header() { return reinterpret_cast<msg_hdr*>(data.data() + sizeof(Frame_hdr)); };

        /** \brief Return a pointer on the frame, beginning at the length field
         *
         * \return Frame_hdr* the pointer
         *
         */
		Frame_hdr* FullFrame() { return reinterpret_cast<Frame_hdr*>(data.data()); };

	private:
        /** \brief Remove the VMAC tag
         *
         */
		void RemoveVMACtag() { packet_size -= VMAC_SIZE; };
		char* Body() { return ( data.data() + sizeof(msg_hdr) + sizeof(Frame_hdr)); };
		uint32_t BodySize() { return (VMAC_tag() - reinterpret_cast<unsigned char*>(Body())); };
		unsigned char* VMAC_tag() { return reinterpret_cast<unsigned char*>(data.data() + packet_size); };
		const unsigned char* VMAC_tag() const { return reinterpret_cast<const unsigned char*>(data.data() + packet_size); };

//		void SetConnectionData(const ConnectionData* const cx_data) { this->cx_data = cx_data; };
		const ConnectionData* CxData() const { return cx_data; };
		int FileDescriptor() const { return fd; };

		void SendMessage(int fd, unsigned char major, unsigned char minor, cryptography::VMAC_StreamHasher* hasher);
		void SendMessageNoVMAC(int fd, unsigned char major, unsigned char minor);

		void append(const void* in, std::size_t sizeBytes);

		std::vector<char> data;
		uint32_t packet_size;
		const ConnectionData* const cx_data;
		const int fd;
	};

	template<>
	void FrameObject::Resize<false>(size_t new_size);

	template<>
	FrameObject& FrameObject::operator<<(const std::string& in);
}

#endif // PROTOCOL_H_INCLUDED
