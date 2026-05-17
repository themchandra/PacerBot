#include "comm/ipc/lane_pipe_reader.h"

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
    constexpr const char *kLanePipePath = "/tmp/pacerbot_lane.pipe";
    int g_pipe_fd                       = -1;

    bool ensurePipeExists()
    {
        if (mkfifo(kLanePipePath, 0666) == 0) {
            return true;
        }

        return errno == EEXIST;
    }

    void closePipe()
    {
        if (g_pipe_fd >= 0) {
            close(g_pipe_fd);
            g_pipe_fd = -1;
        }
    }
} // namespace

bool initializeLanePipe()
{
    if (g_pipe_fd >= 0) {
        return true;
    }

    if (!ensurePipeExists()) {
        return false;
    }

    g_pipe_fd = open(kLanePipePath, O_RDONLY | O_NONBLOCK);
    return g_pipe_fd >= 0;
}

bool readLaneInput(LaneInput &input)
{
    if (!initializeLanePipe()) {
        return false;
    }

    LaneInput pending {};
    const ssize_t bytesRead = read(g_pipe_fd, &pending, sizeof(pending));

    if (bytesRead == static_cast<ssize_t>(sizeof(pending))) {
        input = pending;
        return true;
    }

    if (bytesRead == 0) {
        closePipe();
        return false;
    }

    if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
        return false;
    }

    closePipe();
    return false;
}