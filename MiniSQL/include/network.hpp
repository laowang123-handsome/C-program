#ifndef MINISQL_NETWORK_HPP
#define MINISQL_NETWORK_HPP

#include <string>

class TcpServer {
private:
    int port_;
    std::string data_root_;

public:
    TcpServer(int port, const std::string& data_root);
    int run();
};

class TcpClient {
private:
    std::string host_;
    int port_;

public:
    TcpClient(const std::string& host, int port);
    int runCli();
};

#endif
