//
// Created by Xoroshio on 26/06/2026.
//

#ifndef IRC_SERVER_UTILS_H
#define IRC_SERVER_UTILS_H

#include <openssl/evp.h>
#include <iomanip>
#include <sstream>

inline std::string sha256(const std::string& input) {
    // Create a message digest context
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) {
        return "";
    }

    // Initialize the context with the SHA256 algorithm
    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    // Pass the data you want to hash
    if (EVP_DigestUpdate(context, input.c_str(), input.length()) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    // Allocate memory for the resulting hash bytes
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;

    // Finalize the hash computation
    if (EVP_DigestFinal_ex(context, hash, &length) != 1) {
        EVP_MD_CTX_free(context);
        return "";
    }

    // Clean up memory
    EVP_MD_CTX_free(context);

    // Convert the raw bytes into a readable Hexadecimal string
    std::stringstream ss;
    for (unsigned int i = 0; i < length; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }

    return ss.str();
}

#endif //IRC_SERVER_UTILS_H
