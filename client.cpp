// Non-blocking port scanner using select()
// Fires batches of simultaneous connect() calls, then uses select()
// to efficiently wait for responses with a configurable timeout.
//
// Usage: ./client <ip> [start_port] [end_port] [batch_size] [timeout_ms]
// Example: ./client 192.168.1.1 1 1024 256 500

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cerrno>

int main(int argc, char *argv[])
{
    // --- Parse arguments ---
    std::string ip_addr;
    int start_port = 1;
    int end_port = 65535;
    int batch_size = 256;   // sockets fired simultaneously (max: FD_SETSIZE - 1)
    int timeout_ms = 500;   // how long to wait per batch

    if (argc < 2) {
        std::cout << "Enter the IP address: ";
        std::cin >> ip_addr;
    } else {
        ip_addr = argv[1];
    }
    if (argc >= 3) start_port = std::stoi(argv[2]);
    if (argc >= 4) end_port   = std::stoi(argv[3]);
    if (argc >= 5) batch_size = std::stoi(argv[4]);
    if (argc >= 6) timeout_ms = std::stoi(argv[5]);

    if (start_port < 1 || end_port > 65535 || start_port > end_port) {
        std::cerr << "Error: port range must be 1-65535 with start <= end.\n";
        return 1;
    }
    if (batch_size < 1 || batch_size > FD_SETSIZE - 1) {
        std::cerr << "Error: batch size must be 1-" << (FD_SETSIZE - 1) << ".\n";
        return 1;
    }

    // --- Set up target address ---
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;

    int status = inet_pton(AF_INET, ip_addr.c_str(), &serverAddress.sin_addr);
    if (status <= 0) {
        std::cerr << (status == 0
            ? "Error: Invalid IP address format.\n"
            : "Error: inet_pton system failure.\n");
        return 1;
    }

    int total = end_port - start_port + 1;
    std::cout << "Scanning " << ip_addr << "  [ports " << start_port
              << "-" << end_port << "]  batch=" << batch_size
              << "  timeout=" << timeout_ms << "ms\n\n";

    int open_count = 0;
    int scanned = 0;

    // --- Batch loop ---
    for (int base = start_port; base <= end_port; base += batch_size)
    {
        int n = std::min(batch_size, end_port - base + 1);

        std::vector<int> sockets(n);
        std::vector<int> ports(n);

        fd_set write_fds;
        FD_ZERO(&write_fds);
        int max_fd = -1;
        int active = 0;   // count of sockets waiting on EINPROGRESS

        // ---- Phase 1: fire all non-blocking connects ----
        for (int i = 0; i < n; i++)
        {
            int port = base + i;
            ports[i] = port;

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                sockets[i] = -1;
                continue;
            }
            sockets[i] = sock;

            // Set O_NONBLOCK so connect() returns immediately
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);

            serverAddress.sin_port = htons(port);
            int ret = connect(sock, (sockaddr*)&serverAddress, sizeof(serverAddress));

            if (ret == 0) {
                // Immediate connect — rare for remote, common for localhost
                std::cout << "[+] Port " << port << " is OPEN\n";
                open_count++;
                close(sock);
                sockets[i] = -1;
            }
            else if (errno == EINPROGRESS) {
                // Connection is in progress — add to select() watch list
                FD_SET(sock, &write_fds);
                if (sock > max_fd) max_fd = sock;
                active++;
            }
            else {
                // Immediate failure (port unreachable, connection refused, etc.)
                close(sock);
                sockets[i] = -1;
            }
        }

        // Progress indicator
        scanned += n;
        std::cerr << "\rProgress: " << scanned << "/" << total << " ports scanned..."
                  << std::flush;

        if (active == 0) continue;   // nothing left to wait for in this batch

        // ---- Phase 2: select() waits for the batch ----
        struct timeval tv;
        tv.tv_sec  = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ready = select(max_fd + 1, NULL, &write_fds, NULL, &tv);

        // ---- Phase 3: check results & clean up ----
        for (int i = 0; i < n; i++)
        {
            int sock = sockets[i];
            if (sock < 0) continue;   // already handled

            bool is_open = false;

            if (ready > 0 && FD_ISSET(sock, &write_fds)) {
                // select() says the socket is writable.
                // Verify with SO_ERROR — 0 means success, otherwise it's an error.
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

                if (error == 0) {
                    is_open = true;
                }
            }
            // else: select() timed out or this socket wasn't ready → closed/filtered

            if (is_open) {
                std::cout << "[+] Port " << ports[i] << " is OPEN\n";
                open_count++;
            }

            close(sock);
        }
    }

    std::cerr << "\rProgress: " << scanned << "/" << total << " ports scanned."
              << std::endl;
    std::cout << "\nDone. " << open_count << " open port(s) found.\n";

    return 0;
}
