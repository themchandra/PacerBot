#include "controller_server.h"
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

bool ControllerServer::start() { return openSocket(); }

void ControllerServer::stop() { closeSocket(); }

bool ControllerServer::openSocket()
{
    // Create the listening Unix Domain Socket
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
        std::perror("Socket creation failed");
        return false;
    }

    // Remove a stale socket file from a previous run
    ::unlink(socket_path_);

    // Configure the Unix Domain Socket address.
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path_, sizeof(addr.sun_path) - 1);

    // Bind the socket to the path
    if (bind(server_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::perror("Bind failed");
        close(server_fd_);
        return false;
    }

    // Listen for incoming connections
    if (listen(server_fd_, 5) == -1) {
        std::perror("Listen failed");
        close(server_fd_);
        return false;
    }
    return true;
}

void ControllerServer::run() {}

void ControllerServer::closeSocket()
{
    if (server_fd_ != -1) {
        close(server_fd_);
        server_fd_ = -1;
    }

    ::unlink(socket_path_);
}