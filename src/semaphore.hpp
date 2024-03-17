#pragma once
#include <pthread.h>
#include <iostream>

struct Semaphore {
private:
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int value = 0;

	void check_error(int succ, const char* op, const char* func) {
		if (succ != 0) {
			std::cerr << "[Semaphore] " << op << " returned " << succ << " in " << func << std::endl;
		}
	}
public:
	Semaphore(int value = 0) : value(value) {
		check_error(pthread_mutex_init(&mutex, nullptr), "mutex init", "Semaphore()");
		check_error(pthread_cond_init(&cond, nullptr), "cond init", "Semaphore()");
	}

	Semaphore(const Semaphore&) = delete;

	~Semaphore() {
		check_error(pthread_cond_destroy(&cond), "cond destroy", "~Semaphore()");
		check_error(pthread_mutex_destroy(&mutex), "mutex destroy", "~Semaphore()");
	}

	void reset() {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "reset()");

		value = 0;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "reset()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "reset()");	
	}

	void wait_and_lock(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "wait_and_lock()");
		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "wait_and_lock()");
	
	}

	void unlock() {
		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "unlock()");
	}

	void set(int val) {
		value = val;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "set()");
	}

	void wait_and_reset(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "wait_and_reset()");
		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "wait_and_reset()");

		value = 0;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "wait_and_reset()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "wait_and_reset()");
	}

	void inc_and_wait(int wait_value) {
		check_error(pthread_mutex_lock(&mutex), "mutex lock", "inc_and_wait()");

		value++;
		check_error(pthread_cond_broadcast(&cond), "cond broadcast", "inc_and_wait()");

		while (value != wait_value)
			check_error(pthread_cond_wait(&cond, &mutex), "cond wait", "inc_and_wait()");

		check_error(pthread_mutex_unlock(&mutex), "mutex unlock", "inc_and_wait()");	
	}
};

