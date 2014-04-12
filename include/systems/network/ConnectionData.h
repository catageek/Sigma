#ifndef CONNECTIONDATA_H_INCLUDED
#define CONNECTIONDATA_H_INCLUDED

#include "systems/network/VMAC_StreamHasher.h"

namespace Sigma {
	class ConnectionData {
	public:
		ConnectionData(const unsigned char state) : _auth_state(state), _id(0) {};
		ConnectionData(const id_t id, std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, int nonce_size)
						: _auth_state(0), _id(id), _hasher(std::move(key), nonce, nonce_size) {};
		virtual ~ConnectionData() {};
		ConnectionData(ConnectionData&) = delete;
		ConnectionData& operator=(ConnectionData&) = delete;

		bool CompareAuthState(unsigned char state) const {
			std::lock_guard<std::mutex> locker(_authstate_mutex);
			return (_auth_state == state);
		}

		void SetAuthState(unsigned char state) const {
			std::lock_guard<std::mutex> locker(_authstate_mutex);
			_auth_state = state;
		}

		unsigned char AuthState() const {
			std::lock_guard<std::mutex> locker(_authstate_mutex);
			return _auth_state;
		}

		bool TryLockConnection() const { return _cx_mutex.try_lock(); };

		void ReleaseConnection() const { _cx_mutex.unlock(); };

		const cryptography::VMAC_StreamHasher* const Hasher() const { return &_hasher; };

		id_t Id() const { return _id; };
	private:
		mutable unsigned char _auth_state;
		mutable std::mutex _authstate_mutex;
		const id_t _id;
		const cryptography::VMAC_StreamHasher _hasher;
		mutable std::mutex _cx_mutex;
	};
}
#endif // CONNECTIONDATA_H_INCLUDED
