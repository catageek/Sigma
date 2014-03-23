#ifndef CRYPTO_H_INCLUDED
#define CRYPTO_H_INCLUDED

#include "crypto++/osrng.h"
#include "crypto++/dh.h"

using namespace CryptoPP;

namespace CryptoPP {
	template<class T, int> class VMAC;
}

namespace Sigma {

	class Crypto {
	public:
		Crypto(bool blocking = false) : _prng(blocking) {};
		virtual ~Crypto() {};

		void InitializeDH();
		void GetPublicKeys(char* buffer) const;
		void VMAC64(byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);
		void VMAC128(byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);
		bool VMAC_Verify(const byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);

		void GetRandom64(byte* nonce);
		void GetRandom128(byte* nonce);

		void SetSalt(std::unique_ptr<std::vector<byte>>&& salt) { _salt = std::move(salt); };

	private:

		DH _dh;
		SecByteBlock _priv;
		SecByteBlock _pub;
		AutoSeededRandomPool _prng;
		std::unique_ptr<std::vector<byte>> _salt;
	};
}

#endif // CRYPTO_H_INCLUDED
