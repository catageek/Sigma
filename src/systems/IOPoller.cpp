#include <cstddef>
#include <cstdio>
#include "systems/IOPoller.h"
#include "sys/event.h"
#include "Log.h"

namespace Sigma {

	class IOPoller::IOEvent {
	public:
		IOEvent(int fd, int filter, int operation) { EV_SET(&ke, fd, filter, operation, 0, 5, NULL); };
		virtual ~IOEvent() {};

		struct kevent* getStruct() { return &ke; };

	private:
		struct kevent ke;
	};

	bool IOPoller::Initialize() {
		kqhandle = kqueue();
		return kqhandle != -1;
	}

	void IOPoller::Watch(int fd) {
		IOEvent e(fd, EVFILT_READ, EV_ADD);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			perror ("The following error occurred");
			LOG_ERROR << "Could not watch event " << fd;
			return;
		}
		LOG_DEBUG << "watching " << fd;
	}

	void IOPoller::Unwatch(int fd) {
		IOEvent e(fd, EVFILT_READ, EV_DELETE);
		auto i = kevent(kqhandle, e.getStruct(), 1, NULL, 0, NULL);
		if (i == -1) {
			LOG_ERROR << "Could not unwatch event " << fd;
			return;
		}
		LOG_DEBUG << "unwatching " << fd;
	}

	int IOPoller::Poll(std::vector<struct kevent>& v, const struct timespec *timeout) {
		return kevent(kqhandle, NULL, 0, v.data(), v.size(), timeout);
	}
}
