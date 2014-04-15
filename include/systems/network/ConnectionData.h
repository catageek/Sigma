#ifndef CONNECTIONDATA_H_INCLUDED
#define CONNECTIONDATA_H_INCLUDED

#include <atomic>
#include "systems/network/VMAC_StreamHasher.h"

namespace Sigma {
    /** \brief This object is attached to each socket
     */
	class ConnectionData {
	public:
        /** \brief Constructor
         *
         * \param state const unsigned char the initial state
         *
         */
		ConnectionData(const unsigned char state) : _auth_state(state), _id(0) {};

        /** \brief Constructor
         *
         * \param id const id_t the id of the entity
         * \param key std::unique_ptr<std::vector<byte>>&& the key used in the VMAC hasher (16 bytes)
         * \param nonce const byte* the nonce used in the VMAC hasher
         * \param nonce_size int the size of the nonce
         *
         */
		ConnectionData(const id_t id, std::unique_ptr<std::vector<byte>>&& key, const byte* nonce, int nonce_size)
						: _auth_state(0), _id(id), _hasher(std::move(key), nonce, nonce_size) {};

		virtual ~ConnectionData() { _cx_mutex.unlock(); };

		// Copy functions are deleted
		ConnectionData(ConnectionData&) = delete;
		ConnectionData& operator=(ConnectionData&) = delete;

        /** \brief Compare atomically the current state of the connection
         *
         * \param state unsigned char the state to compare with
         * \return bool true if equal, false otherwise
         *
         */
		bool CompareAuthState(unsigned char state) const {
			return (_auth_state.load() == state);
		}

        /** \brief Set atomically the state of the connection
         *
         * \param state unsigned char the state to set
         *
         */
		void SetAuthState(unsigned char state) const {
			_auth_state.store(state);
		}

        /** \brief Return the state of the connection
         *
         * \return unsigned char the state
         *
         */
		unsigned char AuthState() const {
			return _auth_state.load();
		}

        /** \brief Try to lock the mutex associated with this socket
         *
         * \return bool true if the lock is acquired, false otherwise
         *
         */
		bool TryLockConnection() const { return _cx_mutex.try_lock(); };

        /** \brief Release the mutex associated with this connection
         *
         */
		void ReleaseConnection() const { _cx_mutex.unlock(); };

        /** \brief Return the VMAC hasher associated to this socket
         *
         * \return const cryptography::VMAC_StreamHasher* const the hasher
         *
         */
		const cryptography::VMAC_StreamHasher* const Hasher() const { return &_hasher; };

        /** \brief Return the id of the entity to which this socket is attached
         *
         * \return id_t the id
         *
         */
		id_t Id() const { return _id; };
	private:
		mutable std::atomic_uchar _auth_state;
		const id_t _id;
		const cryptography::VMAC_StreamHasher _hasher;
		mutable std::mutex _cx_mutex;
	};
}
#endif // CONNECTIONDATA_H_INCLUDED
