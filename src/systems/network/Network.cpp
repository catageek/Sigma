#include "systems/network/Network.h"
#include "systems/network/NetworkSystem.h"

namespace Sigma {
	template<>
	void NetworkSystem::CloseConnection<CLIENT>(const FrameObject* frame) const {
		delete frame->CxData();
		RemoveClientConnection();
	}

	template<>
	void NetworkSystem::CloseConnection<SERVER>(const FrameObject* frame) const {
		RemoveConnection(frame->fd);
		delete frame->CxData();
	}

	template<>
	int NetworkSystem::UnauthenticatedDispatch<CLIENT>() const {
        auto req_list = GetPublicReassembledFrameQueue()->Poll();
        if (req_list.empty()) {
            return STOP;
        }
//					LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " PublicDispatchReq events";
        for (auto& req : req_list) {
            msg_hdr* header = req->Header();
            auto major = header->type_major;
            if (IS_RESTRICTED(major)) {
                LOG_DEBUG << "restricted";
                CloseConnection<CLIENT>(req.get());
                break;
            }
            switch(major) {
            case NET_MSG:
                {
                    auto minor = header->type_minor;
                    switch(minor) {
                        case AUTH_SEND_SALT:
                            {
                                network_packet_handler::INetworkPacketHandler<CLIENT>::GetQueue<NET_MSG,AUTH_SEND_SALT>()->Push(std::move(req));
                                break;
                            }
                        case AUTH_KEY_REPLY:
                            {
                                network_packet_handler::INetworkPacketHandler<CLIENT>::GetQueue<NET_MSG,AUTH_KEY_REPLY>()->Push(std::move(req));
                                break;
                            }
                        default:
                            {
                                LOG_DEBUG << "(" << sched_getcpu() << ") invalid minor code, closing";
                                CloseConnection<CLIENT>(req.get());
                            }
                    }
                    break;
                }
            default:
                {
                    LOG_DEBUG << "(" << sched_getcpu() << ") invalid major code in unauthenticated chain, packet of " << req->PacketSize() << " bytes, closing";
                    CloseConnection<CLIENT>(req.get());
                }
            }
        }
        if(! network_packet_handler::INetworkPacketHandler<CLIENT>::GetQueue<NET_MSG, AUTH_SEND_SALT>()->Empty()) {
            network_packet_handler::INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_SEND_SALT>();
        }
        if(! network_packet_handler::INetworkPacketHandler<CLIENT>::GetQueue<NET_MSG, AUTH_KEY_REPLY>()->Empty()) {
            network_packet_handler::INetworkPacketHandler<CLIENT>::Process<NET_MSG,AUTH_KEY_REPLY>();
        }
        return STOP;

	}

	template<>
	int NetworkSystem::UnauthenticatedDispatch<SERVER>() const {
	    					auto req_list = GetPublicReassembledFrameQueue()->Poll();
					if (req_list.empty()) {
						return STOP;
					}
//					LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " PublicDispatchReq events";
					for (auto& req : req_list) {
						msg_hdr* header = req->Header();
						auto major = header->type_major;
						if (IS_RESTRICTED(major)) {
							LOG_DEBUG << "restricted";
							CloseConnection<SERVER>(req.get());
							break;
						}
						switch(major) {
						case NET_MSG:
							{
								auto minor = header->type_minor;
								switch(minor) {
									case AUTH_INIT:
										{
											network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<NET_MSG,AUTH_INIT>()->Push(std::move(req));
											break;
										}
									case AUTH_KEY_EXCHANGE:
										{
											network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<NET_MSG,AUTH_KEY_EXCHANGE>()->Push(std::move(req));
											break;
										}
									default:
										{
											LOG_DEBUG << "(" << sched_getcpu() << ") invalid minor code, closing";
											CloseConnection<SERVER>(req.get());
										}
								}
								break;
							}
						default:
							{
								LOG_DEBUG << "(" << sched_getcpu() << ") invalid major code in unauthenticated chain, packet of " << req->PacketSize() << " bytes, closing";
								CloseConnection<SERVER>(req.get());
							}
						}
					}
					if(! network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<NET_MSG, AUTH_INIT>()->Empty()) {
						network_packet_handler::INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_INIT>();
					}
					if(! network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<NET_MSG, AUTH_KEY_EXCHANGE>()->Empty()) {
						network_packet_handler::INetworkPacketHandler<SERVER>::Process<NET_MSG,AUTH_KEY_EXCHANGE>();
					}
					return STOP;

	}

    template<>
	int NetworkSystem::AuthenticatedDispatch<SERVER>() const {
        auto req_list = NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Poll();
        if (req_list.empty()) {
            return STOP;
        }
        for (auto& req : req_list) {
            msg_hdr* header = req->Header();
            auto major = header->type_major;
            switch(major) {
                // select handlers
            case TEST:
                {
                    auto minor = header->type_minor;
                    switch(minor) {
                        case TEST:
                            {
                                network_packet_handler::INetworkPacketHandler<SERVER>::GetQueue<TEST,TEST>()->Push(std::move(req));
                                network_packet_handler::INetworkPacketHandler<SERVER>::Process<TEST,TEST>();
                                break;
                            }
                        default:
                            {
                                LOG_ERROR << "(" << sched_getcpu() << ") TEST: closing";
                                CloseConnection<SERVER>(req.get());
                            }
                    }
                    break;
                }

            default:
                {
                    LOG_ERROR << "(" << sched_getcpu() << ") Authenticated switch: closing";
                    CloseConnection<SERVER>(req.get());
                }
            }
        }
        return STOP;
    }

    template<>
	int NetworkSystem::AuthenticatedDispatch<CLIENT>() const {
        auto req_list = NetworkSystem::GetAuthenticatedCheckedFrameQueue()->Poll();
        if (req_list.empty()) {
            return STOP;
        }
//					LOG_DEBUG << "(" << sched_getcpu() << ") got " << req_list->size() << " AuthenticatedCheckedDispatchReq events";
        for (auto& req : req_list) {
            msg_hdr* header = req->Header();
            auto major = header->type_major;
            switch(major) {
                // select handlers
            case TEST:
                {
                    auto minor = header->type_minor;
                    switch(minor) {
                        case TEST:
                            {
                                network_packet_handler::INetworkPacketHandler<CLIENT>::GetQueue<TEST,TEST>()->Push(std::move(req));
                                network_packet_handler::INetworkPacketHandler<CLIENT>::Process<TEST,TEST>();
                                break;
                            }
                        default:
                            {
                                LOG_ERROR << "(" << sched_getcpu() << ") TEST: closing";
                                CloseConnection<CLIENT>(req.get());
                            }
                    }
                    break;
                }

            default:
                {
                    LOG_ERROR << "(" << sched_getcpu() << ") Authenticated switch: closing";
                    CloseConnection<CLIENT>(req.get());
                }
            }
        }
        return STOP;
    }
}
