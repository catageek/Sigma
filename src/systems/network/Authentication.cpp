#include "systems/network/Authentication.h"

namespace Sigma {
	Crypto Authentication::crypto(true);

	std::shared_ptr<DHKeyExchangePacket> Authentication::GetDHKeysPacket() {
		auto packet = std::make_shared<DHKeyExchangePacket>();
		crypto.GetPublicKeys(reinterpret_cast<char*>(packet->dhkey));
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		crypto.GetRandom64(packet->nonce);
		crypto.GetRandom128(packet->alea);
		crypto.VMAC(packet->vmac, reinterpret_cast<const byte*>(packet.get()), 56, key, packet->nonce);
		return packet;
	}

	bool DHKeyExchangePacket::VerifyVMAC() {
		// TODO : hard coded key
		byte key[] = "very_secret_key";
		return Authentication::GetCryptoEngine()->VMAC_Verify(vmac, reinterpret_cast<const byte*>(this), 56, key, nonce);
	}

	bool Authentication::ComputeSharedSecretKey(const DHKeyExchangePacket& packet) {};

}
