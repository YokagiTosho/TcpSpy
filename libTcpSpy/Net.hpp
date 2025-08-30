#ifndef NET_HPP
#define NET_HPP

#include <string>
#include <stdexcept>
#include <sstream>

#include <ws2tcpip.h>

#include "Utils.hpp"

inline static std::wstring _SockaddrToWString(LPSOCKADDR sock_addr, DWORD len) {
	WCHAR buf[64];
	DWORD buf_size = 64;

	INT res = WSAAddressToStringW(sock_addr, len, NULL, buf, &buf_size);

	if (res == SOCKET_ERROR) {
		std::cerr << "`WSAAddressToStringW` failed: " << WSAGetLastError() << std::endl;
		exit(1);
	}

	return { buf };
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
		sockaddr_in6 s_in{};
		s_in.sin6_family = AF_INET6;
		memcpy(s_in.sin6_addr.u.Byte, a, 16);

		return ::_SockaddrToWString((SOCKADDR*)&s_in, sizeof(s_in));
	}
}

#endif
