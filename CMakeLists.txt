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

set(MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
)
add_executable(network-monitor-exe ${MAIN_SOURCES})
target_compile_features(network-monitor-exe
    PRIVATE
        cxx_std_17
)
target_include_directories(network-monitor-exe
    PUBLIC
    ${INC})
target_link_libraries(network-monitor-exe
    PUBLIC
        ${Boost_LIBRARIES}
        transport-network
        openssl::openssl
        nlohmann_json::nlohmann_json
        stomp
)

set(STOMP_LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/stomp-frame.cpp"
)
add_library(stomp STATIC ${STOMP_LIB_SOURCES})
target_compile_features(stomp
    PRIVATE
        cxx_std_17
)
target_include_directories(stomp
    PUBLIC
    ${INC})
target_link_libraries(stomp
    PUBLIC
        ${Boost_LIBRARIES}
)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-frame.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/stomp-client.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/transport-network.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp")

set(TEST_INC "tests")
add_executable(network-monitor-tests ${TEST_SOURCES})
target_compile_features(network-monitor-tests
    PRIVATE
        cxx_std_17
)
target_include_directories(network-monitor-tests
    PRIVATE
    ${INC}
    ${TEST_INC})
target_link_libraries(network-monitor-tests
    PRIVATE
        ${Boost_LIBRARIES}
        abseil::abseil
        file-downloader
        transport-network
        stomp
)
set(PEM_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
)
set(NETWORK_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/network-layout.json")
target_compile_definitions(network-monitor-tests
    PRIVATE
        TESTS_CACERT_PEM="${PEM_PATH}"
        TESTS_NETWORK_LAYOUT_JSON="${NETWORK_PATH}"
)

add_test(NAME network-monitor-tests COMMAND $<TARGET_FILE:network-monitor-tests>)
set_tests_properties(network-monitor-tests PROPERTIES
    PASS_REGULAR_EXPRESSION ".* No errors detected"
)

add_test(NAME network-monitor-exe
  COMMAND $<TARGET_FILE:network-monitor-exe>
  --url=ltnm.learncppthroughprojects.com
  --endpoint=/network-events
  --port=443
  --username=$ENV{stomp_username}
  --password=$ENV{stomp_password}
  --stomp_endpoint=/passengers
  --cert_path=${PEM_PATH}
  --network_layout_path=${NETWORK_PATH}
  --runtime_s=5
)
set_tests_properties(network-monitor-exe PROPERTIES
    PASS_REGULAR_EXPRESSION "OnConnect \| ok\n OnSubscribe \| ok\n OnClose \| ok\n.*"
)
