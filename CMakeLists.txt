cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(network-monitor)

# Dependencies
# If dependencies are managed with conan, we prepend the current build folder
# to the CMake module path, where CMake looks for dependencies.
find_package(Boost 1.85.0 REQUIRED COMPONENTS system)
find_package(OpenSSL 3.2.1 REQUIRED COMPONENTS system)
find_package(CURL 8.6.0 REQUIRED)
find_package(CURL 8.6.0 REQUIRED)
find_package(nlohmann_json 3.11.3 REQUIRED)
find_package(absl 20240116.2 REQUIRED)

set(INC "inc")

set(TRANSPORT_INTERNAL_LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/transport-network-internal.cpp"
)
add_library(transport-network-int STATIC ${TRANSPORT_INTERNAL_LIB_SOURCES})
target_compile_features(transport-network-int
    PRIVATE
        cxx_std_17
)
target_include_directories(transport-network-int
    PRIVATE
    ${INC})

set(TRANSPORT_LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/transport-network.cpp"
)
add_library(transport-network STATIC ${TRANSPORT_LIB_SOURCES})
target_compile_features(transport-network
    PRIVATE
        cxx_std_17
)
target_include_directories(transport-network
    PUBLIC
    ${INC})
target_link_libraries(transport-network
    PUBLIC
        ${Boost_LIBRARIES}
        openssl::openssl
        nlohmann_json::nlohmann_json
    PRIVATE
        transport-network-int
)

set(LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp"
)
add_library(network-monitor STATIC ${LIB_SOURCES})
target_compile_features(network-monitor
    PRIVATE
        cxx_std_17
)
set(INC "inc")
target_include_directories(network-monitor
    PUBLIC
    ${INC})
target_link_libraries(network-monitor
    PUBLIC
        ${Boost_LIBRARIES}
        openssl::openssl
)

add_library(file-downloader STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/file-downloader.cpp")
target_compile_features(file-downloader
    PRIVATE
        cxx_std_17
)
target_include_directories(file-downloader
    PUBLIC
    ${INC})
target_link_libraries(file-downloader
    PRIVATE
        CURL::libcurl
    PUBLIC
        nlohmann_json::nlohmann_json
)

enable_testing()

set(TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/file-downloader.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/transport-network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp")

add_executable(network-monitor-tests ${TEST_SOURCES})
target_compile_features(network-monitor-tests
    PRIVATE
        cxx_std_17
)
target_include_directories(network-monitor-tests
    PRIVATE
    ${NETWORK_MONITOR_INC})
target_link_libraries(network-monitor-tests
    PRIVATE
        ${Boost_LIBRARIES}
        abseil::abseil
        network-monitor
        file-downloader
        transport-network
)
target_compile_definitions(network-monitor-tests
    PRIVATE
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
        TESTS_NETWORK_LAYOUT_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json"
)

add_test(NAME websocket-client-test COMMAND ./network-monitor-tests)