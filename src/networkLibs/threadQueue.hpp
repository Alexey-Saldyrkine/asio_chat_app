#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <coroutine>

template<typename T>
struct threadQueue {
private:
	std::queue<T> q;
	mutable std::mutex mtx;
	std::condition_variable cv;
public:
	bool empty() const {
		std::unique_lock lock(mtx);
		return q.empty();
	}

	void push(T value) {
		std::unique_lock lock(mtx);
		q.push(std::move(value));
		cv.notify_one();
	}

	T pop() {
		std::unique_lock lock(mtx);
		cv.wait(lock, [this] {
			return !q.empty();
		});
		T value = std::move(q.front());
		q.pop();
		return value;
	}

};

