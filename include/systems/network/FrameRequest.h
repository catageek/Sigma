#ifndef FRAMEREQUEST_H_INCLUDED
#define FRAMEREQUEST_H_INCLUDED

#include "Sigma.h"
#include "systems/network/AuthenticationHandler.h"

namespace Sigma {
	class Frame_req {
	public:
		Frame_req(int fd, size_t length_total, vmac_pair* vmac_ptr = nullptr) : reassembled_frames_list(),
			length_total(length_total), length_requested(sizeof(Frame_hdr)), length_got(0) {
				reassembled_frames_list.emplace_back(new FrameObject(fd, vmac_ptr));
			};

		// delete copy constructor and assignment for zero-copy guarantee
		Frame_req(Frame_req&) = delete;
		Frame_req& operator=(Frame_req&) = delete;

		template<bool isClient, bool hasVMAC>
		bool CheckVMAC() const {
			return std::all_of(begin(reassembled_frames_list), end(reassembled_frames_list), [](const std::unique_ptr<FrameObject>& f) { return f->Verify_Authenticity<isClient>(); });
		}


		std::list<std::unique_ptr<FrameObject>> reassembled_frames_list;
		uint32_t length_requested;
		uint32_t length_got;
		size_t length_total;
		void (*return_queue)(void);
	};
}

#endif // FRAMEREQUEST_H_INCLUDED
