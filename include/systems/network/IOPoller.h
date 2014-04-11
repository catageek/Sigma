#ifndef IOPOLLER_H_INCLUDED
#define IOPOLLER_H_INCLUDED

#include <vector>
#include <cstddef>
#include <cstdio>
#include "sys/event.h"
#include "sys/time.h"
#include "Log.h"

namespace Sigma {
	class IOPoller {
	public:
		IOPoller() {};
		virtual ~IOPoller() {};

		bool Initialize() {
			kqhandle = kqueue();
			return kqhandle != -1;
		}

		IOPoller(IOPoller&) = delete;
		IOPoller& operator=(IOPoller&) = delete;

		void Create(const int fd) const;

		void Create(const int fd, void* udata) const;

		void CreatePermanent(int fd) const;

		template<bool isClient>
		void Watch(const int fd) const {};

		void Unwatch(const int fd) const;

		void Delete(const int fd) const;

		int Poll(std::vector<struct kevent>& v) const;
	private:
		class IOEvent;			// An IO event
		mutable int kqhandle;			// the handle of the kqueue
		const timespec ts{};	// for non-blocking kevent
	};

	// implementation of the functions are here for inline

	class IOPoller::IOEvent {
	public:
		IOEvent(int fd, int filter, int operation, void* udata = NULL) { EV_SET(&ke, fd, filter, operation, 0, 5, udata); };
		virtual ~IOEvent() {};

		const struct kevent* getStruct() const { return &ke; };

	private:
		struct kevent ke;
	};

	inline void IOPoller::Create(int fd) const {
		IOEvent e(fd, EVFILT_READ, EV_ADD|EV_DISPATCH, NULL);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
			return;
		}
	}

	inline void IOPoller::Create(int fd, void* udata) const {
		IOEvent e(fd, EVFILT_READ, EV_ADD|EV_DISPATCH, udata);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
			return;
		}
	}

	inline void IOPoller::CreatePermanent(int fd) const {
		IOEvent e(fd, EVFILT_READ, EV_ADD);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
			return;
		}
	}

	template<>
	inline void IOPoller::Watch<false>(int fd) const {
		IOEvent e(fd, EVFILT_READ, EV_ENABLE);
		LOG_DEBUG << "Watching...";
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
			return;
		}
	}

	inline void IOPoller::Unwatch(int fd) const {
		IOEvent e(fd, EVFILT_READ, EV_DISABLE);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
	}

	inline void IOPoller::Delete(int fd) const {
		IOEvent e(fd, EVFILT_READ, EV_DELETE);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
	}

	inline int IOPoller::Poll(std::vector<struct kevent>& v) const {
//		return kevent(kqhandle, NULL, 0, v.data(), v.size(), &ts);
		auto i = kevent(kqhandle, NULL, 0, v.data(), v.size(), NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
		}
		return i;
	}
}

#endif // IOPOLLER_H_INCLUDED
