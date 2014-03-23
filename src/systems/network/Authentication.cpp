#include "systems/network/Authentication.h"
#include <cstring>
#include "composites/VMAC_Checker.h"
#include "crypto++/vmac.h"

namespace Sigma {
	Crypto Authentication::crypto(true);

	std::shared_ptr<KeyExchangePacket> Authentication::GetKeyExchangePacket() {
		auto packet = std::make_shared<KeyExchangePacket>();
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		crypto.GetRandom64(packet->nonce);
		crypto.GetRandom128(packet->nonce2);
		crypto.GetRandom128(packet->alea);
		crypto.VMAC64(packet->vmac, reinterpret_cast<const byte*>(packet.get()), VMAC_MSG_SIZE, key, packet->nonce);
		return packet;
	}

	bool KeyExchangePacket::VerifyVMAC() {
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		return Authentication::GetCryptoEngine()->VMAC_Verify(vmac, reinterpret_cast<const byte*>(this), VMAC_MSG_SIZE, key, nonce);
	}

	bool KeyExchangePacket::VMAC_BuildHasher() {
		// TODO : hard coded key of the player
		byte key[] = "very_secret_key";
		// The variable that will hold the hasher key for this session
		std::unique_ptr<std::vector<byte>> checker_key(new std::vector<byte>(16));
		// We derive the player key using the alea given by the player
		Authentication::GetCryptoEngine()->VMAC128(checker_key->data(), alea, ALEA_SIZE, key, nonce2);
		// We store the key in the component
		// TODO : entity ID is hardcoded
		LOG_DEBUG << "Inserting checker";
		VMAC_Checker::AddEntity(1, std::move(checker_key), nonce2, 8);
		return true;
	}

	bool KeyReplyPacket::Compute(const byte* m) {
		std::memcpy(challenge, m, NONCE2_SIZE);
		return VMAC_Checker::Digest(1, vmac, challenge, NONCE2_SIZE);
	}

	bool KeyReplyPacket::VerifyVMAC() {
		return VMAC_Checker::Verify(1, vmac, challenge, NONCE2_SIZE);
	}
}
