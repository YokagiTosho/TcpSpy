#include <stddef.h>
#include <stdlib.h>
#include <new>

#include <functional>

#include "ConnectionEntry.hpp"

template<typename T, typename R>
class ConnectionsTable {
public:
    ConnectionsTable()
    {
        alloc_table();
    }

    const T &get_table() {
        return *m_table;
    }

    int update_table();

    ~ConnectionsTable() { free_table(); }

private:
    typedef DWORD (__stdcall *GetExtendedTcpTablePtr)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG);
    typedef DWORD (__stdcall *GetExtendedUdpTablePtr)(PVOID, PDWORD, BOOL, ULONG, UDP_TABLE_CLASS, ULONG);

    using GetExtendedTablePtrs = std::variant<GetExtendedTcpTablePtr, GetExtendedUdpTablePtr>;

    DWORD _call(
        GetExtendedTablePtrs getExtendedTable, 
        ConnectionProtocol proto,
        ProtocolFamily af,
        std::variant<TCP_TABLE_CLASS, UDP_TABLE_CLASS> tableClass)
    {
        DWORD dwRes;
        switch (proto) {
        case ConnectionProtocol::PROTO_TCP:
            dwRes = std::get<GetExtendedTcpTablePtr>(getExtendedTable)
                (m_table, &m_size, TRUE, (ULONG)af, std::get<TCP_TABLE_CLASS>(tableClass), 0);
            break;
        case ConnectionProtocol::PROTO_UDP:
            dwRes = std::get<GetExtendedUdpTablePtr>(getExtendedTable)
                (m_table, &m_size, TRUE, (ULONG)af, std::get<UDP_TABLE_CLASS>(tableClass), 0);
            break;
        default:
            return -1;
        }
        

        return dwRes;
    }

    int _update_table(
        GetExtendedTablePtrs getExtendedTable, 
        ConnectionProtocol proto, 
        ProtocolFamily af, 
        std::variant<TCP_TABLE_CLASS, UDP_TABLE_CLASS> tableClass)
    {
        DWORD dwRes = _call(getExtendedTable, proto, af, tableClass);
        
        if (dwRes == ERROR_INSUFFICIENT_BUFFER) {
            free_table();
            alloc_table();

            dwRes = _call(getExtendedTable, proto, af, tableClass);

            if (dwRes != NO_ERROR) {
                free_table();
                return dwRes;
            }
        }

        return dwRes;
    }

    void alloc_table() {
        m_table = (T*)malloc(m_size);
        if (!m_table) {
            throw std::bad_alloc();
        }
    }

    void free_table() {
        if (m_table) {
            free(m_table);
            m_table = nullptr;
        }
    }

    T *m_table { nullptr };
    DWORD m_size { sizeof(T) };
};


template<>
int ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>::update_table() {
    int dwRes = _update_table(
        GetExtendedTcpTable, 
        ConnectionProtocol::PROTO_TCP, 
        ProtocolFamily::INET, 
        TCP_TABLE_OWNER_PID_ALL);

    return dwRes;
}

template<>
int ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>::update_table() {
    int dwRes = _update_table(
        GetExtendedTcpTable,
        ConnectionProtocol::PROTO_TCP,
        ProtocolFamily::INET6,
        TCP_TABLE_OWNER_PID_ALL);

    return dwRes;
}

template<>
int ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>::update_table() {
    int dwRes = _update_table(
        GetExtendedUdpTable,
        ConnectionProtocol::PROTO_UDP,
        ProtocolFamily::INET,
        UDP_TABLE_OWNER_PID);

    return dwRes;
}

template<>
int ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>::update_table() {
    int dwRes = _update_table(
        GetExtendedUdpTable,
        ConnectionProtocol::PROTO_UDP,
        ProtocolFamily::INET6,
        UDP_TABLE_OWNER_PID);

    return dwRes;
}
