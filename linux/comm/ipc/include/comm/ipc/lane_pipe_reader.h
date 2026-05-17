/**

 * @file lane_pipe_reader.h
 * @brief IPC interface for receiving lane detection data.
 * @author Michael Chandra
 * @date May-17-2026
 */

#pragma once

struct LaneInput {
    bool valid;
    float steering_error;
};

// Initialize/open the IPC pipe
bool initializeLanePipe();

// Read latest lane input from pipe
bool readLaneInput(LaneInput &input);