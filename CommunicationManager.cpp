#include "CommunicationManager.h"

CommunicationManager::CommunicationManager(const std::string& host, const std::string& port)
        : io_service_(), socket_(io_service_) {
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(host, port);
    boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    boost::asio::connect(socket_, endpoint_iterator);
}

void CommunicationManager::send(const std::string& message) {
    boost::asio::write(socket_, boost::asio::buffer(message));
}

std::string CommunicationManager::receive() {
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket_, buffer, "\n");

    std::istream input_stream(&buffer);
    std::string response;
    std::getline(input_stream, response);
    return response;
}