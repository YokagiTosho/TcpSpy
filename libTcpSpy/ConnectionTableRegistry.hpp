#ifndef CONNECTIONS_TABLE_REGISRY_HPP
#define CONNECTIONS_TABLE_REGISRY_HPP

#include "ConnectionEntry.hpp"
#include "ConnectionsTable.hpp"

#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <memory>

enum class ConnectionFilter {
};

template<typename K, typename V>
class Cache {
public:
    void set(const K &key, V val) {
        m_cache[key] = val;
    }
    
    const std::optional<V> get(const K &key) {
        if (auto it = m_cache.find(key); it != m_cache.end()) {
            return {it->second};
        }
        return {};
    }

    bool exists(const K &key) {
        return m_cache.find(key) != m_cache.end();
    }
private:
    std::unordered_map<K, V> m_cache{};
};

class ConnectionsableRegistry {
public:
    ConnectionsableRegistry()
    { }

    void update() {
        m_tcp_table4.update();
        m_tcp_table6.update();
        m_udp_table4.update();
        m_udp_table6.update();

        m_updated = false;
    }

    const ConnectionEntryPtrs &get() {
        if (m_updated) return m_rows;

        add_rows(m_tcp_table4);
        add_rows(m_tcp_table6);
        add_rows(m_udp_table4);
        add_rows(m_udp_table6);

        m_updated = true;

        return m_rows;
    }

private:
    template<typename T>
    void add_rows(ConnectionEntryPtrs &rows, T &table) {
        for (const auto& row : table) {
            std::shared_ptr<Process> proc = std::make_shared<Process>(row.dwOwningPid);

            

            m_rows.push_back(new T::ConnectionEntryT(row, Process(row.dwOwningPid)));
        }
    }

    TcpTable4 m_tcp_table4{};
    TcpTable6 m_tcp_table6{};
    UdpTable4 m_udp_table4{};
    UdpTable6 m_udp_table6{};

    std::unordered_set<int> m_filters;

    ConnectionEntryPtrs m_rows;
    Cache<DWORD, std::shared_ptr<Process>> m_proc_cache;
    bool m_updated{ false };
};

#endif
