#ifndef BASE_CAMERA_H
#define BASE_CAMERA_H

#include <opencv2/opencv.hpp>
#include <string>

class BaseCamera {
private:
    cv::VideoCapture cap;
    cv::VideoWriter writer;
    int device_id;
    int width;
    int height;
    bool is_recording;

public:
    BaseCamera(int device_id = 0, int width = 640, int height = 480);
    bool getFrame(cv::Mat &frame);

    // Methods to handle video recording
    bool startRecording(const std::string& filename, int codec = cv::VideoWriter::fourcc('M','J','P','G'), double fps = 30.0);
    void stopRecording();
    bool isRecording() const { return is_recording; }

    ~BaseCamera();
};

#endif // BASE_CAMERA_H
