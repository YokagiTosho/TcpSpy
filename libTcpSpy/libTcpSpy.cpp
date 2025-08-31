#include <iostream>
#include <vector>
#include <memory>

#include "ConnectionEntry.hpp"
#include "ConnectionsTable.hpp"

void test_ConnectionsTable();

int main()
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);

    if (WSAStartup(wVersionRequested, &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        exit(1);
    }

    test_ConnectionsTable();

    WSACleanup();
    return 0;
}

template<typename T>
T create_table() {
    T table{};

    table.update();

    return table;
}

void test_ConnectionsTable() {
#if 0
    std::vector<ConnectionEntry*> rows;

    TcpTable4 tcp_table4 = create_table<TcpTable4>();
    TcpTable6 tcp_table6 = create_table<TcpTable6>();
    UdpTable4 udp_table4 = create_table<UdpTable4>();
    UdpTable6 udp_table6 = create_table<UdpTable6>();

    add_rows(rows, tcp_table4);
    add_rows(rows, tcp_table6);
    add_rows(rows, udp_table4);
    add_rows(rows, udp_table6);
    
    std::cout << rows.size() << std::endl;

    for (auto& row : rows) {
        if (ConnectionEntryTCP* tcp = dynamic_cast<ConnectionEntryTCP*>(row)) {
            std::wcout << row->get_process_name() << "\t" << row->local_addr_str() << "\t" << row->local_port_str() << "\t" << tcp->remote_addr_str() << std::endl;
        }
        else {
            std::wcout << row->get_process_name() << "\t" << row->local_addr_str() << "\t" << row->local_port_str() << std::endl;
        }
    }

    for (int i = 0; i < rows.size(); i++) {
        delete rows[i];
    }
#endif
}
