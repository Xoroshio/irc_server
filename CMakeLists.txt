cmake_minimum_required(VERSION 4.2)
project(irc_server)

add_compile_definitions(ASIO_STANDALONE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/openssl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/asio/ssl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(OPENSSL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})

set(CMAKE_CXX_STANDARD 20)

add_executable(irc_server main.cpp
        src/Session.cpp
        src/Session.h
        src/Server.cpp
        src/Server.h
        src/utils.h)

set(PATH_TO_LIBSSL ${CMAKE_CURRENT_SOURCE_DIR}/include/libssl.lib)
set(PATH_TO_LIBCRYPTO ${CMAKE_CURRENT_SOURCE_DIR}/include/libcrypto.lib)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
endif()

target_link_libraries(irc_server PRIVATE
        ${PATH_TO_LIBSSL}
        ${PATH_TO_LIBCRYPTO}
)

if(WIN32)
    target_link_libraries(irc_server PRIVATE ws2_32 wsock32)
endif()

add_custom_command(TARGET irc_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/libssl-4-x64.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/libssl-4-x64.dll"
)

add_custom_command(TARGET irc_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/libcrypto-4-x64.dll"
        "${CMAKE_CURRENT_BINARY_DIR}/libcrypto-4-x64.dll"
)
