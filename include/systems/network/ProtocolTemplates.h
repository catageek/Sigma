#ifndef PROTOCOLTEMPLATES_H_INCLUDED
#define PROTOCOLTEMPLATES_H_INCLUDED

#include "systems/network/Protocol.h"

namespace Sigma {
	extern template	void FrameObject::Resize<CLIENT>(size_t new_size);
	extern template	void FrameObject::Resize<SERVER>(size_t new_size);
	extern template	void FrameObject::Resize<NONE>(size_t new_size);
}

#endif // PROTOCOLTEMPLATES_H_INCLUDED
