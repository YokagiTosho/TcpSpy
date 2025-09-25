#include "Consts.hpp"

const std::array<std::wstring, (int)Column::Count> COLUMNS{
	L"Process name", L"PID", L"Protocol",
	L"IP version", L"Local Address", L"Local Port",
	L"Remote Address", L"Remote Port", L"State",
};