#include <iostream>
#include <windows.h>
#include <iphlpapi.h>
#include <iomanip>
#include <thread>
#include <chrono>

#pragma comment(lib, "iphlpapi.lib")

MIB_IFTABLE *getIfTable()
{
    DWORD size = 0;
    GetIfTable(nullptr, &size, FALSE);

    MIB_IFTABLE *table = (MIB_IFTABLE *)malloc(size);

    if (GetIfTable(table, &size, FALSE) != NO_ERROR)
    {
        free(table);
        return nullptr;
    }

    return table;
}

int selectInterface()
{
    MIB_IFTABLE *table = getIfTable();

    if (!table)
    {
        std::cerr << "Failed to get interfaces.\n";
        return -1;
    }

    std::cout << "\n===== AVAILABLE INTERFACES =====\n\n";

    for (DWORD i = 0; i < table->dwNumEntries; i++)
    {
        MIB_IFROW &row = table->table[i];

        if (row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL && (row.dwType == 6 || row.dwType == 71))
        {

            std::cout << "[" << row.dwIndex << "] ";

            for (DWORD j = 0; j < row.dwDescrLen; j++)
            {
                std::cout << (char)row.bDescr[j];
            }
            std::cout << "\n";
        }
    }

    free(table);

    std::cout << "\nEnter interface number: ";
    int choice;
    std::cin >> choice;

    return choice;
}

bool getInterfaceBytes(int index, DWORD &inBytes, DWORD &outBytes)
{
    MIB_IFTABLE *table = getIfTable();

    if (!table)
        return false;

    for (DWORD i = 0; i < table->dwNumEntries; i++)
    {
        if (table->table[i].dwIndex == (DWORD)index)
        {
            inBytes = table->table[i].dwInOctets;
            outBytes = table->table[i].dwOutOctets;
            free(table);
            return true;
        }
    }

    free(table);
    return false;
}

std::string formatSpeed(double bytesPerSec)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (bytesPerSec >= 1048576) // MB/s
    {
        oss << (bytesPerSec / 1048576.0) << " MB/s";
    }
    else if (bytesPerSec >= 1024) // KB/s
    {
        oss << (bytesPerSec / 1024.0) << " KB/s";
    }
    else
    {
        oss << bytesPerSec << " B/s";
    }

    return oss.str();
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "       SIMPLE BANDWIDTH MONITOR         \n";
    std::cout << "========================================\n";

    int interfaceIndex = selectInterface();

    if (interfaceIndex < 0)
    {
        return 1;
    }

    DWORD prevIn = 0, prevOut = 0;

    if (!getInterfaceBytes(interfaceIndex, prevIn, prevOut))
    {
        std::cerr << "Failed to read interface.\n";
        return 1;
    }

    std::cout << "\n========================================\n";
    std::cout << "   Monitoring... Press Ctrl+C to stop   \n";
    std::cout << "========================================\n\n";

    DWORD totalDownloaded = 0;
    DWORD totalUploaded = 0;

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        DWORD currIn = 0, currOut = 0;

        if (!getInterfaceBytes(interfaceIndex, currIn, currOut))
        {
            std::cerr << "Failed to read interface.\n";
            continue;
        }

        DWORD downloadSpeed = currIn - prevIn;
        DWORD uploadSpeed = currOut - prevOut;

        totalDownloaded += downloadSpeed;
        totalUploaded += uploadSpeed;

        std::cout << "\r";
        std::cout << "DOWN: " << std::setw(12) << formatSpeed(downloadSpeed);
        std::cout << "  |  ";
        std::cout << "UP: " << std::setw(12) << formatSpeed(uploadSpeed);
        std::cout << "  |  ";
        std::cout << "Total: " << std::setw(10) << formatSpeed(totalDownloaded) << " / " << formatSpeed(totalUploaded);
        std::cout << std::flush;

        prevIn = currIn;
        prevOut = currOut;
    }

    return 0;
}