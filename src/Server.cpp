//
// Created by richard on 25/06/2026.
//

#include "Server.h"

#include "utils.h"

Server::Server(asio::io_context& io_context, asio::ssl::context& ssl_ctx, short port)
        : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), ssl_ctx_(ssl_ctx) {

    permission_keys = {
        {sha256("Super Secret"), 10}
    };

    accept_connection();
}

void Server::remove_session(const int id, const std::string &name){
    if (!name.empty() && hasName(name)) {
        std::erase(client_names, name);
        name_to_id.erase(name);
    }
    clients_.erase(id);
    std::cout << "[Server] [Client#" << id << "] disconnected. Total clients: " << clients_.size() << std::endl;
}

void Server::setName(const std::string &name, const int clientId, Session &session) {
    name_to_id[name] = clientId;
    if (!hasName(name)) {
        client_names.push_back(name);
        std::cout << "[Server] [Client#" << clientId << "] self identified as: " << name << std::endl;
        std::string msg = "Name self successful";
        session.send_packet(2, msg.data(), msg.size());
    } else {
        std::string msg = "Someone else already has that name.";
        session.send_packet(2, msg.data(), msg.size());
    }
}

bool Server::hasName(const std::string &name) {
    for (const auto & client_name : client_names) {
        if (client_name == name) return true;
    }
    return false;
}

void Server::accept_connection() {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (ec == asio::error::operation_aborted) {
                return;
            }

            if (!ec) {
                int current_id = next_id_++;

                // Create the session
                auto session = std::make_shared<Session>(std::move(socket), ssl_ctx_, current_id, *this);

                // Store it in the hashmap
                clients_[current_id] = session;

                std::cout << "[Server] [Client#" << current_id << "] connected. Total clients: " << clients_.size() << std::endl;

                session->start();
            }
            accept_connection();
        });
}
