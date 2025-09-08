#ifndef CONNECTION_ENTRY_HPP
#define CONNECTION_ENTRY_HPP

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include <ws2ipdef.h>
#include <tcpmib.h>
#include <psapi.h>

#include <string>
#include <variant>
#include <array>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <utility>
#include <vector>

#include "Net.hpp"
#include "Process.hpp"


enum class ConnectionProtocol {
	UNSET,
	PROTO_TCP,
	PROTO_UDP,
};

enum class ProtocolFamily {
	UNSET,
	INET = AF_INET,
	INET6 = AF_INET6,
};

constexpr size_t IP6AddressLen = 16;

using IP4Address = DWORD;
using IP6Address = std::array<UCHAR, IP6AddressLen>;

using IPAddress = std::variant<IP4Address, IP6Address>;

inline IPAddress makeIP6Address(const UCHAR* addr) {
	IP6Address _addr{};
	std::copy(addr, addr + IP6AddressLen, _addr.begin());
	return _addr;
}

class ConnectionEntry
{
public:
	ConnectionEntry()
	{
	}

	ConnectionEntry(
		IPAddress local_addr,
		DWORD local_port,
		ConnectionProtocol proto,
		ProtocolFamily af,
		ProcessPtr proc
	)
		: m_local_addr(local_addr)
		, m_local_port(ntohs(local_port))
		, m_proto(proto)
		, m_af(af)
		, m_proc(proc)
	{
	}

	IPAddress local_addr() const { return m_local_addr; }

	DWORD local_port() const { return m_local_port; }

	ProtocolFamily address_family() const { return m_af; }

	ConnectionProtocol protocol() const { return m_proto; }

	const std::wstring local_addr_str() const {
		return _get_addr_str(m_local_addr);
	}

	const std::wstring local_port_str() const {
		return Net::ConvertPortToStr(m_local_port);
	}

	const std::wstring& get_process_name() const {
		return m_proc->m_name;
	}

	std::wstring pid_str() const {
		return Utils::ConvertFrom<DWORD>(m_proc->m_pid);
	}

	std::wstring proto_str() const { return Utils::ConvertFrom<int>((int)m_proto); }

	HICON icon() const { return m_proc->m_icon; }

	DWORD pid() const { return m_proc->m_pid; }

	Process& proc() const { return *m_proc; }

	virtual ~ConnectionEntry() = default;
protected:
	const std::wstring _get_addr_str(IPAddress addr) const {
		switch (m_af) {
		case ProtocolFamily::INET:
			return Net::ConvertAddrToStr(std::get<IP4Address>(addr));
		case ProtocolFamily::INET6:
			return Net::ConvertAddrToStr(std::get<IP6Address>(addr).data());
		default:
			break;
		}
		assert(false);
	}
	ProcessPtr m_proc;

	ConnectionProtocol m_proto{ ConnectionProtocol::UNSET };
	ProtocolFamily m_af{ ProtocolFamily::UNSET };

	IPAddress m_local_addr{ (DWORD)-1 };
	DWORD m_local_port{ (DWORD)-1 };
};

/*
 * ConnectionEntryTCP used for TCP classes
 * With additional members like remote port, addr etc.
 */
class ConnectionEntryTCP : public ConnectionEntry {
public:
	ConnectionEntryTCP(
		const MIB_TCPROW_OWNER_PID& row,
		ConnectionProtocol proto,
		ProtocolFamily af,
		ProcessPtr proc
	)
		: ConnectionEntry(row.dwLocalAddr, row.dwLocalPort, proto, af, proc)
		, m_remote_addr(row.dwRemoteAddr), m_remote_port(ntohs(row.dwRemotePort)), m_state(row.dwState)
	{
	}

	ConnectionEntryTCP(
		const MIB_TCP6ROW_OWNER_PID& row,
		ConnectionProtocol proto,
		ProtocolFamily af,
		ProcessPtr proc
	)
		: ConnectionEntry(::makeIP6Address(row.ucLocalAddr), row.dwLocalPort, proto, af, proc)
		, m_remote_addr(::makeIP6Address(row.ucRemoteAddr)), m_remote_port(ntohs(row.dwRemotePort)), m_state(row.dwState)
	{
	}

