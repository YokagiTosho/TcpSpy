#ifndef NET_HPP
#define NET_HPP

#include <string>
#include <stdexcept>
#include <sstream>

#include <ws2tcpip.h>

#include "Utils.hpp"


namespace Net {
	inline std::wstring ConvertPortToStr(DWORD port) {
		return Utils::ConvertFrom<DWORD>(port);
	}

	namespace IPv4 {
		inline std::wstring ConvertAddrToStr(DWORD a) {
			in_addr addr{};
			addr.S_un.S_addr = a;
#define STRIP4LEN 16
			CHAR buf[STRIP4LEN];

			PCSTR res = inet_ntop(AF_INET, (void*)&addr, buf, STRIP4LEN);
			if (res == nullptr) {
				throw std::runtime_error("Failed to cast addr to AF_INET IP address");
			}
			return Utils::ConvertFrom<CHAR[]>(buf);	
		}

		
	}

	namespace IPv6 {
#define STRIP6LEN 39

	}
}



#endif