#include "atmosmesh/ld2410s_frame.hpp"

#include <cstring>

namespace atmosmesh {
namespace {

constexpr std::uint8_t kStandardHead[4] = {0xF4, 0xF3, 0xF2, 0xF1};
constexpr std::uint8_t kStandardTail[4] = {0xF8, 0xF7, 0xF6, 0xF5};
constexpr std::uint8_t kCommandHead[4] = {0xFD, 0xFC, 0xFB, 0xFA};
constexpr std::uint8_t kCommandTail[4] = {0x04, 0x03, 0x02, 0x01};
constexpr std::size_t kStandardHeaderBytes = 6U;   // head (4) + length (2)
constexpr std::size_t kStandardMaxDataBytes = 80U;
constexpr std::size_t kCommandMaxDataBytes = 40U;

Ld2410sReport report_from(std::uint8_t state, std::uint16_t distance_cm, bool standard) {
    Ld2410sReport report;
    report.ok = true;
    report.state = state;
    report.occupied = state >= 2U;
    report.distance_cm = distance_cm;
    report.standard = standard;
    return report;
}

// Shared by the standard-report and command branches: both are head (4) · length (2) · data ·
// tail (4). Returns 0 = need more bytes, 1 = complete and well-formed, -1 = drop the front byte.
int framed_status(const std::uint8_t* buf, std::size_t size, const std::uint8_t* head,
                  const std::uint8_t* tail, std::size_t max_data, std::size_t cap,
                  std::size_t* total_out) {
    const std::size_t have = size < 4U ? size : 4U;
    for (std::size_t i = 0; i < have; ++i) {
        if (buf[i] != head[i]) {
            return -1;
        }
    }
    if (size < kStandardHeaderBytes) {
        return 0;
    }
    const std::size_t length = static_cast<std::size_t>(buf[4] | (buf[5] << 8));
    if (length < 2U || length > max_data) {
        return -1;
    }
    const std::size_t total = kStandardHeaderBytes + length + 4U;
    if (total > cap) {
        return -1;
    }
    if (size < total) {
        return 0;
    }
    if (std::memcmp(buf + kStandardHeaderBytes + length, tail, 4U) != 0) {
        return -1;
    }
    *total_out = total;
    return 1;
}

}  // namespace

std::uint16_t ld2410s_ack_status(const Ld2410sAck& ack) {
    if (!ack.ok || ack.value_len < 2U) {
        return 0xFFFFU;
    }
    return static_cast<std::uint16_t>(ack.value[0] | (ack.value[1] << 8));
}

std::size_t ld2410s_build_command(std::uint8_t* out, std::size_t cap, std::uint16_t command,
                                  const std::uint8_t* value, std::size_t value_len) {
    const std::size_t total = 4U + 2U + 2U + value_len + 4U;
    if (out == nullptr || total > cap || (value == nullptr && value_len > 0U)) {
        return 0;
    }
    std::memcpy(out, kCommandHead, 4U);
    const std::size_t length = 2U + value_len;
    out[4] = static_cast<std::uint8_t>(length & 0xFFU);
    out[5] = static_cast<std::uint8_t>(length >> 8);
    out[6] = static_cast<std::uint8_t>(command & 0xFFU);
    out[7] = static_cast<std::uint8_t>(command >> 8);
    if (value_len > 0U) {
        std::memcpy(out + 8, value, value_len);
    }
    std::memcpy(out + 8 + value_len, kCommandTail, 4U);
    return total;
}

void Ld2410sStream::reset() {
    size_ = 0;
}

void Ld2410sStream::drop_front() {
    if (size_ == 0) {
        return;
    }
    std::memmove(buffer_, buffer_ + 1, size_ - 1);
    --size_;
}

Ld2410sReport Ld2410sStream::feed(std::uint8_t byte) {
    if (size_ >= kCapacity) {
        drop_front();
    }
    buffer_[size_++] = byte;

    while (size_ > 0) {
        const std::uint8_t head = buffer_[0];

        if (head == kLd2410sMinimalHead) {
            if (size_ < kLd2410sMinimalFrameBytes) {
                return {};   // wait for the rest
            }
            const std::uint8_t state = buffer_[1];
            const std::uint16_t distance =
                static_cast<std::uint16_t>(buffer_[2] | (buffer_[3] << 8));
            if (buffer_[4] == kLd2410sMinimalTail && state <= 3U &&
                distance <= kLd2410sMaxDistanceCm) {
                size_ = 0;
                return report_from(state, distance, false);
            }
            drop_front();
            continue;
        }

        if (head == kStandardHead[0]) {
            std::size_t total = 0;
            const int st = framed_status(buffer_, size_, kStandardHead, kStandardTail,
                                         kStandardMaxDataBytes, kCapacity, &total);
            if (st == 0) {
                return {};
            }
            if (st > 0) {
                const std::size_t length = total - kStandardHeaderBytes - 4U;
                const std::uint8_t type = buffer_[6];
                const std::uint8_t state = buffer_[7];
                const std::uint16_t distance =
                    static_cast<std::uint16_t>(buffer_[8] | (buffer_[9] << 8));
                if (length >= 6U && type == kLd2410sStandardType && state <= 3U &&
                    distance <= kLd2410sMaxDistanceCm) {
                    size_ = 0;
                    return report_from(state, distance, true);
                }
            }
            drop_front();
            continue;
        }

        if (head == kCommandHead[0]) {
            std::size_t total = 0;
            const int st = framed_status(buffer_, size_, kCommandHead, kCommandTail,
                                         kCommandMaxDataBytes, kCapacity, &total);
            if (st == 0) {
                return {};
            }
            if (st > 0) {
                const std::uint16_t word = static_cast<std::uint16_t>(buffer_[6] | (buffer_[7] << 8));
                if ((word & kLd2410sAckFlag) != 0U) {
                    Ld2410sReport report;
                    report.ack.ok = true;
                    report.ack.command = static_cast<std::uint16_t>(word & ~kLd2410sAckFlag);
                    const std::size_t value_len = total - kStandardHeaderBytes - 4U - 2U;
                    report.ack.value_len =
                        value_len < kLd2410sAckValueMax ? value_len : kLd2410sAckValueMax;
                    std::memcpy(report.ack.value, buffer_ + 8, report.ack.value_len);
                    size_ = 0;
                    return report;
                }
                // A command echoed back (or our own TX seen on RX): not a report, not an ACK.
                size_ = 0;
                return {};
            }
            drop_front();
            continue;
        }

        drop_front();
    }
    return {};
}

}  // namespace atmosmesh
