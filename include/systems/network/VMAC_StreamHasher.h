#ifndef VMAC_STREAMHASHER_H_INCLUDED
#define VMAC_STREAMHASHER_H_INCLUDED

#include <mutex>
#include "crypto++/vmac.h"
#include "crypto++/aes.h"
#include "crypto++/integer.h"

namespace Sigma {
	namespace cryptography {
		class VMAC_StreamHasher {
		public:
			VMAC_StreamHasher(std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, size_t nonce_size);
			virtual ~VMAC_StreamHasher() {};

//			VMAC_StreamHasher(VMAC_StreamHasher&) = delete;
			VMAC_StreamHasher& operator=(const VMAC_StreamHasher&) = delete;

			bool Verify(const byte* digest, const byte* message, int len) const ;

			void CalculateDigest(byte* digest, const byte* message, int len) const;

		private:
			mutable CryptoPP::Integer _nonce;
			const size_t _nonce_size;
			const std::unique_ptr<std::vector<byte>> _key;
			const byte* const _key_ptr;
			const size_t _key_size;
			mutable CryptoPP::VMAC<CryptoPP::AES,64> _hasher;
			mutable std::mutex vmac_mutex;
		};
	}
}

#endif // VMAC_STREAMHASHER_H_INCLUDED