	IPAddress remote_addr() const { return m_remote_addr; }

	DWORD remote_port() const { return m_remote_port; }

	DWORD state() const { return m_state; }

	const std::wstring remote_addr_str() const {
		return _get_addr_str(m_remote_addr);
	}

	const std::wstring remote_port_str() const {
		return Net::ConvertPortToStr(m_remote_port);
	}

	std::wstring state_str() const {
		switch (m_state) {
		case MIB_TCP_STATE_CLOSED:     return L"CLOSED";
		case MIB_TCP_STATE_LISTEN:     return L"LISTEN";
		case MIB_TCP_STATE_SYN_SENT:   return L"SYN_SENT";
		case MIB_TCP_STATE_SYN_RCVD:   return L"SYN_RCVD";
		case MIB_TCP_STATE_ESTAB:      return L"ESTAB";
		case MIB_TCP_STATE_FIN_WAIT1:  return L"FIN_WAIT1";
		case MIB_TCP_STATE_FIN_WAIT2:  return L"FIN_WAIT2";
		case MIB_TCP_STATE_CLOSE_WAIT: return L"CLOSE_WAIT";
		case MIB_TCP_STATE_CLOSING:    return L"CLOSING";
		case MIB_TCP_STATE_LAST_ACK:   return L"LAST_ACK";
		case MIB_TCP_STATE_TIME_WAIT:  return L"TIME_WAIT";
		case MIB_TCP_STATE_DELETE_TCB: return L"DELETE_TCB";
		}
		return L"";
	}

	virtual ~ConnectionEntryTCP() = default;
protected:
	IPAddress m_remote_addr{ (DWORD)-1 };
	DWORD m_remote_port{ (DWORD)-1 };

	DWORD m_state{ (DWORD)-1 };
};

class ConnectionEntry4TCP : public ConnectionEntryTCP {
public:
	ConnectionEntry4TCP(const MIB_TCPROW_OWNER_PID& row, ProcessPtr proc)
		: ConnectionEntryTCP(
			row,
			ConnectionProtocol::PROTO_TCP,
			ProtocolFamily::INET,
			proc)
	{
	}
};

class ConnectionEntry4UDP : public ConnectionEntry {
public:
	ConnectionEntry4UDP(const MIB_UDPROW_OWNER_PID& row, ProcessPtr proc)
		: ConnectionEntry(
			row.dwLocalAddr,
			row.dwLocalPort,
			ConnectionProtocol::PROTO_UDP,
			ProtocolFamily::INET,
			proc)
	{
	}
};

class ConnectionEntry6 {
public:
	ConnectionEntry6(DWORD local_scope_id)
		: m_local_scope_id(local_scope_id)
	{
	}

	DWORD local_scope_id() const { return m_local_scope_id; }

	virtual ~ConnectionEntry6() = default;
private:
	DWORD m_local_scope_id{ (DWORD)-1 };
};

class ConnectionEntry6TCP : public ConnectionEntry6, public ConnectionEntryTCP {
public:
	ConnectionEntry6TCP(const MIB_TCP6ROW_OWNER_PID& row, ProcessPtr proc)
		: ConnectionEntry6(row.dwLocalScopeId)
		, ConnectionEntryTCP(
			row,
			ConnectionProtocol::PROTO_TCP,
			ProtocolFamily::INET6,
			proc)
		, m_remote_scope_id(row.dwRemoteScopeId)
	{
	}

	DWORD remote_scope_id() const { return m_remote_scope_id; }
private:
	DWORD m_remote_scope_id{ (DWORD)-1 };
};

class ConnectionEntry6UDP : public ConnectionEntry6, public ConnectionEntry {
public:
	ConnectionEntry6UDP(const MIB_UDP6ROW_OWNER_PID& row, ProcessPtr proc)
		: ConnectionEntry6(row.dwLocalScopeId)
		, ConnectionEntry(
			::makeIP6Address(row.ucLocalAddr),
			row.dwLocalPort,
			ConnectionProtocol::PROTO_UDP,
			ProtocolFamily::INET6,
			proc)
	{
	}
};

using ConnectionEntryPtr = std::unique_ptr<ConnectionEntry>;
using ConnectionEntryPtrs = std::vector<ConnectionEntryPtr>;

#endif
