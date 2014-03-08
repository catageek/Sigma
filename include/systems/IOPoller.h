#ifndef IOPOLLER_H_INCLUDED
#define IOPOLLER_H_INCLUDED

#include "sys/event.h"
#include <vector>

namespace Sigma {
	class IOPoller {
	public:
		IOPoller() {};
		virtual ~IOPoller() {};

		bool Initialize();

		void Watch(int fd);

		void Unwatch(int fd);

		int Poll(std::vector<struct kevent>& v, const struct timespec *timeout = NULL);
	private:
		class IOEvent;			// An IO event
		int kqhandle;			// the handle of the kqueue
	};
}

#endif // IOPOLLER_H_INCLUDED
