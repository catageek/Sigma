#include "systems/network/FrameRequest.h"

namespace Sigma {
	// We disable VMAC check when template parameter is false
	template<>
	bool Frame_req::CheckVMAC<false, false>() const { return true; };

	template<>
	bool Frame_req::CheckVMAC<true,false>() const { return true; };

	template<>
	bool Frame_req::CheckVMAC<true, true>() const { return true; };

	template<> bool Frame_req::CheckVMAC<false,true>() const {
	return std::all_of(std::begin(reassembled_frames_list), std::end(reassembled_frames_list),
						[&](const std::unique_ptr<FrameObject>& message) { message->RemoveVMACtag(); return cx_data->Hasher()->Verify(message->VMAC_tag(), reinterpret_cast<unsigned char*>(message->FullFrame()), message->PacketSize()); });
	}

}
