#ifndef COLUMN_HPP
#define COLUMN_HPP

enum class Column {
	ProcessName,
	PID,
	Protocol,
	INET,
	LocalAddress,
	LocalPort,
	RemoteAddress,
	RemotePort,
	State,
	Count,
};

using SortBy = Column;
using SearchBy = Column;

#endif