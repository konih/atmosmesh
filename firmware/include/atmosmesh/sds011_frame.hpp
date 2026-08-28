#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace atmosmesh {

inline constexpr std::size_t kSds011FrameSize = 10;
inline constexpr std::uint8_t kSds011Head = 0xAA;
inline constexpr std::uint8_t kSds011QueryCmd = 0xC0;
inline constexpr std::uint8_t kSds011Tail = 0xAB;

struct Sds011Sample {
    float pm25_ug_m3;
    float pm10_ug_m3;
    bool ok;
};

// Nova SDS011 query-data frame: AA C0 PM25L PM25H PM10L PM10H ID1 ID2 CRC AB.
// CRC is the low 8 bits of the sum of bytes 2..7.
bool sds011_checksum_ok(const std::uint8_t bytes[kSds011FrameSize]);

Sds011Sample parse_sds011_frame(const std::uint8_t* bytes);

// Assembles frames from a UART byte stream; ignores noise and incomplete packets.
class Sds011Stream {
public:
    Sds011Sample feed(std::uint8_t byte);

private:
    // Ten buffered bytes that do not parse mean the stream desynchronized. Discarding all ten
    // would swallow whatever real header sits among them and lose the following report too, so
    // the buffer is re-anchored onto the earliest plausible header at or after `start`.
    void resync_after(std::size_t start);

    std::uint8_t buf_[kSds011FrameSize]{};
    std::size_t filled_{0};
};

std::string format_sds011_listen_log();
std::string format_sds011_no_frame_log();

}  // namespace atmosmesh
