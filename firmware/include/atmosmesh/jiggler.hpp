#pragma once
#include <cstdint>

namespace atmosmesh {
// HOGP mouse with three unused buttons. A buttonless X/Y-only map made macOS
// drop the link and bounce Bluetooth during pairing.
inline constexpr std::uint8_t kMouseReportMap[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x01,
    0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03,
    0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x02,
    0x81, 0x06, 0xc0, 0xc0
};
// Host-testable button and motion policy. Connection means an encrypted, subscribed HID host.
class Jiggler {
public:
    bool enabled() const { return enabled_; }
    std::int8_t update(std::uint32_t now, bool connected, bool pressed) {
        if (!initialized_) {
            initialized_ = true;
            raw_ = stable_ = pressed;
            edge_ = now;
        }
        if (pressed != raw_) {
            raw_ = pressed;
            edge_ = now;
        }
        if (now - edge_ >= 30 && stable_ != raw_) {
            stable_ = raw_;
            if (stable_ && connected) {
                enabled_ = !enabled_;
                pending_ = false;
                cycle_ = now;
            }
        }
        if (!connected) {
            enabled_ = false;
            pending_ = false;
        }
        if (!enabled_) return 0;
        if (pending_) {
            if (now - outbound_ < 80) return 0;
            pending_ = false;
            // Late loops never emit a catch-up burst.
            if (now - cycle_ >= 15000) cycle_ = now;
            return -1;
        }
        if (now - cycle_ < 15000) return 0;
        cycle_ = outbound_ = now;
        pending_ = true;
        return 1;
    }
private:
    bool initialized_ = false;
    bool raw_ = false;
    bool stable_ = false;
    bool enabled_ = false;
    bool pending_ = false;
    std::uint32_t edge_ = 0, cycle_ = 0, outbound_ = 0;
};
}

#include <atomic>
namespace atmosmesh {
// BLE callbacks publish link state; only the Arduino loop touches motion state.
class JigglerHid {
public:
    void authenticated(bool value) { encrypted_.store(value); }
    void subscribed(bool value) {
        subscribed_.store(value);
        if (!value) lost_.store(true);
    }
    void disconnected() {
        encrypted_.store(false);
        subscribed_.store(false);
        lost_.store(true);
    }
    bool ready() const { return encrypted_.load() && subscribed_.load(); }
    bool enabled() const { return motion_.enabled(); }
    // False means transport failure; caller logs it. Report is buttons=0, X, Y.
    template <typename Sender>
    bool update(std::uint32_t now, bool pressed, Sender send) {
        if (lost_.exchange(false)) motion_.update(now, false, pressed);
        const auto dx = motion_.update(now, ready(), pressed);
        if (dx == 0) return true;
        const std::uint8_t report[] = {0, static_cast<std::uint8_t>(dx), 0};
        if (send(report, sizeof(report))) return true;
        motion_.update(now, false, pressed);
        return false;
    }
private:
    Jiggler motion_;
    std::atomic<bool> encrypted_{false}, subscribed_{false}, lost_{false};
};
}
