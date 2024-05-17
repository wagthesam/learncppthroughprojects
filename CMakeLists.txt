cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(network-monitor)

# Dependencies
# If dependencies are managed with conan, we prepend the current build folder
# to the CMake module path, where CMake looks for dependencies.
find_package(Boost 1.85.0 REQUIRED COMPONENTS system)
find_package(OpenSSL 3.2.1 REQUIRED COMPONENTS system)
set(LIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-client.cpp"
)
add_library(network-monitor STATIC ${LIB_SOURCES})
target_compile_features(network-monitor
    PRIVATE
        cxx_std_17
)

set(NETWORK_MONITOR_INC "inc")
target_include_directories(network-monitor
    PUBLIC
    ${NETWORK_MONITOR_INC})
target_link_libraries(network-monitor
    PUBLIC
        ${Boost_LIBRARIES}
        openssl::openssl
)

enable_testing()

set(TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/websocket-client.cpp"
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
        network-monitor
        ${Boost_LIBRARIES}
)
target_compile_definitions(network-monitor-tests
    PRIVATE
        TESTS_CACERT_PEM="${CMAKE_CURRENT_SOURCE_DIR}/tests/cacert.pem"
)

add_test(NAME websocket-client-test COMMAND ./network-monitor-tests)