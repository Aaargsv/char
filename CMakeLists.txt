cmake_minimum_required(VERSION 3.17)
project(chat)

set(CMAKE_CXX_STANDARD 14)
link_libraries(ws2_32)

add_executable(server src/server.cpp)
add_executable(client src/client.cpp include/custom_stream.h include/custom_recorder.h include/chat.h include/utils.h include/semaphore.h)
include_directories(include)
include_directories(common/cryptopp/include)

# use static libs
set( SFML_STATIC_LIBRARIES TRUE )

#directory where the cmake files for SFML are. You will want to change this to wherever your SFML installation is
set(SFML_DIR "common/SFML-2.5.1/lib/cmake/SFML" )
find_package(SFML 2.5 COMPONENTS graphics window main network audio REQUIRED)

add_library(cryptopp STATIC IMPORTED)
set_target_properties(cryptopp PROPERTIES
        IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/common/cryptopp/lib/libcryptopp.a)
add_dependencies(cryptopp chat)


target_link_libraries( client
        PRIVATE
        sfml-audio cryptopp  -static -static-libgcc -static-libstdc++
        )
target_link_libraries(server cryptopp -static -static-libgcc -static-libstdc++)