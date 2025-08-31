#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <stdexcept>

#include <functional>
#include <type_traits>
#include <variant>

#include "ConnectionEntry.hpp"

using TableClasses = std::variant<TCP_TABLE_CLASS, UDP_TABLE_CLASS>;

template<typename T, typename R>
class ConnectionsTable {
public:
    struct Iterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = R;
        using pointer           = R*;
        using reference         = R&;

        Iterator(pointer ptr)
            : m_ptr(ptr)
        {}

        Iterator(const Iterator &it)
            : m_ptr(it.m_ptr)
        {}

        Iterator(Iterator &&it) noexcept
            : m_ptr(it.m_ptr)
        {
            it.m_ptr = nullptr;
        }

        reference operator*() {
            return *m_ptr;
        }

        pointer operator->() {
            return m_ptr;
        }

        Iterator &operator++() {
            m_ptr++;
            return *this;
        }

        Iterator &operator++(int) {
            Iterator tmp = *this;

            ++(*this);

            return tmp;
        }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };
    private:
        pointer m_ptr;
    };

    // Type for suitable ConnectionEntry type
    using ConnectionEntryT = std::conditional_t<
        std::is_same_v<T, MIB_TCPTABLE_OWNER_PID>,
        ConnectionEntry4TCP,
        std::conditional_t<
        std::is_same_v<T, MIB_TCP6TABLE_OWNER_PID>,
        ConnectionEntry6TCP,
        std::conditional_t<
        std::is_same_v<T, MIB_UDPTABLE_OWNER_PID>,
        ConnectionEntry4UDP,
        ConnectionEntry6UDP
        >
        >
    >;

    ConnectionsTable()
    {
    }

    ConnectionsTable(ConnectionsTable &&ct) noexcept
        : m_table(ct.m_table), m_size(ct.m_size)
    {
        ct.m_table = nullptr;
        ct.m_size = sizeof(T);
    }

    DWORD update();

    ~ConnectionsTable() { free_table(); }

    Iterator begin() { return m_table ? Iterator(&m_table->table[0]) : nullptr; }
    Iterator end() { return m_table ? Iterator(&m_table->table[m_table->dwNumEntries]) : nullptr; }

private:
    typedef DWORD (__stdcall *GetExtendedTcpTablePtr)(PVOID, PDWORD, BOOL, ULONG, TCP_TABLE_CLASS, ULONG);
    typedef DWORD (__stdcall *GetExtendedUdpTablePtr)(PVOID, PDWORD, BOOL, ULONG, UDP_TABLE_CLASS, ULONG);

    using GetExtendedTablePtrs = std::variant<GetExtendedTcpTablePtr, GetExtendedUdpTablePtr>;


    template<typename Table, typename TableClass, ULONG AddressFamily = AF_INET>
    DWORD _update_table(
            GetExtendedTablePtrs getExtendedTable,
            TableClasses tableClass)
    {
        if (m_table == nullptr) {
            alloc_table();
        }

        DWORD dwRes = std::get<Table>(getExtendedTable)
            (m_table, &m_size, TRUE, AddressFamily, std::get<TableClass>(tableClass), 0);
        
        if (dwRes == ERROR_INSUFFICIENT_BUFFER) {
            free_table();
            alloc_table();

            dwRes = std::get<Table>(getExtendedTable)
                (m_table, &m_size, TRUE, AddressFamily, std::get<TableClass>(tableClass), 0);

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

using TcpTable4 = ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>;
using TcpTable6 = ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>;
using UdpTable4 = ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>;
using UdpTable6 = ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>;

template<>
DWORD ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedTcpTablePtr, TCP_TABLE_CLASS>
        (
         GetExtendedTcpTable,
         TCP_TABLE_OWNER_PID_ALL
        );

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedTcpTablePtr, TCP_TABLE_CLASS, AF_INET6>
        (
         GetExtendedTcpTable,
         TCP_TABLE_OWNER_PID_ALL
        );

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedUdpTablePtr, UDP_TABLE_CLASS>
        (
         GetExtendedUdpTable,
         UDP_TABLE_OWNER_PID
        );

    return dwRes;
}

template<>
DWORD ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>::update_table() {
    DWORD dwRes = _update_table<GetExtendedUdpTablePtr, UDP_TABLE_CLASS, AF_INET6>
        (
         GetExtendedUdpTable,
         UDP_TABLE_OWNER_PID
        );

    return dwRes;
}
