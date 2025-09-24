#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <iostream>
#include <vector>
#include <thread>
#include <functional>

#include "BlockingQueue.hpp"

using Task = std::function<void()>;

class ThreadPool {
public:
	ThreadPool(int workers_size = 1)
		: m_workers_size(workers_size)
	{
		for (int i = 0; i < workers_size; i++) {
			m_workers.push_back(std::thread([this]() {
				worker_main_loop();
			}));
		}
	}

	void submit(Task task) {
		m_queue.push(task);
	}

	void stop() {
		for (int i = 0; i < m_workers_size; i++) {
			m_queue.push(nullptr);
		}
		for (int i = 0; i < m_workers_size; i++) {
			m_workers[i].join();
		}
	}
private:
	void worker_main_loop() {
		Task task = nullptr;
		while (task = m_queue.pop()) { task(); }
	}

	int m_workers_size;
	std::vector<std::thread> m_workers;
	BlockingQueue<Task> m_queue;
};

#endif
