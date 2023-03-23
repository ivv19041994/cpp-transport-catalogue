cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto transport_router.proto graph.proto)

set(FILES json_builder.h serialization.cpp domain.cpp json_reader.cpp serialization.h domain.h json_reader.h geo.cpp geo.h main.cpp svg.cpp graph.h map_renderer.cpp svg.h map_renderer.h transport_catalogue.cpp ranges.h transport_catalogue.h json.cpp request_handler.cpp transport_catalogue.proto json.h request_handler.h transport_router.cpp json_builder.cpp router.h transport_router.h)

add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)