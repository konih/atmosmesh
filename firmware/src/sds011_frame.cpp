#include "atmosmesh/sds011_frame.hpp"

namespace atmosmesh {

bool sds011_checksum_ok(const std::uint8_t bytes[kSds011FrameSize]) {
    if (bytes == nullptr) {
        return false;
    }
    std::uint16_t sum = 0;
    for (std::size_t i = 2; i <= 7; ++i) {
        sum = static_cast<std::uint16_t>(sum + bytes[i]);
    }
    return static_cast<std::uint8_t>(sum) == bytes[8];
}

Sds011Sample parse_sds011_frame(const std::uint8_t* bytes) {
    Sds011Sample sample{};
    if (bytes == nullptr || bytes[0] != kSds011Head || bytes[1] != kSds011QueryCmd ||
        bytes[9] != kSds011Tail || !sds011_checksum_ok(bytes)) {
        return sample;
    }
    const int pm25_raw = (static_cast<int>(bytes[3]) << 8) | bytes[2];
    const int pm10_raw = (static_cast<int>(bytes[5]) << 8) | bytes[4];
    sample.pm25_ug_m3 = static_cast<float>(pm25_raw) / 10.0F;
    sample.pm10_ug_m3 = static_cast<float>(pm10_raw) / 10.0F;
    sample.ok = true;
    return sample;
}

Sds011Sample Sds011Stream::feed(std::uint8_t byte) {
    if (filled_ == 0) {
        if (byte != kSds011Head) {
            return {};
        }
        buf_[filled_++] = byte;
        return {};
    }

    if (filled_ == 1 && byte != kSds011QueryCmd) {
        filled_ = 0;
        if (byte == kSds011Head) {
            buf_[filled_++] = byte;
        }
        return {};
    }

    buf_[filled_++] = byte;
    if (filled_ < kSds011FrameSize) {
        return {};
    }

    const Sds011Sample sample = parse_sds011_frame(buf_);
    if (sample.ok) {
        filled_ = 0;
        return sample;
    }

    // A dropped byte or a truncated report leaves a real header sitting inside these ten bytes.
    // Throwing the whole buffer away costs the next report as well, so re-anchor instead.
    resync_after(1);
    return {};
}

void Sds011Stream::resync_after(std::size_t start) {
    std::size_t from = start;
    while (from < filled_) {
        std::size_t head = from;
        while (head < filled_ && buf_[head] != kSds011Head) {
            ++head;
        }
        if (head >= filled_) {
            break;
        }
        // If the command byte is already in hand and is wrong, this 0xAA was payload, not a head.
        if (filled_ - head >= 2 && buf_[head + 1] != kSds011QueryCmd) {
            from = head + 1;
            continue;
        }
        const std::size_t kept = filled_ - head;
        for (std::size_t i = 0; i < kept; ++i) {
            buf_[i] = buf_[head + i];
        }
        filled_ = kept;
        return;
    }
    filled_ = 0;
}

std::string format_sds011_listen_log() {
    return "sds011: listen GPIO16 (RX2); TX2=GPIO17 is ESP output — sensor TX goes to RX2";
}

std::string format_sds011_no_frame_log() {
    return "sds011: no AA C0 frame (listening GPIO16/RX2; sensor TX must not sit on TX2/GPIO17)";
}

}  // namespace atmosmesh
