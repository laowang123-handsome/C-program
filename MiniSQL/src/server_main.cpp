#include "network.hpp"
#include <cstdlib>
#include <iostream>

int main(int argc, char** argv) {
    int port = 3307;
    std::string data_root = "data";
    if (argc >= 2) port = std::atoi(argv[1]);
    if (argc >= 3) data_root = argv[2];
    TcpServer server(port, data_root);
    return server.run();
}
