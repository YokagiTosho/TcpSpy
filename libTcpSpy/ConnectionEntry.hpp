#pragma once

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

#include "Net.hpp"


class Process {
public:
	Process() {}

	Process(DWORD pid)
		: m_pid(pid)
	{}

	Process(const Process& p) = delete;

	Process(Process&& p) noexcept
		: m_icon(p.m_icon), m_path(std::move(p.m_path)), m_pid(p.m_pid)
	{

	}

	void open() {
		HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_pid);

		if (hProc == NULL) {
			HICON icon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
			return;
		}

		m_path = get_proc_path(hProc);

		m_name = extract_exe_name(m_path);
		
		m_icon = get_proc_icon(m_path);

		CloseHandle(hProc);
	}


private:
	std::wstring get_proc_path(HANDLE hProc) {
		constexpr int proc_name_len = 512;

		WCHAR wProcName[proc_name_len];

		if (!GetModuleFileNameEx(hProc, NULL, wProcName, proc_name_len)) {
			return L"Can not obtain ownership information";
		}
		 
		return std::wstring(wProcName);
	}

	HICON get_proc_icon(std::wstring lpszFile) {
		HICON icon = NULL;

		UINT res = ExtractIconEx(lpszFile.c_str(), 0, &icon, NULL, 1);

		if (res == UINT_MAX) {
			icon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
			return icon;
		}

		return icon;
	}

	std::wstring extract_exe_name(const std::wstring &path) {
		LPCWSTR wProcPath = path.c_str();

		for (size_t i = path.size() -1; i; i--) {
			if (wProcPath[i] == '\\') {
				return std::wstring(wProcPath + i + 1);
			}
		}

		return path;
	};

	DWORD m_pid{ (DWORD)-1};
	HICON m_icon{ nullptr };
	std::wstring m_path{ L"Can not obtain ownership information" };
	std::wstring m_name{ L"Can not obtain ownership information" };

	friend class ConnectionEntry;
};

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

inline IPAddress make_addr(DWORD addr) {
	return {addr};
}

inline IPAddress makeIP6Address(const UCHAR *addr) {
	IP6Address _addr{};
	std::copy(addr, addr + IP6AddressLen, _addr.begin());
	return _addr;
}

class ConnectionEntry
{
public:
	ConnectionEntry()
	{}

	ConnectionEntry(
		IPAddress local_addr,
		DWORD local_port,
		ConnectionProtocol proto,
		ProtocolFamily af,
		Process&& proc
	)
		: m_local_addr(local_addr)
		, m_local_port(ntohs(local_port))
		, m_proto(proto)
		, m_af(af)
		, m_proc(std::move(proc))
	{
		m_proc.open();
	}

	IPAddress local_addr() const { return m_local_addr; }

	DWORD local_port() const { return m_local_port; }

	ProtocolFamily address_family() const { return m_af; }

	ConnectionProtocol protocol() const { return m_proto; }

	virtual const std::wstring local_addr_str() const = 0;

	const std::wstring local_port_str() const {
		return Net::ConvertPortToStr(m_local_port);
	}

	const std::wstring &get_process_name() const {
		return m_proc.m_name;
	}
protected:
	Process m_proc;

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
			Process &&proc
			)
		: ConnectionEntry(row.dwLocalAddr, row.dwLocalPort, proto, af, std::forward<Process>(proc))
		, m_remote_addr(row.dwRemoteAddr), m_remote_port(ntohs(row.dwRemotePort)), m_state(row.dwState)
	{
		
	}

	ConnectionEntryTCP(
			const MIB_TCP6ROW_OWNER_PID& row,
			ConnectionProtocol proto,
			ProtocolFamily af,
			Process &&proc
			)
		: ConnectionEntry(::makeIP6Address(row.ucLocalAddr), row.dwLocalPort, proto, af, std::forward<Process>(proc))
		, m_remote_addr(::makeIP6Address(row.ucRemoteAddr)), m_remote_port(row.dwRemotePort), m_state(row.dwState)
	{ }


	IPAddress remote_addr() const {
		if (std::holds_alternative<IP4Address>(m_remote_addr)) {
			return std::get<IP4Address>(m_remote_addr);
		}
		return std::get<IP6Address>(m_remote_addr);
	}

	DWORD remote_port() const { return m_remote_port; }

	DWORD state() const { return m_state; }

	virtual const std::wstring remote_addr_str() const = 0;

	const std::wstring remote_port_str() const {
		return Net::ConvertPortToStr(m_remote_port);
	}

