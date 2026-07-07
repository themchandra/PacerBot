#include "manual_control.h"

#include <cstdio>
#include <termios.h>
#include <unistd.h>


namespace {
    /*
    //
    //
    https://stackoverflow.com/questions/421860/capture-characters-from-standard-input-without-waiting-for-enter-to-be-pressed
    */
    char readKey()
    {
        // Store character read from the keyboard
        char buf = 0;

        // Save current terminal settings
        termios old;
        if (tcgetattr(STDIN_FILENO, &old) < 0) {
            perror("tcgetattr()");
            return '\0';
        }

        termios current = old;

        // Turn off canonical mode (no enter needed)
        // and echo mode (don't display typed characters)
        current.c_lflag &= ~ICANON;
        current.c_lflag &= ~ECHO;

        // min characters to read (non-blocking)
        current.c_cc[VMIN] = 0;

        // no timeout
        current.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &current) < 0) {
            perror("tcsetattr ICANON");
            return '\0';
        }
        // read character
        ssize_t bytesRead = read(STDIN_FILENO, &buf, 1);

        // restore original terminal settings
        if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
            perror("restore terminal");

        if (bytesRead < 0) {
            perror("read()");
            return '\0';
        }

        if (bytesRead == 0) {
            return '\0';
        }

        return buf;
    }
} // namespace

void updateManualControl(ManualControl &control)
{
    char direction = readKey();

    if (direction == '\0') {
        return;
    }

    // stop
    if (direction == ' ') {
        control = ManualControl {};
        return;
    }

    // forward backward controls
    if (direction == 'w') {
        control.isForward = true;
        control.isReverse = false;
    } else if (direction == 's') {
        control.isReverse = true;
        control.isForward = false;
    } else if (direction == 'x') {
        control.isForward = false;
        control.isReverse = false;
    }

    // left and right controls
    if (direction == 'a') {
        control.isLeft  = true;
        control.isRight = false;
    } else if (direction == 'd') {
        control.isRight = true;
        control.isLeft  = false;
    } else if (direction == 'q') {
        control.isLeft  = false;
        control.isRight = false;
    }
}

uint8_t encodeManualControl(const ManualControl &control)
{
    uint8_t bitmask           = 0;
    constexpr uint8_t forward = 1 << 3;
    constexpr uint8_t reverse = 1 << 2;
    constexpr uint8_t left    = 1 << 1;
    constexpr uint8_t right   = 1 << 0;

    if (control.isForward) {
        bitmask |= forward;
    } else if (control.isReverse) {
        bitmask |= reverse;
    }

    if (control.isLeft) {
        bitmask |= left;
    } else if (control.isRight) {
        bitmask |= right;
    }
    return bitmask;
}
