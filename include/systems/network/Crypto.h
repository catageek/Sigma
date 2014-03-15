#ifndef CRYPTO_H_INCLUDED
#define CRYPTO_H_INCLUDED

#include "crypto++/osrng.h"
#include "crypto++/cryptlib.h"

using namespace CryptoPP;

namespace Sigma {
	class Crypto {
	public:
		Crypto(bool blocking = false) : _prng(blocking) {};
		virtual ~Crypto() {};

		uint64_t GetNonce() {
			uint64_t ret;
			_prng.GenerateBlock(reinterpret_cast<byte*>(&ret), 8);
			return ret;
		};

	private:
		AutoSeededRandomPool _prng;
	};
}

#endif // CRYPTO_H_INCLUDED
