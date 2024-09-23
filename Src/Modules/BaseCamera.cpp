#include "BaseCamera.h"
#include <iostream>

BaseCamera::BaseCamera(int device_id, int width, int height)
    : device_id(device_id), width(width), height(height), is_recording(false) {
    // Initialize the camera
    cap.open(device_id);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera." << std::endl;
        exit(EXIT_FAILURE);
    }
    // Set frame width and height
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
}

bool BaseCamera::getFrame(cv::Mat &frame) {
    if (!cap.read(frame)) {
        return false;
    }

    // If recording is enabled, write the frame to the file
    if (is_recording) {
        writer.write(frame);
    }

    return true;
}

bool BaseCamera::startRecording(const std::string& filename, int codec, double fps) {
    if (is_recording) {
        std::cerr << "Recording is already in progress." << std::endl;
        return false;
    }

    // Initialize VideoWriter for recording
    writer.open(filename, codec, fps, cv::Size(width, height));
    if (!writer.isOpened()) {
        std::cerr << "Error: Could not open video file for writing." << std::endl;
        return false;
    }

    is_recording = true;
    return true;
}

void BaseCamera::stopRecording() {
    if (is_recording) {
        writer.release();
        is_recording = false;
    }
}

BaseCamera::~BaseCamera() {
    // Ensure resources are released
    cap.release();
    if (is_recording) {
        writer.release();
    }
}
