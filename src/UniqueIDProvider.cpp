#include "UniqueIDProvider.h"

namespace Sigma {
	std::shared_ptr<BitArray<unsigned int>> UniqueIDProvider::ba_ptr = BitArray<unsigned int>::Create(true);
	size_t UniqueIDProvider::last_free = 0;
}
