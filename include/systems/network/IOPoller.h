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

		void Watch(int fd);

		void Unwatch(int fd);

		int Poll(std::vector<struct kevent>& v);
	private:
		class IOEvent;			// An IO event
		int kqhandle;			// the handle of the kqueue
		const timespec ts{};	// for non-blocking kevent
	};

	// implementation of the functions are here for inline

	class IOPoller::IOEvent {
	public:
		IOEvent(int fd, int filter, int operation) { EV_SET(&ke, fd, filter, operation, 0, 5, NULL); };
		virtual ~IOEvent() {};

		const struct kevent* getStruct() { return &ke; };

	private:
		struct kevent ke;
	};


	inline void IOPoller::Watch(int fd) {
		IOEvent e(fd, EVFILT_READ, EV_ADD);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred in Watch(): ");
			return;
		}
	}

	inline void IOPoller::Unwatch(int fd) {
		IOEvent e(fd, EVFILT_READ, EV_DELETE);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
	}

	inline int IOPoller::Poll(std::vector<struct kevent>& v) {
		return kevent(kqhandle, NULL, 0, v.data(), v.size(), &ts);
	}
}

#endif // IOPOLLER_H_INCLUDED
