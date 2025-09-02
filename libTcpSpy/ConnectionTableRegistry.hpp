#ifndef CONNECTIONS_TABLE_REGISTRY_HPP
#define CONNECTIONS_TABLE_REGISTRY_HPP

#include "ConnectionsTable.hpp"

#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>
#include <algorithm>

enum class SortBy {
	ProcessName,
	PID,
	Protocol,
	INET,
	LocalAddress,
	LocalPort,
	RemoteAddress,
	RemotePort,
	State,
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

		if (m_rows.size()) m_rows.clear();

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

	ConnectionEntryPtrs::iterator begin() {
		return m_rows.begin();
	}

	ConnectionEntryPtrs::iterator end() {
		return m_rows.end();
	}

	

	void sort(SortBy sort_by, bool asc_order = true) {
		using TCP_ptr = ConnectionEntryTCP*;

		switch (sort_by)
		{
		case SortBy::ProcessName:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return a->get_process_name() < b->get_process_name();
				else           return a->get_process_name() > b->get_process_name();
				});
			break;
		case SortBy::PID:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return a->pid() < b->pid();
				else           return a->pid() > b->pid();
				});
			break;
		case SortBy::Protocol:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return (int)a->protocol() < (int)b->protocol();
				else           return (int)a->protocol() > (int)b->protocol();
				});
			break;
		case SortBy::INET:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return (int)a->address_family() < (int)b->address_family();
				else           return (int)a->address_family() > (int)b->address_family();
				});
			break;
		case SortBy::LocalAddress:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return a->local_addr_str() < b->local_addr_str();
				else           return a->local_addr_str() > b->local_addr_str();
				});
			break;
		case SortBy::LocalPort:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (asc_order) return a->local_port() < b->local_port();
				else           return a->local_port() > b->local_port();
				});
			break;
		// TCP rows can be compared, while UDP rows do not have remote addr, remote port, state
		case SortBy::RemoteAddress:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				TCP_ptr tcp_a{ nullptr };
				TCP_ptr tcp_b{ nullptr };

				if ((tcp_a = dynamic_cast<TCP_ptr>(a.get())) &&
					(tcp_b = dynamic_cast<TCP_ptr>(b.get())))
				{
					if (asc_order) return tcp_a->remote_addr_str() < tcp_b->remote_addr_str();
					else           return tcp_a->remote_addr_str() > tcp_b->remote_addr_str();
				}
				// unable to cast becase its not ConnectionEntryTCP*, but UDP
				return false;
				});
			break;
		case SortBy::RemotePort:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				TCP_ptr tcp_a{ nullptr };
				TCP_ptr tcp_b{ nullptr };

				
				if ((tcp_a = dynamic_cast<TCP_ptr>(a.get())) &&
					(tcp_b = dynamic_cast<TCP_ptr>(b.get())))
				{
					if (asc_order) return tcp_a->remote_port() < tcp_b->remote_port();
					else           return tcp_a->remote_port() > tcp_b->remote_port();
				}
				return false;
				});
			break;
		case SortBy::State:
			std::sort(m_rows.begin(), m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				TCP_ptr tcp_a{ nullptr };
				TCP_ptr tcp_b{ nullptr };

				if ((tcp_a = dynamic_cast<TCP_ptr>(a.get())) &&
					(tcp_b = dynamic_cast<TCP_ptr>(b.get())))
				{
					if (asc_order) return tcp_a->state() < tcp_b->state();
					else           return tcp_a->state() > tcp_b->state();
				}
				return false;
				});
			break;
		default:
			break;
		}

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
				if (!proc_ptr->open()) {
					continue; // do not store processes and thus ConnectionEntry
				}

				m_proc_cache[proc_pid] = proc_ptr;
			}

			m_rows.push_back(std::make_unique<typename T::ConnectionEntryT>(row, proc_ptr));
		}
	}

	TcpTable4 m_tcp_table4{};
	TcpTable6 m_tcp_table6{};
	UdpTable4 m_udp_table4{};
	UdpTable6 m_udp_table6{};

	ConnectionEntryPtrs m_rows;
	std::unordered_set<int> m_filters;

	std::unordered_map<DWORD, ProcessPtr> m_proc_cache{};

	bool m_updated{ false };
};

#endif
