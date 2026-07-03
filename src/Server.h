//
// Created by Xoroshio on 23/06/2026.
//

#ifndef IRC_SERVER_SERVER_H
#define IRC_SERVER_SERVER_H
#include "Session.h"

class Server {
public:
    Server(asio::io_context& io_context, asio::ssl::context& ssl_ctx, short port);

    void stop() {
        // Correctly loop through every active pairing inside the hashmap
        for (const auto& [id, session] : clients_) {
            session->kick("Server stopping.");
        }
        acceptor_.close();
    }

    void remove_session(const int id, const std::string& name);

    void setName(const std::string& name, const int clientId, Session& session);

    bool hasName(const std::string& name);

    std::unordered_map<std::string, int> name_to_id;

    std::unordered_map<std::string, int> permission_keys;

    // The hashmap holding the clients
    std::unordered_map<int, std::shared_ptr<Session>> clients_;
    std::vector<std::string> client_names;
private:
    void accept_connection();

    asio::ip::tcp::acceptor acceptor_;
    asio::ssl::context& ssl_ctx_;

    int next_id_ = 0;
};

#endif //IRC_SERVER_SERVER_H
