#include <network-monitor/stomp-frame.h>

#include <boost/test/unit_test.hpp>

#include <sstream>
#include <string>

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;

using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame);

BOOST_AUTO_TEST_SUITE(class_StompFrame);

BOOST_AUTO_TEST_CASE(parse_well_formed)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    std::string a = std::string(frame.GetBody());

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_well_formed_content_length)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:10\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kContentLength), "10");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_empty_body)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_empty_body_content_length)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:0\n"
        "\n"
        "\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kContentLength), "0");
    BOOST_CHECK_EQUAL(frame.GetBody(), "");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_empty_headers)
{
    std::string plain {
        "DISCONNECT\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaders().size(), 0);
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kDisconnect);
}

BOOST_AUTO_TEST_CASE(parse_only_command)
{
    std::string plain {
        "DISCONNECT\n"
        "\n"
        "\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaders().size(), 0);
    BOOST_CHECK_EQUAL(frame.GetBody(), "");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kDisconnect);
}

BOOST_AUTO_TEST_CASE(parse_bad_command)
{
    std::string plain {
        "CONNECTX\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_bad_header)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "login\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_missing_body_newline)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_missing_last_header_newline)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com"
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_unrecognized_header)
{
    std::string plain {
        "CONNECT\n"
        "bad_header:42\n"
        "host:host.com\n"
        "\n"
        "\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_empty_header_value)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:\n"
        "host:host.com\n"
        "\n"
        "\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_just_command)
{
    std::string plain {
        "CONNECT"
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_double_colon_in_header_line)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42:43\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42:43");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_repeated_headers)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "accept-version:43\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);
}

BOOST_AUTO_TEST_CASE(parse_repeated_headers_error_in_second)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "accept-version:\n"
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_unterminated_body)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_unterminated_body_content_length)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:10\n"
        "\n"
        "Frame body"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_junk_after_body)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0\n\njunk\n"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_junk_after_body_content_length)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:10\n"
        "\n"
        "Frame body\0\n\njunk\n"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_newlines_after_body)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0\n\n\n"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_newlines_after_body_content_length)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:10\n"
        "\n"
        "Frame body\0\n\n\n"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK_EQUAL(error, StompError::kOk);

    // Add more checks here.
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
}

BOOST_AUTO_TEST_CASE(parse_content_length_wrong_number)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:9\n" // This is one byte off
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_content_length_exceeding)
{
    std::string plain {
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:15\n" // Way above the actual body length
        "\n"
        "Frame body\0"s
    };
    StompError error;
    StompFrame frame {error, std::move(plain)};
    BOOST_CHECK(error != StompError::kOk);

    // Add more checks here.
    // ...
}

BOOST_AUTO_TEST_CASE(parse_required_headers)
{
    StompError error;
    {
        std::string plain {
            "CONNECT\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);

    // Add more checks here.
    // ...
    }
    {
        std::string plain {
            "CONNECT\n"
            "accept-version:42\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);

    // Add more checks here.
    // ...
    }
    {
        std::string plain {
            "CONNECT\n"
            "accept-version:42\n"
            "host:host.com\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
    }
    {
        std::string plain {
            "ERROR\n"
            "message:malformed frame received\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kMessage), "malformed frame received");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kError);
    }
    {
        std::string plain {
            "RECEIPT\n"
            "receipt-id:message-12345\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kReceiptId), "message-12345");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kReceipt);
    }
    {
        std::string plain {
            "RECEIPT\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "MESSAGE\n"
            "destination:/queue/a\n"
            "message-id:007\n"
            "subscription:0\n"
            "content-type:text/plain\n"
            "content-length:11\n"
            "\n"
            "hello queue\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kMessageId), "007");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kSubscription), "0");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kContentLength), "11");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kContentType), "text/plain");
        BOOST_CHECK_EQUAL(frame.GetBody(), "hello queue");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kMessage);
    }
    {
        std::string plain {
            "MESSAGE\n"
            "destination:/queue/a\n"
            "message-id:007\n"
            "subscription:0\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kMessageId), "007");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kSubscription), "0");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kMessage);
    }
    {
        std::string plain {
            "MESSAGE\n"
            "destination:/queue/a\n"
            "message-id:007\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kMessageId), "007");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kMessage);
    }
    {
        std::string plain {
            "SEND\n"
            "destination:/queue/a\n"
            "\n"
            "hello queue a\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetBody(), "hello queue a\n");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kSend);
    }
    {
        std::string plain {
            "SEND\n"
            "\n"
            "hello queue a\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "SUBSCRIBE\n"
            "id:0\n"
            "destination:/queue/a\n"
            "ack:client\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAck), "client");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kId), "0");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kSubscribe);
    }
    {
        std::string plain {
            "SUBSCRIBE\n"
            "id:0\n"
            "destination:/queue/a\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kDestination), "/queue/a");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAck), "auto");
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kId), "0");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kSubscribe);
    }
    {
        std::string plain {
            "SUBSCRIBE\n"
            "ack:client\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "UNSUBSCRIBE\n"
            "id:0\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kId), "0");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kUnsubscribe);
    }
    {
        std::string plain {
            "UNSUBSCRIBE\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "ACK\n"
            "id:12345\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kId), "12345");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kAck);
    }
    {
        std::string plain {
            "ACK\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "NACK\n"
            "id:12345\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kId), "12345");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kNack);
    }
    {
        std::string plain {
            "NACK\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
    {
        std::string plain {
            "DISCONNECT\n"
            "receipt:77\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kReceipt), "77");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kDisconnect);
    }
    {
        std::string plain {
            "DISCONNECT\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);
    }
    {
        std::string plain {
            "RECEIPT\n"
            "receipt-id:77\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error == StompError::kOk);

        // Add more checks here.
        BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kReceiptId), "77");
        BOOST_CHECK_EQUAL(frame.GetBody(), "");
        BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kReceipt);
    }
    {
        std::string plain {
            "RECEIPT\n"
            "\n"
            "\0"s
        };
        StompFrame frame {error, std::move(plain)};
        BOOST_CHECK(error != StompError::kOk);
    }
}

// ...

BOOST_AUTO_TEST_SUITE_END(); // class_StompFrame

BOOST_AUTO_TEST_SUITE_END(); // stomp_frame

BOOST_AUTO_TEST_SUITE_END(); // network_monitor