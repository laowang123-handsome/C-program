#include "network.hpp"
#include <cstdlib>

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    int port = 3307;
    if (argc >= 2) host = argv[1];
    if (argc >= 3) port = std::atoi(argv[2]);
    TcpClient client(host, port);
    return client.runCli();
}
