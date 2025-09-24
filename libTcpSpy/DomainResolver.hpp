#ifndef DOMAIN_RESOLVER_HPP
#define DOMAIN_RESOLVER_HPP

#include <string>
#include <optional>
#include <thread>
#include <functional>

#include "Cache.hpp"
#include "ThreadPool.hpp"

class DomainResolver {
public:
	std::optional<std::wstring> resolve_domain(
		IPAddress addr,
		ProtocolFamily af,
		std::function<void(std::wstring &)> func)
	{
		std::wstring addr_str;
		switch (af) {
		case ProtocolFamily::INET: 
			addr_str = Net::ConvertAddrToStr(std::get<IP4Address>(addr));
			break;
		case ProtocolFamily::INET6:
			addr_str = Net::ConvertAddrToStr(std::get<IP6Address>(addr).data());
			break;
		}

		std::optional<std::wstring> cached_domain = m_domain_cache.get(addr_str);

		if (!cached_domain) {
			// capture by value because thread will obviously outlive stack variables
			m_thread_pool.submit([this, addr, addr_str, af, func]() {
				std::wstring domain;
				switch (af) {
				case ProtocolFamily::INET:
					domain = Net::ResolveAddrToDomainName(std::get<IP4Address>(addr));
					break;
				case ProtocolFamily::INET6:
					domain = Net::ResolveAddrToDomainName(std::get<IP6Address>(addr).data());
					break;
				}

				this->m_domain_cache.set(addr_str, domain);
				func(domain); // call callback with resolved domain

				});

			return std::nullopt;
		}
		else {
			func(*cached_domain);

			return *cached_domain;
		}
	}

	~DomainResolver() {
		m_thread_pool.stop();
	}
private:
	Cache<std::wstring, std::wstring> m_domain_cache{};
	ThreadPool m_thread_pool{ 5 };
};

#endif