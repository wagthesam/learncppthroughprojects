#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <ostream>

namespace NetworkMonitor {

/*! \brief Available STOMP commands, from the STOMP protocol v1.2.
 */
enum class StompCommand {
    // client commands
    kUndefined = 0,
    kSend = 1,
    kSubscribe = 2,
    kUnsubscribe = 3,
    kBegin = 4,
    kCommit = 5,
    kAbort = 6,
    kAck = 7,
    kNack = 8,
    kError = 9,
    kDisconnect = 10,
    kConnect = 11,
    kStomp = 12,
    // server commands
    kConnected = 13,
    kMessage = 14,
    kReceipt = 15,
    kServerError = 16,
};

/*! \brief Available STOMP headers, from the STOMP protocol v1.2.
 */
enum class StompHeader {
    // TODO: Your enum values go here
    kUndefined = 0,
    kContentLength = 1,
    kContentType = 2,
    kReceipt = 3,
    kHost = 4,
    kAcceptVersion = 5,
    kMessage = 6,
    kReceiptId = 7,
    kDestination = 8,
    kMessageId = 9,
    kAck = 10,
    kSubscription = 11,
    kId = 12,
    kVersion = 13,
    kTransaction = 14,
    kSession = 15,
    kLogin = 16,
    kPasscode = 17,
    kServer = 18,
    kHeartBeat = 19,
};

/*! \brief Error codes for the STOMP protocol
 */
enum class StompError {
    kOk = 0,
    kParsing = 1,
    kValidation = 2,
};

/* \brief STOMP frame representation, supporting STOMP v1.2.
 */
class StompFrame {
public:
    /*! \brief Construct the STOMP frame from a string. The string is copied.
     *
     *  The result of the operation is stored in the error code.
     */
    StompFrame(
        StompError& ec,
        const std::string& frame
    );

    /*! \brief Construct the STOMP frame from a string. The string is moved into
     *         the object.
     *
     *  The result of the operation is stored in the error code.
     */
    StompFrame(
        StompError& ec,
        std::string&& frame
    );

    std::unordered_set<StompHeader> GetHeaders() const;

    std::string_view GetHeaderValue(StompHeader header) const;

    std::string_view GetBody() const;

    StompCommand GetCommand() const;

private:
    void Parse(const std::string& frame);
    uint32_t ParseCommand(const std::string& frame);
    uint32_t ParseHeaders(const std::string& frame, uint32_t idx);
    uint32_t ParseHeader(const std::string& frame, uint32_t idx);
    uint32_t ParseBody(const std::string& frame, uint32_t idx);
    void ParseEol(const std::string& frame, uint32_t idx);
    void Validate();
    bool HasBody() const;

    static std::optional<StompCommand> strToCommand(const std::string& str);

    static std::optional<StompHeader> strToHeader(const std::string& str);

    std::unordered_map<StompHeader, std::string> headers_;
    uint32_t content_length_;
    StompCommand command_{StompCommand::kUndefined};
    StompError& state_;
    std::string_view body_;
    const std::string frame_;
};

std::ostream& operator<<(std::ostream& os, const StompError& error);

std::ostream& operator<<(std::ostream& os, const StompHeader& header);

std::ostream& operator<<(std::ostream& os, const StompCommand& command);

} // namespace NetworkMonitor
