#ifndef VMAC_STREAMHASHER_H_INCLUDED
#define VMAC_STREAMHASHER_H_INCLUDED

#include "crypto++/vmac.h"
#include "crypto++/aes.h"

namespace Sigma {
	namespace cryptography {
		class VMAC_StreamHasher {
		public:
			VMAC_StreamHasher(std::unique_ptr<std::vector<byte>> key, const byte* nonce, size_t nonce_size);
			virtual ~VMAC_StreamHasher() {};

			bool Verify(const byte* digest, const byte* message, int len);

			void CalculateDigest(byte* digest, const byte* message, int len);

		private:
			std::vector<byte> _nonce;
			byte* _nonce_ptr;
			const size_t _nonce_size;
			std::unique_ptr<std::vector<byte>> _key;
			const byte* _key_ptr;
			const size_t _key_size;
			CryptoPP::VMAC<CryptoPP::AES,64> _hasher;
		};
	}
}

#endif // VMAC_STREAMHASHER_H_INCLUDED
