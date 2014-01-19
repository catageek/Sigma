#include "UniqueIDProvider.h"

namespace Sigma {
	BitArray<unsigned int> UniqueIDProvider::ba_ptr = BitArray<unsigned int>(true);
	size_t UniqueIDProvider::last_free = 0;
}
