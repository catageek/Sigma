#include "systems/network/VMAC_StreamHasher.h"

#include <cstring>
#include <iostream>
#include "crypto++/vmac.h"
#include "crypto++/osrng.h"
#include "crypto++/dh.h"
#include "crypto++/integer.h"

using namespace CryptoPP;

namespace Sigma {
	namespace cryptography {
		VMAC_StreamHasher::VMAC_StreamHasher(std::unique_ptr<std::vector<byte>> key, const byte* nonce, size_t nonce_size)
			: _key(std::move(key)), _key_ptr(_key->data()), _key_size(_key->size()), _nonce_size(nonce_size) {
			_nonce = std::vector<byte>(_nonce_size);
			_nonce_ptr = _nonce.data();
			std::memcpy(_nonce_ptr, nonce, nonce_size);
			_hasher.SetKey(_key_ptr, _key_size, MakeParameters(Name::IV(), ConstByteArrayParameter(_nonce_ptr, _nonce_size, false), false));
			Integer nonc, k;
			nonc.Decode(_nonce_ptr, _nonce_size);
			k.Decode(_key->data(), 16);
		};

		bool VMAC_StreamHasher::Verify(const byte* digest, const byte* message, int len) {
			++(*_nonce_ptr);
			_hasher.Resynchronize(_nonce_ptr, _nonce_size);
			Integer vmac, nonc, k, m;
			vmac.Decode(digest, 8);
			nonc.Decode(_nonce_ptr, _nonce_size);
			k.Decode(_key->data(), 16);
			m.Decode(message, len);
			std::cout << ">>Verify VMAC" << std::endl;
			std::cout << "VMAC is " << std::hex << vmac << std::endl;
			std::cout << "nonce is " << std::hex << nonc << std::endl;
			std::cout << "key is " << std::hex << k << std::endl;
			std::cout << "message is " << std::hex << m << std::endl;
			return _hasher.VerifyDigest(digest, message, len);
		}

		void VMAC_StreamHasher::CalculateDigest(byte* digest, const byte* message, int len) {
			++(*_nonce_ptr);
			_hasher.Resynchronize(_nonce_ptr, _nonce_size);
			_hasher.CalculateDigest(digest, message, len);
/*			Integer vmac, nonc, k, m;
			vmac.Decode(digest, 8);
			nonc.Decode(_nonce_ptr, _nonce_size);
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
