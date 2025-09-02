#ifndef CONNECTIONS_TABLE_REGISTRY_HPP
#define CONNECTIONS_TABLE_REGISTRY_HPP

#include "ConnectionsTable.hpp"

#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>
#include <algorithm>
#include <functional>

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
		TCP_CONNECTIONS,
		TCP_LISTENING,
	};

	ConnectionsTableRegistry()
	{
	}

	void update() {
		m_tcp_table4.clear();
		m_tcp_table6.clear();
		m_udp_table4.clear();
		m_udp_table6.clear();

		if (m_rows.size()) m_rows.clear();

		if (m_filters.contains(Filters::IPv4)) {
			update_tcp_table(m_tcp_table4);

			if (m_filters.contains(Filters::UDP)) {
				m_udp_table4.update(UDP_TABLE_OWNER_PID);
				add_rows(m_udp_table4);
			}
		}

		if (m_filters.contains(Filters::IPv6)) {
			update_tcp_table(m_tcp_table6);

			if (m_filters.contains(Filters::UDP)) {
				m_udp_table6.update(UDP_TABLE_OWNER_PID);
				add_rows(m_udp_table6);
			}
		}

		sort(SortBy::ProcessName);
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
		auto beg = m_rows.begin();

		switch (sort_by) {
		case SortBy::RemoteAddress:
		case SortBy::RemotePort:
		case SortBy::State:
		{
			// Move UDP entries to the beginning or the end, depending on sorting.
			// UDP entries do not have remote address, remote port and state
			// so that they will be always in the beginning, or the end
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

		auto cmpFun = [asc_order]<typename T>(T a, T b) {
			if (asc_order) return a < b;
			else           return a > b;
			};

		switch (sort_by)
		{
		case SortBy::ProcessName:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun(a->get_process_name(), b->get_process_name());
				});
			break;
		case SortBy::PID:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun(a->pid(), b->pid());
				});
			break;
		case SortBy::Protocol:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun((int)a->protocol(), (int)b->protocol());
				});
			break;
		case SortBy::INET:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun((int)a->address_family(), (int)b->address_family());
				});
			break;
		case SortBy::LocalAddress:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun(a->local_addr_str(), b->local_addr_str());
				});
			break;
		case SortBy::LocalPort:
			std::sort(m_rows.begin(), m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				return cmpFun(a->local_port(), b->local_port());
				});
			break;
		// TCP rows can be compared, while UDP rows do not have remote addr, remote port, state
		case SortBy::RemoteAddress:
			std::sort(beg, m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (a->protocol() == ConnectionProtocol::PROTO_TCP &&
					b->protocol() == ConnectionProtocol::PROTO_TCP)
					return cmpFun(
						dynamic_cast<ConnectionEntryTCP*>(a.get())->remote_addr_str(),
						dynamic_cast<ConnectionEntryTCP*>(b.get())->remote_addr_str()
					);
				return false;
				});
			break;
		case SortBy::RemotePort:
			std::sort(beg, m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (a->protocol() == ConnectionProtocol::PROTO_TCP &&
					b->protocol() == ConnectionProtocol::PROTO_TCP) 
					return cmpFun(
						dynamic_cast<ConnectionEntryTCP*>(a.get())->remote_port(),
						dynamic_cast<ConnectionEntryTCP*>(b.get())->remote_port()
					);
				return false;
				});
			break;
		case SortBy::State:
			std::sort(beg, m_rows.end(), [&cmpFun](const ConnectionEntryPtr& a, const ConnectionEntryPtr& b) {
				if (a->protocol() == ConnectionProtocol::PROTO_TCP &&
					b->protocol() == ConnectionProtocol::PROTO_TCP)
					return cmpFun(
						dynamic_cast<ConnectionEntryTCP*>(a.get())->state(),
						dynamic_cast<ConnectionEntryTCP*>(b.get())->state()
					);
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

	template<typename Table>
	void update_tcp_table(Table &table) {
		bool table_updated = false;
		TCP_TABLE_CLASS tcp_class;

		if (m_filters.contains(Filters::TCP_CONNECTIONS) &&
			m_filters.contains(Filters::TCP_LISTENING)) {
			tcp_class = TCP_TABLE_OWNER_PID_ALL;
			table_updated = true;
		}
		else if (m_filters.contains(Filters::TCP_CONNECTIONS)) {
			tcp_class = TCP_TABLE_OWNER_PID_CONNECTIONS;
			table_updated = true;
		}
		else if (m_filters.contains(Filters::TCP_LISTENING)) {
			tcp_class = TCP_TABLE_OWNER_PID_LISTENER;
			table_updated = true;
		}

		if (table_updated) {
			table.update(tcp_class);
			add_rows(table);
		}
	}

	TcpTable4 m_tcp_table4{};
	TcpTable6 m_tcp_table6{};
	UdpTable4 m_udp_table4{};
	UdpTable6 m_udp_table6{};

	ConnectionEntryPtrs m_rows;

	std::unordered_set<Filters> m_filters{ 
		Filters::IPv4, Filters::IPv6, Filters::TCP_CONNECTIONS,	Filters::TCP_LISTENING, Filters::UDP
	};

	std::unordered_map<DWORD, ProcessPtr> m_proc_cache{};

	bool m_updated{ false };
};

#endif
