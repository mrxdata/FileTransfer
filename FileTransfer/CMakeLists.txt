cmake_minimum_required(VERSION 3.10)

#set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(CMAKE_PREFIX_PATH "C:/vcpkg/vcpkg/installed/x64-windows-static")

project(FileTransfer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCES
    main.cpp
    client.cpp
    server.cpp
)

set(HEADER
    client.h
    server.h
)

add_executable(FileTransfer ${SOURCES} ${HEADER})

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(FileTransfer PRIVATE OpenSSL::SSL)
target_link_libraries(FileTransfer PRIVATE OpenSSL::Crypto)
target_link_libraries(FileTransfer PRIVATE ws2_32)
