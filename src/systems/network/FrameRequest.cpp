#include "systems/network/FrameRequest.h"

namespace Sigma {
	// We disable VMAC check when template parameter is false
	template<>
	bool Frame_req::CheckVMAC<NONE>() const { return true; };

	template<>
	bool Frame_req::CheckVMAC<CLIENT>() const { return true; };

	template<>
	bool Frame_req::CheckVMAC<SERVER>() const {
	return std::all_of(std::begin(reassembled_frames_list), std::end(reassembled_frames_list),
						[&](const std::unique_ptr<FrameObject>& message) { message->RemoveVMACtag(); return (*cx_data->Verifier())(message->VMAC_tag(), reinterpret_cast<unsigned char*>(message->FullFrame()), message->PacketSize()); });
	}

}
