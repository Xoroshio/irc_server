//
// Created by Xoroshio on 23/06/2026.
//

#include "Session.h"

#include <iostream>

#include "Server.h"
#include "utils.h"

void Session::read_data()  {
    auto self(shared_from_this());
    ssl_socket_.async_read_some(asio::buffer(data_, max_length),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                for (int i = 0; i < length; i++) {
                    receive_byte(data_[i]);
                }
                read_data();
            } else {
                // Log the exact reason for debugging
                if (ec == asio::error::eof) {
                    std::cout << "[Server] [Client#" << id_ << "] disconnected cleanly (TCP EOF)." << std::endl;
                } else if (ec == asio::ssl::error::stream_truncated) {
                    std::cout << "[Server] [Client#" << id_ << "] closed transport layer abruptly." << std::endl;
                } else {
                    std::cout << "[Server] [Client#" << id_ << "] disconnected with error: " << ec.message() << std::endl;
                }

                handle_disconnect();
            }
        });
}

void Session::write_data(std::size_t length) {
    auto self(shared_from_this());
    asio::async_write(ssl_socket_, asio::buffer(data_, length),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {

            } else if (!is_disconnecting){
                // Log the exact reason for debugging
                if (ec == asio::error::eof) {
                    std::cout << "[Server] [Client#" << id_ << "] disconnected cleanly (TCP EOF)." << std::endl;
                } else if (ec == asio::ssl::error::stream_truncated) {
                    std::cout << "[Server] [Client#" << id_ << "] closed transport layer abruptly." << std::endl;
                } else {
                    std::cout << "[Server] [Client#" << id_ << "] disconnected with error: " << ec.message() << std::endl;
                }

                handle_disconnect();
            }
        });
}

void Session::receive_byte(char byte) {
    if (!read_packet_size) {
        packet_size_buffer[packet_size_len++] = byte;
        if (packet_size_len == 4)read_packet_size = true;
    } else if (!read_packet_id) {
        packet_id_buffer[packet_id_len++] = byte;
        if (packet_id_len == 4)read_packet_id = true;
    } else {
        if (packet_size == 0) {
            packet_size = packet_size_buffer[0] << 24 |
                            packet_size_buffer[1] << 16 |
                            packet_size_buffer[2] << 8 |
                            packet_size_buffer[3];

            packet_id = packet_id_buffer[0] << 24 |
                            packet_id_buffer[1] << 16 |
                            packet_id_buffer[2] << 8 |
                            packet_id_buffer[3];

            packet = (char*)malloc(sizeof(char) * packet_size);
        }

        packet[current_packet_length++] = byte;
        if (current_packet_length == packet_size) {
            received_packet();
            read_packet_size = false;
            read_packet_id = false;
            packet_size_len = 0; // RESET THIS
            packet_id_len = 0;   // RESET THIS
            packet_size = 0;
            packet_id = 0;
            current_packet_length = 0;
            free(packet);
            packet = nullptr;
        }
    }
}

void Session::kick(std::string reason) {
    send_packet(4, reason.data(), reason.size());
    shutdown();
}

void Session::received_packet() {
    if (packet_id == 0) { // Debug message
        std::cout << "[Server] [Client#" << id_ << "] Debug message: " << std::string(packet, packet_size) << std::endl;
    } else if (packet_id == 1) { // Set name
        std::string str = std::string(packet, packet_size);
        if (!server_.hasName(str)) {
            name = str;
            server_.setName(name, id_, *this);
        }
    } else if (packet_id == 2) { // Request perms
        const std::string key = sha256(std::string(packet, packet_size));
        bool granted = false;
        for (const auto& [hash, i] : server_.permission_keys) {
            if (key == hash) {
                if (i > perms) perms = i;
                granted = true;
                break;
            }
        }
        if (granted) {
            std::string msg = "Permission Granted, Level: " + std::to_string(perms) + ".";
            send_packet(3, msg.data(), msg.size());
            log("Acquired permission level " + std::to_string(perms) + ".");
        } else {
            std::string msg = "Password Incorrect.";
            send_packet(3, msg.data(), msg.size());
        }
    } else if (packet_id == 3) { // Kick other user
        if (perms >= 2) {
            std::string user = std::string(packet, packet_size);
            std::shared_ptr<Session> target = nullptr;
            for (const auto& [o_id, o_client] : server_.clients_) {
                if (std::to_string(o_id) == user || (!user.empty() && user == o_client->name)) {
                    target = o_client;
                    break;
                }
            }
            if (target != nullptr) target->kick("Kicked by staff.");
        }
    } else if (packet_id == 4) { // General Message
        const auto msg = std::string(packet, packet_size);
        const std::string send_as_name = name.empty()? "Anonymous" : name;
        std::string final = "[" + send_as_name + "] " + msg;
        std::cout << final << std::endl;
        for (const auto& pair: server_.clients_) {
            if (pair.second->id_ == id_) continue;
            pair.second->send_packet(5, final.data(), final.size());
        }
    }
}

void Session::send_packet(uint32_t id, char *data, uint32_t size) {
    char* p = (char*)malloc(sizeof(char) * size + 8);

    p[0] = static_cast<char>((size >> 24) & 0xFF); // Most significant byte
    p[1] = static_cast<char>((size >> 16) & 0xFF);
    p[2] = static_cast<char>((size >> 8) & 0xFF);
    p[3] = static_cast<char>(size & 0xFF);

    p[4] = static_cast<char>((id >> 24) & 0xFF); // Most significant byte
    p[5] = static_cast<char>((id >> 16) & 0xFF);
    p[6] = static_cast<char>((id >> 8) & 0xFF);
    p[7] = static_cast<char>(id & 0xFF);

    mempcpy(p+8, data, size);

    asio::write(ssl_socket_, asio::buffer(p, size + 8));

    free(p);
}

void Session::handle_disconnect() {
    if (is_disconnecting) return; // Prevent recursive loops during destruction
    is_disconnecting = true;

    server_.remove_session(id_, name);

    if (packet != nullptr) {
        free(packet);
        packet = nullptr;
    }
}
