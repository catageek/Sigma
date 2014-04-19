#ifndef ESIGN_STREAMHASHER_H_INCLUDED
#define ESIGN_STREAMHASHER_H_INCLUDED

#include <functional>
#include "systems/network/Crypto.h"
#include "crypto++/esign.h"
#include "crypto++/tiger.h"

namespace Sigma {
	namespace cryptography {

	    using namespace std::placeholders;

		class Esign_StreamHasher {

		public:
			Esign_StreamHasher() {};

			virtual ~Esign_StreamHasher() {};

			void Initialize() {
				signer = ESIGN<Tiger>::Signer{privatekey};
				verifier = ESIGN<Tiger>::Verifier{publickey};
			}

			bool GenerateKeys() {
                InvertibleESIGNFunction parameters;

                // Modulus size should be a multiple of 3
                // k = 32 by default
                parameters.GenerateRandomWithKeySize( Crypto::RNG(), 64 * 3 );
                privatekey = ESIGN<Tiger>::PrivateKey{parameters};
                publickey = ESIGN<Tiger>::PublicKey{parameters};
			};

			void Sign(byte* sig, const byte* message, size_t len) const {
				signer.SignMessage(Crypto::RNG(), message, len, sig);
			}

			bool Verify(const byte* sig, const byte* message, size_t len) const {
				return verifier.VerifyMessage(message, len, sig, 24);
			}

			size_t MaxSignatureLength() const { return signer.MaxSignatureLength(); };

			std::unique_ptr<std::vector<byte>> PublicKey() const {
			    ByteQueue queue;
			    publickey.Save(queue);
			    auto ret = std::unique_ptr<std::vector<byte>>(new std::vector<byte>(queue.MaxRetrievable()));
			    queue.Get(ret->data(), ret->size());
			    return ret;
			}

			std::function<void(unsigned char*,const unsigned char*,size_t)> Hasher() const {
			    return std::bind(&Sigma::cryptography::Esign_StreamHasher::Sign, this, _1, _2, _3);
			}

			std::function<bool(const unsigned char*,const unsigned char*,size_t)> Verifier() const {
			    return std::bind(&Sigma::cryptography::Esign_StreamHasher::Verify, this, _1, _2, _3);
			}

		private:
			ESIGN<Tiger>::PrivateKey privatekey;
			ESIGN<Tiger>::PublicKey publickey;
			ESIGN<Tiger>::Signer signer;
			ESIGN<Tiger>::Verifier verifier;
		};
	}
}

#endif // ESIGN_STREAMHASHER_H_INCLUDED
