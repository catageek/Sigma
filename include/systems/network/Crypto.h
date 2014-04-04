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
//		void InitializeDH();
//		void GetPublicKeys(char* buffer) const;
		static void VMAC64(byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);
		static void VMAC128(byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);
		static bool VMAC_Verify(const byte* digest, const byte* message, size_t len, const byte* key, const byte* nonce);

		static void GetRandom64(byte* nonce);
		static void GetRandom128(byte* nonce);

	private:

/*		DH _dh;
		SecByteBlock _priv;
		SecByteBlock _pub;
*/		static AutoSeededRandomPool _prng;
	};
}

#endif // CRYPTO_H_INCLUDED
