// 1 ) take ip address as input from user in CLI

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    // std::cout << "Number of arguments passed " << argc << '\n';
    // std::cout << "Arguments passed : ";
    // for (int i = 1; i < argc; i++)
    // {
    //     std::cout << argv[i] << " ";
    // }
    // std::cout << "\n";

    // if (argc < 2)
    // {
    //     std::cout << "Arguments passed not enough : ";
    //     std::cout << "Usage : " << argv[0] <<
    // }

    std::string ip_addr;
    std::cout << "Enter the ip address : ";
    std::cin >> ip_addr;

    // int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // if (clientSocket < 0)
    // {
    //     std::cerr << "error: socket address not valid\n"
    //               << std::endl;
    //     return 1;
    // }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;

    // serverAddress.sin_port = htons(10001);
    // serverAddress.sin_addr = ip_addr;

    // in_add
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
        // close(clientSocket);
        return 1;
    }

    // 17 - 25 port numbers

    for (int i = 0; i <= 1024; i++)
    {
        // create new client Socket for each port
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (clientSocket < 0)
        {
            std::cerr << "error: socket address not valid\n"
                      << std::endl;
            return 1;
        }

        // server ports
        serverAddress.sin_port = htons(i);

        if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        {
            std::cerr << "Error connecting on port " << i << '\n';
        }
        else
        {
            std::cout << "Port address " << i << " valid." << '\n';
        }
    }

    // if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    // {
    //     std::cerr << "Error connecting " << '\n';
    //     return 1;
    // }
    // else
    // {

    // for (;;)
    // {
    //     bzero(buff, sizeof(buff));

    //     std::cout << "Enter string " << '\n';

    //     n = 0;

    //     while ((buff[n++] = getchar()) != '\n')
    //         ;

    //     write(clientSocket, buff, sizeof(buff));

    //     bzero(buff, sizeof(buff));

    //     read(clientSocket, buff, sizeof(buff));

    //     printf("From Server : %s", buff);

    //     if (strncmp("exit", buff, 4) == 0)
    //     {
    //         printf("Server Exit...\n");
    //         break;
    //     }
    // }
    // }

    // for(int i = 0; i <= 1023; i++){

    // }
}
