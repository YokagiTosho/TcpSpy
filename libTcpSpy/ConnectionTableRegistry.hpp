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
	enum class Filters {
		IPv4,
		IPv6,
		UDP,
	};

	ConnectionsTableRegistry()
	{
	}

	void update() {
		m_tcp_table4.update();
		m_tcp_table6.update();
		m_udp_table4.update();
		m_udp_table6.update();

		if (m_rows.size()) m_rows.clear();

		// just to update m_rows, result is not needed
		add_rows(m_tcp_table4);
		add_rows(m_tcp_table6);
		add_rows(m_udp_table4);
		add_rows(m_udp_table6);
	}

	const ConnectionEntryPtrs& get() {
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

	void add_filter(Filters filter) {
		m_filters.insert(filter);
	}

	bool remove_filter(Filters filter) {
		if (m_filters.find(filter) != m_filters.end()) {
			m_filters.erase(filter);
			return true;
		}
		return false;
	}

	void sort(SortBy sort_by, bool asc_order = true) {
		using TCP_ptr = ConnectionEntryTCP*;

		auto beg = m_rows.begin();

		switch (sort_by) {
		case SortBy::RemoteAddress:
		case SortBy::RemotePort:
		case SortBy::State:
		{
			auto it = std::partition(beg, m_rows.end(), [asc_order](const ConnectionEntryPtr& r) {
				if (r->protocol() == ConnectionProtocol::PROTO_UDP) {
					return asc_order;
				}
				else {
					return !asc_order;
				}
				});
			
			if (asc_order) {
				beg = it;
			}
		}
			break;
		}

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
			std::sort(beg, m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
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
			std::sort(beg, m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
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
			std::sort(beg, m_rows.end(), [asc_order](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
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

	std::unordered_set<Filters> m_filters{ Filters::IPv4, Filters::IPv6, Filters::UDP };

	std::unordered_map<DWORD, ProcessPtr> m_proc_cache{};

	bool m_updated{ false };
};

#endif
