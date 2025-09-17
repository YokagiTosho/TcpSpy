#ifndef FIND_DLG_HPP
#define FIND_DLG_HPP

#include "resource.h"
#include "framework.h"

#include "libTcpSpy/Column.hpp"

#include <array>
#include <string>
#include <functional>

HWND InitFindDialog(HWND hWnd, const std::array<std::wstring, (int)Column::Count>& columns);

#endif