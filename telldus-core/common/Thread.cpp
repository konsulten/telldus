//
// C++ Implementation: Thread
//
// Description: 
//
//
// Author: Micke Prag <micke.prag@telldus.se>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "Thread.h"

using namespace TelldusCore;

class TelldusCore::ThreadPrivate {
public:
#ifdef _WINDOWS
	HANDLE thread;
	DWORD threadId;
	HANDLE noEvent, noWait;
#else
	pthread_t thread;
	pthread_cond_t noEvent, noWait;
#endif
	MUTEX_T mutex;
	
	//Must be locked by the mutex!
	bool hasEvent;
	std::string strMessage;
	int intMessage;
};

Thread::Thread() {
	d = new ThreadPrivate;
	d->thread = 0;
	initMutex(&d->mutex);
	
	lockMutex(&d->mutex);
	d->hasEvent = false;
	unlockMutex(&d->mutex);
	
#ifdef _WINDOWS
	d->noEvent = CreateEvent(0, FALSE, FALSE, 0);
	d->noWait = CreateEvent(0, FALSE, FALSE, 0);
#else
	pthread_cond_init(&d->noEvent, NULL);
	pthread_cond_init(&d->noWait, NULL);
#endif
}

Thread::~Thread() {
	destroyMutex(&d->mutex);
#ifdef _WINDOWS
	CloseHandle(d->noEvent);
	CloseHandle(d->noWait);
#else
	pthread_cond_destroy(&d->noEvent);
	pthread_cond_destroy(&d->noWait);
#endif
	delete d;
}

void Thread::start() {
#ifdef _WINDOWS
	d->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&Thread::exec, this, 0, &d->threadId);
#else
	pthread_create(&d->thread, NULL, &Thread::exec, this );
#endif
}

bool Thread::wait() {
	if (!d->thread) {
		return true;
	}
#ifdef _WINDOWS
	WaitForSingleObject(d->thread, INFINITE);
	CloseHandle(d->thread);
#else
	pthread_join(d->thread, 0);
#endif
	return true;
}

void *Thread::exec( void *ptr ) {
	Thread *t = reinterpret_cast<Thread *>(ptr);
	if (t) {
		t->run();
	}
	return 0;
}

std::string Thread::waitForEvent(int *intMessage) {
	std::string strMessage;
	lockMutex(&d->mutex);
	while(!d->hasEvent) {
#ifdef _WINDOWS
		unlockMutex(&d->mutex);
		WaitForSingleObject(d->noWait, INFINITE);
		lockMutex(&d->mutex);
#else
		pthread_cond_wait(&d->noWait, &d->mutex);
#endif
	}
	d->hasEvent = false;
	strMessage = d->strMessage;
	(*intMessage) = d->intMessage;
	unlockMutex(&d->mutex);
#ifdef _WINDOWS
	SetEvent(d->noEvent);
#else
	pthread_cond_broadcast(&d->noEvent);
#endif
	return strMessage;
}

void Thread::sendEvent(const std::string &strMessage, int intMessage) {
	lockMutex(&d->mutex);
	while (d->hasEvent) { //We have an unprocessed event
#ifdef _WINDOWS
		unlockMutex(&d->mutex);
		WaitForSingleObject(d->noEvent, INFINITE);
		lockMutex(&d->mutex);
#else
		pthread_cond_wait(&d->noEvent, &d->mutex);
#endif
	}
	d->hasEvent = true;
	d->strMessage = strMessage;
	d->intMessage = intMessage;
	unlockMutex(&d->mutex);
#ifdef _WINDOWS
	SetEvent(d->noWait);
#else
	pthread_cond_broadcast(&d->noWait);
#endif
}

void Thread::initMutex(MUTEX_T * m) {
#ifdef _WINDOWS
	InitializeCriticalSection(m);
#else
	pthread_mutex_init(m, NULL);
#endif
}

void Thread::destroyMutex(MUTEX_T * m) {
#ifdef _WINDOWS
	DeleteCriticalSection(m);
#else
	pthread_mutex_destroy(m);
#endif
}

void Thread::unlockMutex(MUTEX_T * m) {
#ifdef _WINDOWS
	LeaveCriticalSection(m);
#else
	pthread_mutex_unlock(m);
#endif
}

void Thread::lockMutex(MUTEX_T * m) {
#ifdef _WINDOWS
	EnterCriticalSection(m);
#else
	pthread_mutex_lock(m);
#endif
}


MutexLocker::MutexLocker(MUTEX_T *m)
	:mutex(m) {
	Thread::lockMutex(mutex);
}

MutexLocker::~MutexLocker() {
	Thread::unlockMutex(mutex);
}