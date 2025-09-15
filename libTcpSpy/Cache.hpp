#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <optional>
#include <mutex>

template<typename Key, typename Value>
class Cache {
public:
	Value set(Key key, Value value) {
		if (auto it = m_cache.find(key); it != m_cache.end()) {
			return it->second;
		}

		std::scoped_lock<std::mutex> lck(m_mut);

		m_cache[key] = value;

		return value;
	}

	std::optional<Value> get(Key key) {
		std::scoped_lock<std::mutex> lck(m_mut);

		return m_cache.find(key) != m_cache.end() ? m_cache[key] : std::optional<Value>();
	}
private:
	std::unordered_map<Key, Value> m_cache{};
	std::mutex m_mut;
};

#endif