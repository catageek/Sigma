#ifndef VMAC_HASHER_H_INCLUDED
#define VMAC_HASHER_H_INCLUDED

#include <unordered_map>
#include "Sigma.h"
#include "systems/network/VMAC_StreamHasher.h"
#include "systems/network/AuthenticationHandler.h"

namespace Sigma {
	class VMAC_Checker {
	public:
		VMAC_Checker() {};
		virtual ~VMAC_Checker() {};

		static void AddEntity(const id_t entity_id, std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, int nonce_size) {
			if(! hasher_map.count(entity_id)) {
				hasher_map.emplace(std::piecewise_construct, std::forward_as_tuple<const id_t>(std::move(entity_id)),
									std::forward_as_tuple< std::unique_ptr<std::vector<byte>>,const byte*, int>(std::move(key), std::move(nonce), std::move(nonce_size)));
			}
		};

		static bool Verify(id_t id, const byte* digest, const byte* message, uint32_t len) {
            auto hasher = hasher_map.find(id);
            if (hasher != hasher_map.end()) {
				return hasher->second.Verify(digest, message, len);
            }
            return false;
		};

		static bool Digest(id_t id, byte* digest, const byte* message, int len) {
            auto hasher = hasher_map.find(id);
            if (hasher != hasher_map.end()) {
				hasher->second.CalculateDigest(digest, message, len);
				return true;
            }
            return false;
		};

        static void RemoveEntity(const id_t id) {
            hasher_map.erase(id);
        };

	private:
		static std::unordered_map<id_t, cryptography::VMAC_StreamHasher> hasher_map;

	};	// class VMAC_Hasher

	namespace reflection {
		template <> inline const char* GetTypeName<VMAC_Checker>() { return "VMAC_Hasher"; }
		template <> inline const unsigned int GetTypeID<VMAC_Checker>() { return 12000; }
	}

} // namespace Sigma

#endif // VMAC_HASHER_H_INCLUDED