protected:
	IPAddress m_remote_addr{ (DWORD)-1 };
	DWORD m_remote_port{ (DWORD)-1 };

	DWORD m_state{ (DWORD)-1 };
};

class ConnectionEntry4TCP : public ConnectionEntryTCP {
public:
	ConnectionEntry4TCP(const MIB_TCPROW_OWNER_PID& row, Process&& proc)
		: ConnectionEntryTCP(
				row,
				ConnectionProtocol::PROTO_TCP,
				ProtocolFamily::INET,
				std::forward<Process>(proc))
	{}

	const std::wstring local_addr_str() const override
	{
		return Net::IPv4::ConvertAddrToStr(std::get<IP4Address>(m_local_addr));
	}

	const std::wstring remote_addr_str() const override
	{
		return Net::IPv4::ConvertAddrToStr(std::get<IP4Address>(m_remote_addr));
	}
private:
};

class ConnectionEntry4UDP : public ConnectionEntry {
public:
	ConnectionEntry4UDP(const MIB_UDPROW_OWNER_PID& row, Process&& proc)
		: ConnectionEntry(
				row.dwLocalPort,
				row.dwLocalAddr,
				ConnectionProtocol::PROTO_UDP,
				ProtocolFamily::INET,
				std::forward<Process>(proc))
	{}

private:
};

class ConnectionEntry6 {
public:
	ConnectionEntry6(DWORD local_scope_id)
		: m_local_scope_id(local_scope_id)
	{}
	
	DWORD local_scope_id() const { return m_local_scope_id; }
private:
	DWORD m_local_scope_id{ (DWORD)-1 };
};

class ConnectionEntry6TCP : public ConnectionEntry6, public ConnectionEntryTCP {
public:
	ConnectionEntry6TCP(const MIB_TCP6ROW_OWNER_PID& row, Process&& proc)
		: ConnectionEntry6(row.dwLocalScopeId)
		, ConnectionEntryTCP(
				row,
				ConnectionProtocol::PROTO_TCP,
				ProtocolFamily::INET6,
				std::forward<Process>(proc))
		, m_remote_scope_id(row.dwRemoteScopeId)
	{}

	const std::wstring local_addr_str() const override
	{
		return Net::IPv6::ConvertAddrToStr(std::get<IP6Address>(m_local_addr).data());
	}

	const std::wstring remote_addr_str() const override
	{
		return Net::IPv6::ConvertAddrToStr(std::get<IP6Address>(m_remote_addr).data());
	}
private:
	DWORD m_remote_scope_id{ (DWORD)-1 };
};

class ConnectionEntry6UDP : public ConnectionEntry6, public ConnectionEntry {
public:
	ConnectionEntry6UDP(const MIB_UDP6ROW_OWNER_PID& row, Process&& proc)
		: ConnectionEntry6(row.dwLocalScopeId)
		, ConnectionEntry(
				::makeIP6Address(row.ucLocalAddr),
				row.dwLocalPort,
				ConnectionProtocol::PROTO_UDP,
				ProtocolFamily::INET6,
				std::forward<Process>(proc))
	{}

	const std::wstring local_addr_str() const override
	{
		return Net::IPv6::ConvertAddrToStr(std::get<IP6Address>(m_local_addr).data());
	}
};



