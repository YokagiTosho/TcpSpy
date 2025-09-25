#ifndef FILE_SAVER_CSV_HPP
#define FILE_SAVER_CSV_HPP

#include "framework.h"

#include "libTcpSpy/FileSaver.hpp"
#include "libTcpSpy/ConnectionsTableManager.hpp"

#include "Consts.hpp"

class FileSaverCSV : public FileSaver<ConnectionsTableManager> {
public:
	FileSaverCSV(LPCWSTR filePath)
		: FileSaver(filePath)
	{
	}

	bool save(const ConnectionsTableManager& mgr) override {

		int i = 0;
		for (; i < COLUMNS.size(); i++) {
			m_file << COLUMNS[i];
			if (i != COLUMNS.size() - 1) {
				m_file << ";";
			}
			else {
				m_file << std::endl;
			}
		}

		for (const auto& row : mgr)
		{
			m_file << row->get_process_name()
				<< ";"
				<< row->pid_str()
				<< ";"
				<< row->proto_str()
				<< ";"
				<< row->address_family_str()
				<< ";"
				<< row->local_addr_str()
				<< ";"
				<< row->local_port_str();

			if (row->protocol() == ConnectionProtocol::PROTO_TCP) {
				ConnectionEntryTCP* tcp_row = nullptr;
				if (tcp_row = dynamic_cast<ConnectionEntryTCP*>(row.get())) {
					m_file << ";";
					m_file << tcp_row->remote_addr_str()
						<< ";"
						<< tcp_row->remote_port_str()
						<< ";"
						<< tcp_row->state_str();
				}
				else {
					return false;
				}
			}

			m_file << std::endl;
		}

		return true;
	}
};

#endif