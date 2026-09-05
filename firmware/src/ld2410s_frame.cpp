#include "atmosmesh/ld2410s_frame.hpp"

#include <cstring>

namespace atmosmesh {
namespace {

constexpr std::uint8_t kStandardHead[4] = {0xF4, 0xF3, 0xF2, 0xF1};
constexpr std::uint8_t kStandardTail[4] = {0xF8, 0xF7, 0xF6, 0xF5};
constexpr std::size_t kStandardHeaderBytes = 6U;   // head (4) + length (2)
constexpr std::size_t kStandardMaxDataBytes = 80U;

Ld2410sReport report_from(std::uint8_t state, std::uint16_t distance_cm, bool standard) {
    Ld2410sReport report;
    report.ok = true;
    report.state = state;
    report.occupied = state >= 2U;
    report.distance_cm = distance_cm;
    report.standard = standard;
    return report;
}

}  // namespace

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
            const std::size_t have = size_ < 4U ? size_ : 4U;
            bool head_ok = true;
            for (std::size_t i = 0; i < have; ++i) {
                if (buffer_[i] != kStandardHead[i]) {
                    head_ok = false;
                    break;
                }
            }
            if (!head_ok) {
                drop_front();
                continue;
            }
            if (size_ < kStandardHeaderBytes) {
                return {};
            }
            const std::size_t length = static_cast<std::size_t>(buffer_[4] | (buffer_[5] << 8));
            if (length < 6U || length > kStandardMaxDataBytes) {
                drop_front();
                continue;
            }
            const std::size_t total = kStandardHeaderBytes + length + 4U;
            if (total > kCapacity) {
                drop_front();
                continue;
            }
            if (size_ < total) {
                return {};
            }
            const std::uint8_t* tail = buffer_ + kStandardHeaderBytes + length;
            const std::uint8_t type = buffer_[6];
            const std::uint8_t state = buffer_[7];
            const std::uint16_t distance =
                static_cast<std::uint16_t>(buffer_[8] | (buffer_[9] << 8));
            if (std::memcmp(tail, kStandardTail, 4U) == 0 && type == kLd2410sStandardType &&
                state <= 3U && distance <= kLd2410sMaxDistanceCm) {
                size_ = 0;
                return report_from(state, distance, true);
            }
            drop_front();
            continue;
        }

        drop_front();
    }
    return {};
}

}  // namespace atmosmesh
