//
// Created by richard on 23/06/2026.
//

#ifndef IRC_SERVER_SESSION_H
#define IRC_SERVER_SESSION_H
#include <iostream>
#include <memory>

#include "stream.hpp"
#include "asio/ip/tcp.hpp"

class Server; // Forward declaration

class Session : public std::enable_shared_from_this<Session> {
public:
    // Pass the client ID and a reference to the server
    Session(asio::ip::tcp::socket socket, asio::ssl::context& ctx, int id, Server& server)
        : ssl_socket_(std::move(socket), ctx), id_(id), server_(server) {
    }

    ~Session() {
        std::cout << "[Memory Log] [Client#" << id_ << "] has been deleted from the heap." << std::endl;
    }

    void start() {
        // You MUST perform the TLS handshake before reading/writing raw bytes
        auto self(shared_from_this());
        ssl_socket_.async_handshake(asio::ssl::stream_base::server,
            [this, self](std::error_code ec) {
                if (!ec) {
                    read_data(); // Handshake succeeded, proceed to normal loop
                } else {
                    std::cout << "Handshake failed: " << ec.message() << std::endl;
                }
            });
    }

    void send_packet(uint32_t id, char* data, uint32_t length);

    void close() {
        handle_disconnect();
    }

    void shutdown() {
        std::error_code ec;
        // Close the underlying TCP socket to force cancel any pending async operations
        ssl_socket_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        ssl_socket_.lowest_layer().close(ec);
    }

    void kick(std::string reason);

    void log(const std::string &msg) const {
        if (!name.empty()) {
            std::cout << "[Server] [Client#" << id_ << "] [" << name << "]: " << msg << std::endl;
        } else {
            std::cout << "[Server] [Client#" << id_ << "]: " << msg << std::endl;
        }
    }

    int get_id() const { return id_; }

private:
    void read_data();

    void write_data(std::size_t length);

    void receive_byte(char byte);

    void received_packet();

    asio::ssl::stream<asio::ip::tcp::socket> ssl_socket_;
    int id_;
    Server& server_;
    static constexpr size_t max_length = 1024;
    char data_[max_length];

    char packet_size_buffer[4] = {};
    char packet_id_buffer[4] = {};

    char* packet = nullptr;

    bool read_packet_size = false;
    bool read_packet_id = false;

    uint32_t packet_size_len = 0;
    uint32_t packet_id_len = 0;
    uint32_t packet_size = 0;
    uint32_t packet_id = 0;
    uint32_t current_packet_length = 0;

    std::string name;

    void handle_disconnect();

    bool is_disconnecting = false;

    int perms = 0;
};

#endif //IRC_SERVER_SESSION_H
