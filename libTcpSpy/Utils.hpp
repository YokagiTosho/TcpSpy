#ifndef UTILS_HPP
#define UTILS_HPP
#include <string>
#include <sstream>
#include "windef.h"

namespace Utils {
	template<typename From, typename To = std::wstring>
	inline To ConvertFrom(From from) {
		std::wstringstream ss;
		To to{};

		ss << from;

		ss >> to;

		return to;
	}
}

#endif