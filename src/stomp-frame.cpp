#include "network-monitor/stomp-frame.h"

#include <algorithm>
#include <string>

namespace NetworkMonitor {
StompFrame::StompFrame(
    StompError& ec,
    const std::string& frame
) : state_(ec), frame_(frame) {
    state_ = StompError::kOk;
    Parse(frame);
}

StompFrame::StompFrame(
    StompError& ec,
    std::string&& frame
) : state_(ec), frame_(frame) {
    state_ = StompError::kOk;
    Parse(frame);
}

std::unordered_set<StompHeader> StompFrame::GetHeaders() const {
    std::unordered_set<StompHeader> headers;
    for (const auto& [header, _] : headers_) {
        headers.insert(header);
    }
    return headers;
}

std::string_view StompFrame::GetHeaderValue(StompHeader header) const {
    auto it = headers_.find(header);
    if (it != headers_.end()) {
        return it->second;
    }
    return {};
}

uint32_t StompFrame::ParseCommand(const std::string& frame) {
    size_t idx = 0;
    while (idx != frame.size()) {
        if (frame[idx] == '\n') {
            auto maybeCommand = strToCommand(frame.substr(0, idx));
            if (maybeCommand.has_value()) {
                command_ = maybeCommand.value();
            }
            break;
        }
        idx++;
    }
    if (command_ == StompCommand::kUndefined) {
        state_ = StompError::kParsing;
    }
    return idx + 1;
}

uint32_t StompFrame::ParseHeader(const std::string& frame, uint32_t idx) {
    auto startIdx = idx;
    auto curIdx = idx;
    while (curIdx < frame.size() && frame[curIdx] != ':' && frame[curIdx] != '\n') {
        curIdx++;
    }
    StompHeader header = StompHeader::kUndefined;
    if (curIdx < frame.size() && frame[curIdx] == ':') {
        auto maybeHeader = strToHeader(frame_.substr(startIdx, curIdx-startIdx));
        if (maybeHeader.has_value()) {
            header = maybeHeader.value();
        }
    }
    if (header == StompHeader::kUndefined) {
        state_ = StompError::kParsing;
        return curIdx + 1;
    }
    curIdx++;
    startIdx = curIdx;
    while (curIdx < frame.size() && frame[curIdx] != '\n') {
        curIdx++;
    }
    if (curIdx >= frame.size() || frame[curIdx] != '\n' || curIdx == startIdx) {
        state_ = StompError::kParsing;
    } else {
        if (headers_.find(header) == headers_.end()) {
            headers_[header] = std::string_view(frame).substr(startIdx, curIdx-startIdx);
        } else {
            state_ = StompError::kParsing;
        }
    }
    return curIdx + 1;
}

uint32_t StompFrame::ParseHeaders(const std::string& frame, uint32_t idx) {
    auto curIdx = idx;
    while (curIdx < frame.size() && frame[curIdx] != '\n') {
        curIdx = ParseHeader(frame, curIdx);
        if (state_== StompError::kParsing) {
            return curIdx + 1;
        }
    }
    if (curIdx >= frame.size() || frame[curIdx] != '\n') {
        state_ = StompError::kParsing;
    }

    if (command_ == StompCommand::kSubscribe) {
        if (headers_.find(StompHeader::kAck) == headers_.end()) {
            headers_[StompHeader::kAck] = "auto";
        }
    }
    return curIdx + 1;
}

uint32_t StompFrame::ParseBody(const std::string& frame, uint32_t idx) {
    auto startIdx = idx;
    auto curIdx = idx;
    while (curIdx < frame.size() && frame[curIdx] != '\0') {
        curIdx++;
    }
    if (curIdx >= frame.size() || frame[curIdx] != '\0') {
        state_ = StompError::kParsing;
    } else {
        body_ = std::string_view(frame).substr(startIdx, curIdx-startIdx);
    }
    return curIdx + 1;
}

void StompFrame::ParseEol(const std::string& frame, uint32_t idx) {
    auto curIdx = idx;
    while (curIdx < frame.size() && frame[curIdx] == '\n') {
        curIdx++;
    }
    if (curIdx != frame.size()) {
        state_ = StompError::kParsing;
    }
}

bool StompFrame::HasBody() const {
    return !body_.empty();
}

void StompFrame::Validate() {
    static const std::unordered_map<StompCommand, std::vector<StompHeader>> requiredHeaders {
        {StompCommand::kConnect, {StompHeader::kAcceptVersion, StompHeader::kHost}},
        {StompCommand::kConnected, {StompHeader::kVersion}},
        {StompCommand::kSend, {StompHeader::kDestination}},
        {StompCommand::kSubscribe, {StompHeader::kDestination, StompHeader::kId}},
        {StompCommand::kUnsubscribe, {StompHeader::kId}},
        {StompCommand::kAck, {StompHeader::kId}},
        {StompCommand::kNack, {StompHeader::kId}},
        {StompCommand::kBegin, {StompHeader::kTransaction}},
        {StompCommand::kCommit, {StompHeader::kTransaction}},
        {StompCommand::kAbort, {StompHeader::kTransaction}},
        {StompCommand::kMessage, {StompHeader::kDestination, StompHeader::kMessageId, StompHeader::kSubscription}},
        {StompCommand::kReceipt, {StompHeader::kReceiptId}},
    };
    static const std::unordered_map<StompCommand, std::vector<StompHeader>> optionalHeaders {
        {StompCommand::kConnect, {StompHeader::kLogin, StompHeader::kPasscode, StompHeader::kHeartBeat}},
        {StompCommand::kConnected, {StompHeader::kSession, StompHeader::kServer, StompHeader::kHeartBeat}},
        {StompCommand::kSend, {StompHeader::kTransaction, StompHeader::kContentType}},
        {StompCommand::kSubscribe, {StompHeader::kAck}},
        {StompCommand::kMessage, {StompHeader::kContentType}},
        {StompCommand::kAck, {StompHeader::kTransaction}},
        {StompCommand::kNack, {StompHeader::kTransaction}},
        {StompCommand::kDisconnect, {StompHeader::kReceipt}},
        {StompCommand::kMessage, {StompHeader::kAck}},
        {StompCommand::kError, {StompHeader::kMessage, StompHeader::kContentType}},
    };

    auto requiredHeadersIt = requiredHeaders.find(command_);
    auto optionalHeadersIt = optionalHeaders.find(command_);
    uint32_t requiredHeadersFound = 0;
    for (auto [header, _] : headers_) {
        bool classified = false;

        if (header == StompHeader::kContentLength) {
            continue;
        }
        
        if (requiredHeadersIt != requiredHeaders.end()) {
            for (auto requiredHeader : requiredHeadersIt->second) {
                if (header == requiredHeader) {
                    classified = true;
                    requiredHeadersFound++;
                    break;
                }
            }
        }
        if (optionalHeadersIt != optionalHeaders.end()) {
            for (auto optionalHeader : optionalHeadersIt->second) {
                if (header == optionalHeader) {
                    classified = true;
                    break;
                }
            }
        }
        if (!classified) {
            state_ = StompError::kValidation;
            return;
        }
    }

    if (requiredHeadersIt != requiredHeaders.end() 
        && requiredHeadersFound != requiredHeadersIt->second.size()) {
        state_ = StompError::kValidation;
        return;
    }

    if (command_ == StompCommand::kSubscribe) {
        static const std::array<std::string, 3> validStates {
            "auto",
            "client",
            "client-individual"
        };
        if (headers_.find(StompHeader::kAck) == headers_.end()
            || std::find(
                validStates.begin(),
                validStates.end(),
                headers_[StompHeader::kAck]) == validStates.end()) {
            state_ = StompError::kValidation;
            return;
        }
    }

    if (headers_.find(StompHeader::kContentLength) != headers_.end()) {
        if (std::stoi(headers_[StompHeader::kContentLength]) != GetBody().size()) {
            state_ = StompError::kValidation;
            return;
        }
    }
}

void StompFrame::Parse(const std::string& frame) {
    if (state_ != StompError::kOk) {
        return;
    }
    auto idx = ParseCommand(frame);
    if (state_ != StompError::kOk) {
        return;
    }
    idx = ParseHeaders(frame, idx);
    if (state_ != StompError::kOk) {
        return;
    }
    idx = ParseBody(frame, idx);
    if (state_ != StompError::kOk) {
        return;
    }
    ParseEol(frame, idx);
    if (state_ != StompError::kOk) {
        return;
    }
    Validate();
    if (state_ != StompError::kOk) {
        return;
    }
}

std::string_view StompFrame::GetBody() const {
    return body_;
}

StompCommand StompFrame::GetCommand() const {
    return command_;
}

std::optional<StompCommand> StompFrame::strToCommand(const std::string& str) {
    static const std::unordered_map<std::string, StompCommand> strToCommandMap = {
        {"SEND", StompCommand::kSend},
        {"SUBSCRIBE", StompCommand::kSubscribe},
        {"UNSUBSCRIBE", StompCommand::kUnsubscribe},
        {"BEGIN", StompCommand::kBegin},
        {"COMMIT", StompCommand::kCommit},
        {"ABORT", StompCommand::kAbort},
        {"ACK", StompCommand::kAck},
        {"NACK", StompCommand::kNack},
        {"ERROR", StompCommand::kError},
        {"DISCONNECT", StompCommand::kDisconnect},
        {"CONNECT", StompCommand::kConnect},
        {"STOMP", StompCommand::kStomp},
        {"CONNECTED", StompCommand::kConnected},
        {"MESSAGE", StompCommand::kMessage},
        {"RECEIPT", StompCommand::kReceipt},
        {"SERVER_ERROR", StompCommand::kServerError},
    };
    auto it = strToCommandMap.find(str);
    if (it != strToCommandMap.end()) {
        return it->second;
    }
    return {};
}

std::optional<StompHeader> StompFrame::strToHeader(const std::string& str) {
    static const std::unordered_map<std::string, StompHeader> strToHeader = {
        {"content-length", StompHeader::kContentLength},
        {"content-type", StompHeader::kContentType},
        {"receipt", StompHeader::kReceipt},
        {"host", StompHeader::kHost},
        {"accept-version", StompHeader::kAcceptVersion},
        {"message", StompHeader::kMessage},
        {"message-id", StompHeader::kMessageId},
        {"receipt-id", StompHeader::kReceiptId},
        {"destination", StompHeader::kDestination},
        {"ack", StompHeader::kAck},
        {"subscription", StompHeader::kSubscription},
        {"id", StompHeader::kId},
        {"version", StompHeader::kVersion},
        {"transaction", StompHeader::kTransaction},
        {"session", StompHeader::kSession},
        {"login", StompHeader::kLogin},
        {"id", StompHeader::kId},
        {"passcode", StompHeader::kPasscode},
        {"server", StompHeader::kServer},
        {"heart-beat", StompHeader::kHeartBeat},
    };
    auto it = strToHeader.find(str);
    if (it != strToHeader.end()) {
        return it->second;
    }
    return {};
}

std::ostream& operator<<(std::ostream& os, const StompError& error) {
    os << "StompError: Code=" << static_cast<uint32_t>(error);
    return os;
}

std::ostream& operator<<(std::ostream& os, const StompHeader& header) {
    os << "StompHeader: Code=" << static_cast<uint32_t>(header);
    return os;
}

std::ostream& operator<<(std::ostream& os, const StompCommand& command) {
    os << "StompCommand: Code=" << static_cast<uint32_t>(command);
    return os;
}

}