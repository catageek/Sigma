#include "composites/VMAC_Checker.h"

namespace Sigma {
	std::unordered_map<id_t, cryptography::VMAC_StreamHasher> VMAC_Checker::hasher_map;
}
