#ifndef VMAC_STREAMHASHER_H_INCLUDED
#define VMAC_STREAMHASHER_H_INCLUDED

#include <mutex>
#include "crypto++/vmac.h"
#include "crypto++/aes.h"
#include "crypto++/integer.h"

namespace Sigma {
	namespace cryptography {
        /** \brief Provide a stateful VMAC hasher using an internal counter as nonce
         *
         */
		class VMAC_StreamHasher {
		public:
            /** \brief Constructor
             *
             * \param key std::unique_ptr<std::vector<byte>>& The secret key
             * \param nonce const byte* the initial nonce
             * \param nonce_size size_t the nonce size (in bytes)
             *
             */
			VMAC_StreamHasher(std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, size_t nonce_size);
			VMAC_StreamHasher() : _nonce_size{}, _key_ptr{}, _key_size{} {};
			virtual ~VMAC_StreamHasher() {};

			VMAC_StreamHasher(VMAC_StreamHasher&) = delete;
			VMAC_StreamHasher& operator=(const VMAC_StreamHasher&) = delete;

            /** \brief Verify a digest
             *
             * This function is thread-safe
             *
             * \param digest const byte* the digest to verify (8 bytes)
             * \param message const byte* the message associated with the digest
             * \param len int the length of the message
             * \return bool true if the verification is positive, false otherwise
             *
             */
			bool Verify(const byte* digest, const byte* message, int len) const ;

            /** \brief Compute a digest of 8 bytes
             *
             * This function is thread-safe
             *
             * \param digest byte* the 8-byte buffer to fill
             * \param message const byte* the message to hash
             * \param len int the length of the message (in bytes)
             *
             */
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
