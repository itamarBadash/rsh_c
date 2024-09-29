#ifndef UDP_VIDEO_STREAMER_H
#define UDP_VIDEO_STREAMER_H

#include <../../../opencv/build/include/opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

class UDPVideoStreamer {
public:
    // Constructor: Takes camera index, destination IP, and port as parameters
    UDPVideoStreamer(int camera_index, const std::string& dest_ip, int dest_port);

    // Destructor: Cleans up resources
    ~UDPVideoStreamer();

    void send_frame_chunks(const std::vector<uchar> &buffer);

    // Method to start streaming video frames over UDP
    void stream();

private:
    int camera_index_;                 // Camera index (for OpenCV)
    std::string dest_ip_;              // Destination IP address for UDP streaming
    int dest_port_;                    // Destination port number
    int sock_;                         // UDP socket
    sockaddr_in server_address_;       // Server address structure for UDP
    cv::VideoCapture cap_;             // OpenCV video capture object
};

#endif // UDP_VIDEO_STREAMER_H
