#include "systems/network/FrameRequest.h"

namespace Sigma {
	template<>
	bool Frame_req::CheckVMAC<false, false>() const { std::cout << "ok"; return true; };

	template<>
	bool Frame_req::CheckVMAC<true, false>() const { return true; };
}
