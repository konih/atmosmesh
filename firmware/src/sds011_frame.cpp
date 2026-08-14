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

    filled_ = 0;
    return parse_sds011_frame(buf_);
}

}  // namespace atmosmesh
