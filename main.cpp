// 1 ) take ip address as input from user in CLI

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{

    std::string ip_addr;
    if (argc >= 2)
    {
        ip_addr = argv[1];
    }
    else
    {
        std::cout << "Enter the IP address: ";
        std::cin >> ip_addr;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;

    int status;
    status = inet_pton(AF_INET, ip_addr.c_str(), &serverAddress.sin_addr);
    if (status <= 0)
    {
        if (status == 0)
        {
            std::cerr << "Error: Invalid IP address format.\n";
        }
        else
        {
            std::cerr << "Error: inet_pton system failure.\n";
        }
        return 1;
    }

    for (int i = 1; i <= 65535; i++)
    {
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (clientSocket < 0)
        {
            std::cerr << "error: socket creation failed\n"
                      << std::endl;
            continue;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 200000;
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        serverAddress.sin_port = htons(i);

        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0)
        {
            std::cout << "[+] Port " << i << " is OPEN\n";
        }
        close(clientSocket);

        if (i % 1000 == 0)
        {
            std::cerr << "\rScanning port " << i << "/65535..." << std::flush;
        }
    }
    std::cerr << "\rScanning port 65535/65535 done \n"
              << std::flush;
}
