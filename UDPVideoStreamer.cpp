#include "UDPVideoStreamer.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <arpa/inet.h>

const size_t MAX_UDP_PAYLOAD_SIZE = 60000; // Slightly smaller to ensure it fits within MTU

UDPVideoStreamer::UDPVideoStreamer(int camera_index, const std::string& dest_ip, int dest_port)
    : camera_index_(camera_index), dest_ip_(dest_ip), dest_port_(dest_port), sock_(-1) {

    // Initialize camera
    cap_.open(camera_index_);
    if (!cap_.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        throw std::runtime_error("Camera could not be opened");
    }

    // Set camera resolution
    cap_.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap_.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Check if the resolution was set correctly
    double width = cap_.get(cv::CAP_PROP_FRAME_WIDTH);
    double height = cap_.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Camera resolution: " << width << "x" << height << std::endl;

    // Initialize UDP socket
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        std::cerr << "Error: Could not create socket" << std::endl;
        throw std::runtime_error("Failed to create socket");
    }

    // Configure server address for UDP
    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(dest_port_);

    // Convert IP address string to binary format
    if (inet_pton(AF_INET, dest_ip_.c_str(), &server_address_.sin_addr) <= 0) {
        std::cerr << "Error: Invalid IP address" << std::endl;
        throw std::runtime_error("Invalid IP address");
    }
}

UDPVideoStreamer::~UDPVideoStreamer() {
    if (sock_ >= 0) {
        close(sock_);
    }
    cap_.release();
}

void UDPVideoStreamer::send_frame_chunks(const std::vector<uchar>& buffer) {
    size_t total_size = buffer.size();
    size_t offset = 0;

    while (offset < total_size) {
        size_t bytes_to_send = std::min(MAX_UDP_PAYLOAD_SIZE, total_size - offset);
        ssize_t bytes_sent = sendto(sock_, buffer.data() + offset, bytes_to_send, 0,
                                    (sockaddr*)&server_address_, sizeof(server_address_));
        if (bytes_sent < 0) {
            perror("Error: Failed to send frame chunk");
            break;
        }
        offset += bytes_sent;
    }

    // Send an empty packet to signal end of frame
    sendto(sock_, nullptr, 0, 0, (sockaddr*)&server_address_, sizeof(server_address_));
}

void UDPVideoStreamer::stream() {
    cv::Mat frame;
    std::vector<uchar> buffer;
    int frame_count = 0;

    while (true) {
        // Capture frame
        cap_ >> frame;
        if (frame.empty()) {
            std::cerr << "Error: No frame captured" << std::endl;
            break;
        }

        frame_count++;
        std::cout << "Frame " << frame_count << ": Shape = " << frame.size()
                  << ", Type = " << frame.type() << std::endl;

        // Encode frame as JPEG and store in buffer
        std::vector<int> encode_params;
        encode_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        encode_params.push_back(70); // Reduced quality to ensure smaller packet size

        if (!cv::imencode(".jpg", frame, buffer, encode_params)) {
            std::cerr << "Error: Could not encode frame" << std::endl;
            break;
        }

        std::cout << "Encoded frame size: " << buffer.size() << " bytes" << std::endl;

        // Send frame size first
        uint32_t frame_size = htonl(buffer.size());
        sendto(sock_, &frame_size, sizeof(frame_size), 0, (sockaddr*)&server_address_, sizeof(server_address_));

        // Send the encoded frame over UDP in chunks
        send_frame_chunks(buffer);

        // Optionally display the frame locally
        cv::imshow("Camera", frame);

        // Save every 100th frame for analysis
        if (frame_count % 100 == 0) {
            cv::imwrite("debug_sent_frame_" + std::to_string(frame_count) + ".jpg", frame);
        }

        // Break if a key is pressed
        if (cv::waitKey(30) >= 0) break;
    }

    // Clean up
    close(sock_);
    cap_.release();
    cv::destroyAllWindows();
}