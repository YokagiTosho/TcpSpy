#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <stdexcept>

#include <functional>

#include "ConnectionEntry.hpp"

template<typename T, typename R>
class ConnectionsTable {
public:
    ConnectionsTable()
    {
    }

    const T &get_table() {
        return *m_table;
    }

    DWORD update_table();

    ~ConnectionsTable() { free_table(); }

private:
    typedef DWORD (__stdcall *GetExtendedTcpTablePtr)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG);
    typedef DWORD (__stdcall *GetExtendedUdpTablePtr)(PVOID, PDWORD, BOOL, ULONG, UDP_TABLE_CLASS, ULONG);

    using GetExtendedTablePtrs = std::variant<GetExtendedTcpTablePtr, GetExtendedUdpTablePtr>;

    template<typename Table, typename TableClass>
    DWORD _update_table(
        GetExtendedTablePtrs getExtendedTable, 
        ProtocolFamily af, 
        std::variant<TCP_TABLE_CLASS, UDP_TABLE_CLASS> tableClass)
    {
        DWORD dwRes = std::get<Table>(getExtendedTable)
            (m_table, &m_size, TRUE, (ULONG)af, std::get<TableClass>(tableClass), 0);
        
        if (dwRes == ERROR_INSUFFICIENT_BUFFER) {
            free_table();
            alloc_table();

            dwRes = std::get<Table>(getExtendedTable)
                (m_table, &m_size, TRUE, (ULONG)af, std::get<TableClass>(tableClass), 0);

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
DWORD ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedTcpTablePtr, TCP_TABLE_CLASS>(
            GetExtendedTcpTable,
            ProtocolFamily::INET,
            TCP_TABLE_OWNER_PID_ALL);

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedTcpTablePtr, TCP_TABLE_CLASS>(
        GetExtendedTcpTable,
        ProtocolFamily::INET6,
        TCP_TABLE_OWNER_PID_ALL);

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedUdpTablePtr, UDP_TABLE_CLASS>(
        GetExtendedUdpTable,
        ProtocolFamily::INET,
        UDP_TABLE_OWNER_PID);

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedUdpTablePtr, UDP_TABLE_CLASS>(
        GetExtendedUdpTable,
        ProtocolFamily::INET6,
        UDP_TABLE_OWNER_PID);

    return dwRes;
}
