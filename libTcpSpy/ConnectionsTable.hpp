#include <stddef.h>
#include <stdlib.h>
#include <new>
#include <stdexcept>

#include <functional>
#include <type_traits>
#include <variant>

#include "ConnectionEntry.hpp"

template<typename T, typename R>
class ConnectionsTable {
public:
	struct Iterator {
		using iterator_category = std::input_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = R;
		using pointer = R*;
		using reference = R&;

		Iterator(pointer ptr)
			: m_ptr(ptr)
		{
		}

		Iterator(const Iterator& it)
			: m_ptr(it.m_ptr)
		{
		}

		Iterator(Iterator&& it) noexcept
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

		Iterator& operator++() {
			m_ptr++;
			return *this;
		}

		Iterator& operator++(int) {
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

	ConnectionsTable(ConnectionsTable&& ct) noexcept
		: m_table(ct.m_table), m_size(ct.m_size)
	{
		ct.m_table = nullptr;
		ct.m_size = sizeof(T);
	}

	template<typename T>
	DWORD update(T tableClass);

	void clear() {
		if (m_table != nullptr) {
			memset(m_table, 0, m_size);
		}
	}

	~ConnectionsTable() { free_table(); }

	Iterator begin() { return m_table ? Iterator(&m_table->table[0]) : nullptr; }
	Iterator end() { return m_table ? Iterator(&m_table->table[m_table->dwNumEntries]) : nullptr; }

private:
	template<typename Func, typename TableClass>
	DWORD _update_table(
		Func getExtendedTable,
		TableClass tableClass,
		ULONG AddressFamily = AF_INET)
	{
		if (m_table == nullptr) {
			alloc_table();
		}

		// m_size will be overwritten on call
		m_size = m_initial_size;

		DWORD dwRes = getExtendedTable(m_table, &m_size, TRUE, AddressFamily, tableClass, 0);

		if (dwRes == ERROR_INSUFFICIENT_BUFFER) {
			free_table();
			alloc_table();

			dwRes = getExtendedTable(m_table, &m_size, TRUE, AddressFamily, tableClass, 0);

			if (dwRes != NO_ERROR) {
				free_table();
				return dwRes;
			}

			// if new size of table is more than initial size, that was allocated on creation, store this size for further calls
			m_initial_size = m_size;
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

	T* m_table{ nullptr };
	// Pre-allocate page, so no need to re-allocate if not enough memory -> the less allocations the better
	DWORD m_initial_size{ 4096 };
	DWORD m_size{ m_initial_size };
};

template<>
template<typename TableClass>
DWORD ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>::update(TableClass tableClass) {
	DWORD dwRes = _update_table(GetExtendedTcpTable, tableClass);

	return dwRes;
}

template<>
template<typename TableClass>
DWORD ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>::update(TableClass tableClass) {
	DWORD dwRes = _update_table(GetExtendedTcpTable, tableClass, AF_INET6);

	return dwRes;
}

template<>
template<typename TableClass>
DWORD ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>::update(TableClass tableClass) {
	DWORD dwRes = _update_table(GetExtendedUdpTable, tableClass);

	return dwRes;
}

template<>
template<typename TableClass>
DWORD ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>::update(TableClass tableClass) {
	DWORD dwRes = _update_table(GetExtendedUdpTable, tableClass, AF_INET6);

	return dwRes;
}

using TcpTable4 = ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID>;
using TcpTable6 = ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID>;
using UdpTable4 = ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID>;
using UdpTable6 = ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID>;