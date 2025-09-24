#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class BlockingQueue {
public:
	BlockingQueue() {}

	void push(T n) {
		std::unique_lock<std::mutex> lck(m_mut);

		m_task_queue.push(n);

		lck.unlock();
		m_not_empty.notify_one();
	}

	T pop() {
		std::unique_lock<std::mutex> lck(m_mut);

		m_not_empty.wait(lck, [this]() { return !m_task_queue.empty(); });

		T val = m_task_queue.front();
		m_task_queue.pop();

		return val;
	}
private:
	std::condition_variable m_not_empty;
	std::mutex m_mut;
	std::queue<T> m_task_queue;
};


#endif
