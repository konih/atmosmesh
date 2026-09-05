#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// HLK-LD2410S report frames (serial protocol V1.00, section 2.1). Little-endian throughout.
//
// Minimal (the factory default):  6E · state · distance cm (2) · 62
// Standard (upper-computer tool): F4 F3 F2 F1 · length (2) · type 01 · state · distance cm (2)
//                                 · reserved (2) · 64 gate energies · F8 F7 F6 F5
// state 0/1 = nobody, 2/3 = somebody. Command/ACK frames (FD FC FB FA … 04 03 02 01) are not
// reports and are skipped like any other noise.
inline constexpr std::uint8_t kLd2410sMinimalHead = 0x6E;
inline constexpr std::uint8_t kLd2410sMinimalTail = 0x62;
inline constexpr std::size_t kLd2410sMinimalFrameBytes = 5U;
inline constexpr std::uint8_t kLd2410sStandardType = 0x01;
// Farthest gate is 16 x 0.7 m; anything past 12 m is a corrupt frame, not a target.
inline constexpr std::uint16_t kLd2410sMaxDistanceCm = 1200U;

struct Ld2410sReport {
    bool ok = false;
    bool occupied = false;
    std::uint8_t state = 0;
    std::uint16_t distance_cm = 0;
    bool standard = false;   // true when decoded from the standard (energy) frame
};

// Feeds one byte at a time and resynchronises on any mismatch by dropping the oldest byte, so a
// UART that starts listening mid-frame, or a stray boot-log byte, costs at most one report.
class Ld2410sStream {
public:
    Ld2410sReport feed(std::uint8_t byte);
    void reset();

private:
    static constexpr std::size_t kCapacity = 96U;   // a standard frame is 80 bytes
    std::uint8_t buffer_[kCapacity] = {};
    std::size_t size_ = 0;

    void drop_front();
};

}  // namespace atmosmesh
