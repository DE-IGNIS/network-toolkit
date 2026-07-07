#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

std::mutex mtx;

void printError(const char *msg)
{
    std::cerr << msg << " failed. Error code: " << WSAGetLastError() << std::endl;
}

void scanPort(std::string input_ip, int startPort, int endPort, bool &anyOpen)
{
    for (int port = startPort; port <= endPort; port++)
    {
        SOCKET scan_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (scan_socket == INVALID_SOCKET)
        {
            std::lock_guard<std::mutex> lock(mtx);
            printError("socket");
            return;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        if (inet_pton(AF_INET, input_ip.c_str(), &address.sin_addr) <= 0)
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cerr << "Invalid IP address format.\n";
            closesocket(scan_socket);
            return;
        }

        u_long mode = 1;
        ioctlsocket(scan_socket, FIONBIO, &mode);

        int conn = connect(scan_socket, (sockaddr *)&address, sizeof(address));
        int err = WSAGetLastError();

        bool isOpen = false;

        if (conn == 0)
        {
            isOpen = true;
        }
        else if (err == WSAEWOULDBLOCK || err == WSAEINPROGRESS)
        {
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(scan_socket, &write_fds);

            timeval timeout{};
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int result = select(0, nullptr, &write_fds, nullptr, &timeout);

            if (result > 0)
            {
                int so_error = 0;
                int len = sizeof(so_error);
                getsockopt(scan_socket, SOL_SOCKET, SO_ERROR,
                           (char *)&so_error, &len);

                if (so_error == 0)
                {
                    isOpen = true;
                }
            }
        }

        if (isOpen)
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "Port " << port << " is OPEN\n";
            anyOpen = true;
        }

        closesocket(scan_socket);
    }
}

int main()
{
    std::string input_ip;
    std::cout << "Enter an ip address: ";
    std::getline(std::cin, input_ip);

    int range;
    std::cout << "Enter a port range (1 to N): ";
    std::cin >> range;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printError("WSAStartup");
        return 1;
    }
    std::cout << "\nScanning " << input_ip << " from port 1 to " << range << "...\n\n";

    bool anyOpen = false;

    const int NUM_THREADS = 100;

    std::vector<std::thread> threads;

    int portsPerThread = range / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        int startPort = i * portsPerThread + 1;
        int endPort = (i == NUM_THREADS - 1) ? range : startPort + portsPerThread - 1;

        threads.emplace_back(scanPort, input_ip, startPort, endPort, std::ref(anyOpen));
    }

    for (auto &t : threads)
    {
        t.join();
    }

    if (!anyOpen)
    {
        std::cout << "No open ports found in range 1 to " << range << ".\n";
    }

    std::cout << "\nScan Complete.\n";

    WSACleanup();
    return 0;
}
