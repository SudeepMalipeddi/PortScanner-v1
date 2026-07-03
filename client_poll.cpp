// Non-blocking port scanner using poll()
// Same batch-scanning approach as the select() version, but poll() has no
// FD_SETSIZE limit — you can fire thousands of sockets per batch.
//
// Usage: ./client_poll <ip> [start_port] [end_port] [batch_size] [timeout_ms]
// Example: ./client_poll 192.168.1.1 1 1024 2048 300

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <cerrno>

int main(int argc, char *argv[])
{
    // --- Parse arguments ---
    std::string ip_addr;
    int start_port = 1;
    int end_port = 65535;
    int batch_size = 4096;   // poll() can handle way more than select()'s 1024
    int timeout_ms = 500;

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

    // --- Set up target address ---
    sockaddr_in target{};
    target.sin_family = AF_INET;

    int status = inet_pton(AF_INET, ip_addr.c_str(), &target.sin_addr);
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

    // --- Poll-based batch loop ---
    for (int base = start_port; base <= end_port; base += batch_size)
    {
        int n = std::min(batch_size, end_port - base + 1);

        // pollfd array: one entry per socket we're watching
        std::vector<struct pollfd> pfds(n);
        std::vector<int>           ports(n);

        int active = 0;   // sockets that returned EINPROGRESS

        // ---- Phase 1: fire all non-blocking connects ----
        for (int i = 0; i < n; i++)
        {
            int port = base + i;
            ports[i] = port;

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                pfds[i].fd = -1;
                continue;
            }

            // Set O_NONBLOCK
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);

            target.sin_port = htons(port);
            int ret = connect(sock, (sockaddr*)&target, sizeof(target));

            if (ret == 0) {
                // Connected immediately (localhost, LAN)
                std::cout << "[+] Port " << port << " is OPEN\n";
                open_count++;
                close(sock);
                pfds[i].fd = -1;
            }
            else if (errno == EINPROGRESS) {
                // Connection in flight — register with poll()
                pfds[i].fd     = sock;
                pfds[i].events = POLLOUT;   // we want to know when writable
                pfds[i].revents = 0;
                active++;
            }
            else {
                // Immediate failure
                close(sock);
                pfds[i].fd = -1;
            }
        }

        scanned += n;
        std::cerr << "\rProgress: " << scanned << "/" << total << " ports scanned..."
                  << std::flush;

        if (active == 0) continue;

        // ---- Phase 2: poll() on the whole batch ----
        int ready = poll(pfds.data(), n, timeout_ms);

        // ---- Phase 3: interpret results ----
        for (int i = 0; i < n; i++)
        {
            if (pfds[i].fd < 0) continue;

            int sock = pfds[i].fd;
            bool is_open = false;

            if (ready > 0 && (pfds[i].revents & POLLOUT)) {
                // Socket became writable — verify with SO_ERROR
                int error = 0;
                socklen_t len = sizeof(error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len);

                if (error == 0) {
                    is_open = true;
                }
                // If SO_ERROR != 0, connection was refused / unreachable
            }
            else if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                // Explicit error — definitely closed
            }
            // else: poll() timed out for this socket → closed/filtered

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
