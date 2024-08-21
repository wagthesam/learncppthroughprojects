#include "acceptor-mock.h"
#include "network-monitor/websocket-server.h"

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <sstream>

#include "absl/strings/match.h"


BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_WebSocketServer_non_mock);

BOOST_AUTO_TEST_CASE(class_WebSocketServer)
{
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END(); // class_WebSocketClient

BOOST_AUTO_TEST_SUITE_END(); // network_monitor
