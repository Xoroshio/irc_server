#include <iostream>
#include <asio.hpp>

#include "Server.h"
#include <conio.h> // Windows specific

#include "context.hpp"

void watch_keyboard(Server& server) {
    std::string input;
    while (true) {
        // Read a single character without echoing it or requiring 'Enter'
        char ch = _getch();

        if (ch == '\r') { // Windows 'Enter' key
            if (input == "stop") {
                std::cout << "[Server] Stopping server..." << std::endl;
                server.stop();
                break;
            }
            input.clear(); // Clear buffer for next command
        } else {
            input += ch; // Append character to our command string
        }
    }
}
int main() {
    try {
        asio::io_context io;

        asio::ssl::context ssl_context(asio::ssl::context::tlsv13_server);

        ssl_context.set_options(asio::ssl::context::default_workarounds
                        | asio::ssl::context::no_sslv2
                        | asio::ssl::context::no_sslv3);

        ssl_context.set_verify_mode(asio::ssl::context::verify_none);

        ssl_context.use_certificate_chain_file("auth/server.crt");
        ssl_context.use_private_key_file("auth/server.key", asio::ssl::context::pem);

        short port = 8080;

        Server server(io, ssl_context,port);

        std::thread input_thread(watch_keyboard, std::ref(server));

        std::cout << "[Server] Running server on port [" << port  << "]..." << std::endl;

        io.run();

        input_thread.join();
    } catch (std::exception &e) {
        std::cerr << "[Error] Exception: " << e.what() << std::endl;
    }

    return 0;
}
