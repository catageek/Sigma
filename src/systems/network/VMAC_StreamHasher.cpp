#include "systems/network/VMAC_StreamHasher.h"

#include <cstring>
//#include <iostream>
#include "crypto++/vmac.h"
#include "crypto++/osrng.h"
#include "crypto++/dh.h"
#include "crypto++/integer.h"

using namespace CryptoPP;

namespace Sigma {
	namespace cryptography {
		VMAC_StreamHasher::VMAC_StreamHasher(std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, size_t nonce_size)
			: _key(std::move(key)), _key_ptr(_key->data()), _key_size(_key->size()), _nonce_size(nonce_size), _nonce(nonce, nonce_size) {
			_hasher.SetKey(_key_ptr, _key_size, MakeParameters(Name::IV(), ConstByteArrayParameter(nonce, _nonce_size, false), false));
		};

		bool VMAC_StreamHasher::Verify(const byte* digest, const byte* message, int len) const {
			std::vector<byte> nonce(_nonce_size);
			std::lock_guard<std::mutex> locker(vmac_mutex);
			++_nonce;
/*			Integer vmac, nonc, k, m;
			vmac.Decode(digest, 8);
			nonc.Decode(nonce.data(), _nonce_size);
			k.Decode(_key->data(), 16);
			m.Decode(message, len);
			std::cout << ">>Verify VMAC" << std::endl;
			std::cout << "VMAC is " << std::hex << vmac << std::endl;
			std::cout << "nonce is " << std::hex << nonc << std::endl;
			std::cout << "key is " << std::hex << k << std::endl;
			std::cout << "message is " << std::hex << m << std::endl;
*/			_nonce.Encode(nonce.data(), _nonce_size);
			_hasher.Resynchronize(nonce.data(), _nonce_size);
			return _hasher.VerifyDigest(digest, message, len);
		}

		void VMAC_StreamHasher::CalculateDigest(byte* digest, const byte* message, int len) const {
			std::vector<byte> nonce(_nonce_size);
			std::lock_guard<std::mutex> locker(vmac_mutex);
			++_nonce;
			_nonce.Encode(nonce.data(), _nonce_size);
			_hasher.Resynchronize(nonce.data(), _nonce_size);
			_hasher.CalculateDigest(digest, message, len);
/*			Integer vmac, nonc, k, m;
			vmac.Decode(digest, 8);
			nonc.Decode(nonce.data(), _nonce_size);
			k.Decode(_key->data(), 16);
			m.Decode(message, len);
			std::cout << ">>Compute VMAC" << std::endl;
			std::cout << "VMAC is " << std::hex << vmac << std::endl;
			std::cout << "nonce is " << std::hex << nonc << std::endl;
			std::cout << "key is " << std::hex << k << std::endl;
			std::cout << "message is " << std::hex << m << std::endl;
*/		}
	}
}
