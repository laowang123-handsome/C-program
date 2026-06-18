#include "network.hpp"
#include "executor.hpp"
#include "result_codec.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

static bool sendAll(int fd, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = send(fd, data.c_str() + sent, data.size() - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

static bool recvLine(int fd, std::string& line) {
    line.clear();
    char ch;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        if (n <= 0) return false;
        if (ch == '\n') return true;
        if (ch != '\r') line += ch;
    }
}

static bool recvUntilEnd(int fd, std::string& text) {
    text.clear();
    std::string line;
    while (recvLine(fd, line)) {
        text += line;
        text += '\n';
        if (line == "END") return true;
    }
    return false;
}

TcpServer::TcpServer(int port, const std::string& data_root) : port_(port), data_root_(data_root) {}

int TcpServer::run() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind failed\n";
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 8) < 0) {
        std::cerr << "listen failed\n";
        close(server_fd);
        return 1;
    }

    std::cout << "MiniSQL server listening on port " << port_ << std::endl;
    database db(data_root_);

    while (true) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client_fd < 0) {
            std::cerr << "accept failed\n";
            continue;
        }
        std::cout << "client connected\n";

        std::string line;
        while (recvLine(client_fd, line)) {
            QueryResult result = db.execute(line);
            sendAll(client_fd, ResultCodec::serialize(result));
            if (line == "exit" || line == "quit") break;
        }
        close(client_fd);
        std::cout << "client disconnected\n";
    }

    close(server_fd);
    return 0;
}

TcpClient::TcpClient(const std::string& host, int port) : host_(host), port_(port) {}

static void printBorder(const SimpleArray<int>& widths) {
    std::cout << '+';
    for (std::size_t i = 0; i < widths.size(); ++i) {
        for (int j = 0; j < widths[i] + 2; ++j) std::cout << '-';
        std::cout << '+';
    }
    std::cout << '\n';
}

static void printCell(const std::string& s, int width) {
    std::cout << "| " << s;
    int rest = width - static_cast<int>(s.size());
    for (int i = 0; i < rest + 1; ++i) std::cout << ' ';
}

static void printResult(const QueryResult& result) {
    if (!result.ok) {
        std::cout << result.message << std::endl;
        return;
    }
    if (result.headers.empty()) {
        std::cout << result.message << std::endl;
        return;
    }

    SimpleArray<int> widths;
    for (std::size_t c = 0; c < result.headers.size(); ++c) {
        widths.push_back(static_cast<int>(result.headers[c].size()));
    }
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        for (std::size_t c = 0; c < result.rows[r].values.size(); ++c) {
            int len = static_cast<int>(result.rows[r].values[c].size());
            if (len > widths[c]) widths[c] = len;
        }
    }

    printBorder(widths);
    for (std::size_t c = 0; c < result.headers.size(); ++c) {
        printCell(result.headers[c], widths[c]);
    }
    std::cout << "|\n";
    printBorder(widths);
    for (std::size_t r = 0; r < result.rows.size(); ++r) {
        for (std::size_t c = 0; c < result.rows[r].values.size(); ++c) {
            printCell(result.rows[r].values[c], widths[c]);
        }
        std::cout << "|\n";
    }
    printBorder(widths);
    std::cout << result.rows.size() << " row(s) in set" << std::endl;
}

int TcpClient::runCli() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "invalid host\n";
        close(fd);
        return 1;
    }

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "connect failed\n";
        close(fd);
        return 1;
    }

    std::cout << "Welcome to MiniSQL. Type 'exit' to quit.\n";
    std::string line;
    while (true) {
        std::cout << "mini-sql> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        sendAll(fd, line + "\n");
        std::string text;
        if (!recvUntilEnd(fd, text)) {
            std::cout << "server disconnected\n";
            break;
        }
        QueryResult result;
        std::string err;
        if (ResultCodec::deserialize(text, result, err)) {
            printResult(result);
        } else {
            std::cout << "bad response: " << err << std::endl;
        }
        if (line == "exit" || line == "quit") break;
    }
    close(fd);
    return 0;
}
