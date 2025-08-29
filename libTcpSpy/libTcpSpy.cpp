// libTcpSpy.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <memory>
#include "ConnectionEntry.hpp"
#include "ConnectionsTable.hpp"

template<typename Table>
bool add_rows(const Table &table, ConnectionProtocol proto, ProtocolFamily af) {
    std::vector<std::unique_ptr<ConnectionEntry>> rows;
    std::wstring process_name;


    for (DWORD i = 0; i < table.dwNumEntries; i++) {
        Process process(table.table[i].dwOwningPid);

        if (proto == ConnectionProtocol::PROTO_TCP && af == ProtocolFamily::INET) {
            rows.push_back(std::make_shared<ConnectionEntry4TCP>(std::move(process)));
        }
        else if (proto == ConnectionProtocol::PROTO_UDP && af == ProtocolFamily::INET) {
            rows.push_back(std::make_shared<ConnectionEntry4UDP>(std::move(process)));
        }
        else if (proto == ConnectionProtocol::PROTO_TCP && af == ProtocolFamily::INET6) {
            rows.push_back(std::make_shared<ConnectionEntry6TCP>(std::move(process)));
        }
        else if (proto == ConnectionProtocol::PROTO_UDP && af == ProtocolFamily::INET6) {
            rows.push_back(std::make_shared<ConnectionEntry6UDP>(std::move(process)));

        }

    }
}

void test_ConnectionEntry();
void test_ConnectionsTable();

int main()
{
    test_ConnectionsTable();


    return 0;
}

void test_ConnectionsTable() {
    ConnectionsTable<MIB_TCPTABLE_OWNER_PID, MIB_TCPROW_OWNER_PID> table;
    //ConnectionsTable<MIB_TCP6TABLE_OWNER_PID, MIB_TCP6ROW_OWNER_PID> table;
    //ConnectionsTable<MIB_UDPTABLE_OWNER_PID, MIB_UDPROW_OWNER_PID> table3;
    //ConnectionsTable<MIB_UDP6TABLE_OWNER_PID, MIB_UDP6ROW_OWNER_PID> table4;

    table.update_table();

    for (const auto& row : table) {
        auto ce = ConnectionEntry4TCP(row, Process(row.dwOwningPid));
        std::wcout << ce.get_process_name() << "\t" << ce.local_addr_str() << "\t" << ce.local_port_str() << "\t" << ce.remote_port_str() << std::endl;
    }

#if 0
    for (int i = 0; i < table.get_table().dwNumEntries; i++) {
        auto row = table.get_table().table[i];

        auto ce = ConnectionEntry4TCP(row, Process(row.dwOwningPid));

        std::wcout << ce.get_process_name() << "\t" << ce.local_port() << "\t" << ce.remote_port() << std::endl;
    }
#endif

    
}

void test_ConnectionEntry() {
    MIB_TCPTABLE_OWNER_PID* table = nullptr;
    ULONG size = sizeof(MIB_TCPTABLE_OWNER_PID);
    DWORD dwRes;

#if 1
    if (table == nullptr) {
        table = (MIB_TCPTABLE_OWNER_PID*)malloc(size);
    }
#endif

    dwRes = GetExtendedTcpTable((LPVOID)table, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);



    if (dwRes == ERROR_INSUFFICIENT_BUFFER) {
        free(table);
        table = (MIB_TCPTABLE_OWNER_PID*)malloc(size);

        dwRes = GetExtendedTcpTable((LPVOID)table, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);

        if (dwRes != NO_ERROR) {
            exit(2);
        }

        std::cout << dwRes << std::endl;
    }



    for (int i = 0; i < table->dwNumEntries; i++) {
        auto row = table->table[i];

        auto ce = ConnectionEntry4TCP(row, Process(row.dwOwningPid));

        std::wcout << ce.get_process_name() << "\t" << ce.local_port() << "\t" << ce.remote_port() << std::endl;
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
