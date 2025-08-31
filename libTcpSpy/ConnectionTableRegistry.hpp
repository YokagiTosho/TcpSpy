#ifndef CONNECTIONS_TABLE_REGISTRY_HPP
#define CONNECTIONS_TABLE_REGISTRY_HPP

#include "ConnectionsTable.hpp"

#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>

enum class ConnectionFilter {
};

class ConnectionsTableRegistry {
public:
	ConnectionsTableRegistry()
	{
	}

	void update() {
		m_tcp_table4.update();
		m_tcp_table6.update();
		m_udp_table4.update();
		m_udp_table6.update();

		m_updated = false;

		// just to update m_rows, result is not needed
		get();
	}

	const ConnectionEntryPtrs& get() {
		if (m_updated) return m_rows;

		add_rows(m_tcp_table4);
		add_rows(m_tcp_table6);
		add_rows(m_udp_table4);
		add_rows(m_udp_table6);

		m_updated = true;

		return m_rows;
	}

	size_t size() const {
		return m_rows.size();
	}

private:
	template<typename T>
	void add_rows(T& table) {
		for (const auto& row : table) {
			DWORD proc_pid = row.dwOwningPid;
			ProcessPtr proc_ptr;

			if (auto it = m_proc_cache.find(proc_pid); it != m_proc_cache.end()) {
				// proc is in cache
				proc_ptr = it->second;

			}
			else {
				proc_ptr = std::make_shared<Process>(row.dwOwningPid);
				proc_ptr->open();

				m_proc_cache[proc_pid] = proc_ptr;
			}

			m_rows.push_back(std::make_unique<typename T::ConnectionEntryT>(row, proc_ptr));
		}
	}

	TcpTable4 m_tcp_table4{};
	TcpTable6 m_tcp_table6{};
	UdpTable4 m_udp_table4{};
	UdpTable6 m_udp_table6{};

	std::unordered_set<int> m_filters;

	ConnectionEntryPtrs m_rows;

	std::unordered_map<DWORD, ProcessPtr> m_proc_cache{};

	bool m_updated{ false };
};

#endif
