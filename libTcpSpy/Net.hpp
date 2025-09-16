#ifndef NET_HPP
#define NET_HPP

#include <string>
#include <stdexcept>

#include <ws2tcpip.h>

#include "Utils.hpp"

inline static std::wstring _SockaddrToWString(LPSOCKADDR sock_addr, DWORD len) {
	WCHAR buf[64];
	DWORD buf_size = 64;

	INT res = WSAAddressToStringW(sock_addr, len, NULL, buf, &buf_size);

	if (res == SOCKET_ERROR) {
		auto err = WSAGetLastError();
		std::cerr << "`WSAAddressToStringW` failed: " << err << std::endl;
		exit(1);
	}

	return { buf };
}

static std::wstring _ResolveAddr(LPSOCKADDR sin, size_t sin_size) {
	WCHAR domain_name[NI_MAXHOST];

	int res = GetNameInfo(
		sin, sin_size,
		domain_name, NI_MAXHOST,
		NULL, 0,
		NI_NAMEREQD);

	if (res) {
		res = WSAGetLastError();
		if (res == WSAHOST_NOT_FOUND) {
			return {};
		}
	}

	return { domain_name };
}

namespace Net {
	inline std::wstring ConvertPortToStr(DWORD port) {
		return Utils::ConvertFrom<DWORD>(port);
	}

	inline std::wstring ConvertAddrToStr(DWORD a) {
		sockaddr_in sin{};
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = a;

		return ::_SockaddrToWString((SOCKADDR*)&sin, sizeof(sin));
	}

	inline std::wstring ConvertAddrToStr(const UCHAR a[]) {
		sockaddr_in6 sin{};
		sin.sin6_family = AF_INET6;
		memcpy(sin.sin6_addr.u.Byte, a, 16);

		return ::_SockaddrToWString((SOCKADDR*)&sin, sizeof(sin));
	}

	inline std::wstring ResolveAddrToDomainName(DWORD addr) {
		sockaddr_in sin{ };
		sin.sin_addr.S_un.S_addr = addr;
		sin.sin_family = AF_INET;
		
		return ::_ResolveAddr((LPSOCKADDR)&sin, sizeof(sin));
	}

	inline std::wstring ResolveAddrToDomainName(const UCHAR a[]) {
		sockaddr_in6 sin{ };
		sin.sin6_family = AF_INET6;
		memcpy(sin.sin6_addr.u.Byte, a, 16);

		return ::_ResolveAddr((LPSOCKADDR)&sin, sizeof(sin));
	}

	inline std::wstring ConvertPortToService(DWORD port, const char *proto) {
		servent *serv = getservbyport(htons(port), proto);
		if (!serv) {
			return Net::ConvertPortToStr(port);
		}
		return { serv->s_name, serv->s_name + strlen(serv->s_name) };
	}
}

#endif
