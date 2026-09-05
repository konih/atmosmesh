#pragma once

#include <cstddef>
#include <cstdint>

namespace atmosmesh {

// HLK-LD2410S report frames (serial protocol V1.00, section 2.1). Little-endian throughout.
//
// Minimal (the factory default):  6E · state · distance cm (2) · 62
// Standard (upper-computer tool): F4 F3 F2 F1 · length (2) · type 01 · state · distance cm (2)
//                                 · reserved (2) · 64 gate energies · F8 F7 F6 F5
// state 0/1 = nobody, 2/3 = somebody.
//
// Command frames (section 2.2): FD FC FB FA · length (2) · command word (2) · value … ·
// 04 03 02 01. The module answers with the same shape, the command word with bit 8 set
// (FF 00 → FF 01), followed by the returned value: a 2-byte status (0 = success) for most
// commands, the 3 × 2-byte version for read-version. While in configuration mode (after an
// enable-configuration ACK) the module stops reporting until end-configuration or a 3 s timeout.
inline constexpr std::uint8_t kLd2410sMinimalHead = 0x6E;
inline constexpr std::uint8_t kLd2410sMinimalTail = 0x62;
inline constexpr std::size_t kLd2410sMinimalFrameBytes = 5U;
inline constexpr std::uint8_t kLd2410sStandardType = 0x01;
// Farthest gate is 16 x 0.7 m; anything past 12 m is a corrupt frame, not a target.
inline constexpr std::uint16_t kLd2410sMaxDistanceCm = 1200U;

inline constexpr std::uint16_t kLd2410sCmdEnableConfig = 0x00FF;
inline constexpr std::uint16_t kLd2410sCmdEndConfig = 0x00FE;
inline constexpr std::uint16_t kLd2410sCmdReadVersion = 0x0000;
inline constexpr std::uint16_t kLd2410sCmdOutputMode = 0x007A;
inline constexpr std::uint16_t kLd2410sAckFlag = 0x0100;
inline constexpr std::size_t kLd2410sAckValueMax = 16U;
inline constexpr std::size_t kLd2410sCommandMaxBytes = 32U;

// Value for kLd2410sCmdOutputMode: minimal (factory default) or standard report frames.
inline constexpr std::uint8_t kLd2410sOutputMinimal[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
inline constexpr std::uint8_t kLd2410sOutputStandard[6] = {0x00, 0x00, 0x01, 0x00, 0x00, 0x00};

struct Ld2410sAck {
    bool ok = false;
    std::uint16_t command = 0;   // the command word as sent, without the ACK flag
    std::uint8_t value[kLd2410sAckValueMax] = {};
    std::size_t value_len = 0;
};

// The 2-byte status most ACKs start with; 0 = success. Meaningless for read-version.
std::uint16_t ld2410s_ack_status(const Ld2410sAck& ack);

// Builds a command frame into `out`; returns the byte count, or 0 when it does not fit.
std::size_t ld2410s_build_command(std::uint8_t* out, std::size_t cap, std::uint16_t command,
                                  const std::uint8_t* value, std::size_t value_len);

struct Ld2410sReport {
    bool ok = false;         // a report (presence) frame completed on this byte
    bool occupied = false;
    std::uint8_t state = 0;
    std::uint16_t distance_cm = 0;
    bool standard = false;   // true when decoded from the standard (energy) frame
    Ld2410sAck ack;          // ack.ok when an ACK frame completed on this byte instead
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
