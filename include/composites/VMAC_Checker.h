#ifndef VMAC_HASHER_H_INCLUDED
#define VMAC_HASHER_H_INCLUDED

#include <unordered_map>
#include "Sigma.h"
#include "systems/network/VMAC_StreamHasher.h"

namespace Sigma {
	typedef std::pair<id_t,cryptography::VMAC_StreamHasher> vmac_pair;
	typedef std::unique_ptr<vmac_pair> kqueue_embedded;

	class VMAC_Checker {
	public:
		VMAC_Checker() {};
		virtual ~VMAC_Checker() {};

		static vmac_pair* AddEntity(const id_t entity_id, std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, int nonce_size) {
			if(! hasher_map.count(entity_id)) {
				auto id2 = entity_id;
				auto tmp2 = new vmac_pair(std::piecewise_construct, std::forward_as_tuple<const id_t>(std::move(id2)),
											std::forward_as_tuple<std::unique_ptr<std::vector<byte>>, const byte*, int>(std::move(key), std::move(nonce), std::move(nonce_size)));
				auto tmp = kqueue_embedded(std::move(tmp2));
				auto ret = tmp.get();
				hasher_map.emplace(std::piecewise_construct, std::forward_as_tuple<const id_t>(std::move(entity_id)),
									std::forward_as_tuple<kqueue_embedded>(std::move(tmp)));
				return ret;
			}
		}

		static bool Verify(id_t id, const byte* digest, const byte* message, uint32_t len) {
			return hasher_map.find(id)->second->second.Verify(digest, message, len);
		}

		static void Digest(id_t id, byte* digest, const byte* message, uint32_t len) {
			hasher_map.find(id)->second->second.CalculateDigest(digest, message, len);
		}

        static void RemoveEntity(const id_t id) {
            hasher_map.erase(id);
        };

	private:
		static std::unordered_map<id_t, kqueue_embedded> hasher_map;

	};	// class VMAC_Hasher

	namespace reflection {
		template <> inline const char* GetTypeName<VMAC_Checker>() { return "VMAC_Hasher"; }
		template <> inline const unsigned int GetTypeID<VMAC_Checker>() { return 12000; }
	}

} // namespace Sigma

#endif // VMAC_HASHER_H_INCLUDED
