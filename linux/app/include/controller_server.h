#ifndef CONTROLLER_SERVER_H_
#define CONTROLLER_SERVER_H_

/**
 * @brief IPC server exposing RobotController functionality over a Unix Domain Socket.
 *
 * Responsibilities:
 * - Accept connections from the FastAPI backend.
 * - Receive JSON-RPC requests.
 * - Dispatch requests to the RobotController.
 * - Return JSON responses.
 */
class ControllerServer {
public:
    bool start();
    void run();
    void stop();

private:
    /**
     * @brief Creates, binds, and begins listening on the Unix domain socket.
     */
    bool openSocket();

    /**
     * @brief Closes the listening socket and releases associated resources.
     */
    void closeSocket();

    /// File descriptor for the listening Unix domain socket.
    int server_fd_ = -1;
    const char* socket_path_ = "/tmp/pacerbot-controller.sock";
};

#endif
