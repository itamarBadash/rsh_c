#include "UDPVideoStreamer.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <arpa/inet.h> // or <winsock2.h> on Windows for network functions

// Max UDP packet size for payload
const size_t MAX_UDP_PAYLOAD_SIZE = 65000;

// Constructor: Initializes camera and UDP socket
UDPVideoStreamer::UDPVideoStreamer(int camera_index, const std::string& dest_ip, int dest_port)
    : camera_index_(camera_index), dest_ip_(dest_ip), dest_port_(dest_port), sock_(-1) {

    // Initialize camera
    cap_.open(camera_index_);
    if (!cap_.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        throw std::runtime_error("Camera could not be opened");
    }

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

// Destructor: Clean up resources
UDPVideoStreamer::~UDPVideoStreamer() {
    if (sock_ >= 0) {
        close(sock_);
    }
    cap_.release();
}

// Function to send frame chunks
void UDPVideoStreamer::send_frame_chunks(const std::vector<uchar>& buffer) {
    size_t total_size = buffer.size();
    size_t offset = 0;

    while (offset < total_size) {
        // Calculate the size of the current chunk
        size_t chunk_size = std::min(MAX_UDP_PAYLOAD_SIZE, total_size - offset);

        // Send the chunk over UDP
        ssize_t bytes_sent = sendto(sock_, buffer.data() + offset, chunk_size, 0,
                                    (sockaddr*)&server_address_, sizeof(server_address_));
        if (bytes_sent < 0) {
            perror("Error: Failed to send frame chunk");
            break;
        }

        // Move the offset forward
        offset += chunk_size;
    }
}

// Stream video frames over UDP
void UDPVideoStreamer::stream() {
    cv::Mat frame;
    std::vector<uchar> buffer;

    while (true) {
        // Capture frame
        cap_ >> frame;
        if (frame.empty()) {
            std::cerr << "Error: No frame captured" << std::endl;
            break;
        }

        // Encode frame as JPEG and store in buffer
        if (!cv::imencode(".jpg", frame, buffer)) {
            std::cerr << "Error: Could not encode frame" << std::endl;
            break;
        }

        // Send the encoded frame over UDP in chunks
        send_frame_chunks(buffer);

        // Optionally display the frame locally
        cv::imshow("Camera", frame);

        // Break if a key is pressed
        if (cv::waitKey(30) >= 0) break;
    }

    // Clean up
    close(sock_);
    cap_.release();
    cv::destroyAllWindows();
}
