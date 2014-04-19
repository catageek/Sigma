#ifndef FRAMEREQUEST_H_INCLUDED
#define FRAMEREQUEST_H_INCLUDED

#include <chrono>
#include "Sigma.h"
#include "systems/network/AuthenticationHandler.h"
#include "systems/network/ConnectionData.h"

#define TIMEOUT 3

namespace Sigma {
    /** \brief This object holds the data needed to reassemble frames. Instances are provided to the reassembling block for processing.
     */
	class Frame_req {
	public:
        /** \brief Constructor
         *
         * fd int the file descriptor of the socket
         * length_total size_t number of byte to reassemble
         * cxdata_ptr const ConnectionData* const a pointer on an object storing some data on the connection
         */
		Frame_req(const int fd, size_t length_total, const ConnectionData* const cxdata_ptr)
				: fd(fd), reassembled_frames_list(), length_total(length_total), length_requested(sizeof(Frame_hdr)), length_got(0),
				timeout(TIMEOUT), expiration_time(std::chrono::steady_clock::now() + timeout), cx_data(cxdata_ptr) {
			reassembled_frames_list.emplace_back(new FrameObject(fd, cxdata_ptr));
		};

		// delete copy constructor and assignment for zero-copy guarantee
		Frame_req(Frame_req&) = delete;
		Frame_req& operator=(Frame_req&) = delete;

        /** \brief Verify the VMAC tag of all the frames reassembled
         *
         * \return bool true if the VMAC tag was successfully verified
         *
         */
        template<TagType T>
		bool CheckVMAC() const;

        /** \brief Return a pointer on the object storing data on the connection
         *
         * \return const ConnectionData* const the object
         *
         */
		const ConnectionData* const CxData() const { return cx_data; };

		void UpdateTimestamp() { this->expiration_time = std::chrono::steady_clock::now() + timeout; };

		bool HasExpired() const { return (std::chrono::steady_clock::now() > expiration_time) ; };

		std::list<std::unique_ptr<FrameObject>> reassembled_frames_list;
		uint32_t length_requested;
		uint32_t length_got;
		size_t length_total;
		const int fd;
	private:
		std::chrono::steady_clock::time_point expiration_time;
		const std::chrono::seconds timeout;
		const ConnectionData* const cx_data;
	};
}

#endif // FRAMEREQUEST_H_INCLUDED
