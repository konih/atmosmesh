#pragma once
#include <cstdint>

namespace atmosmesh {
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
    // False means transport failure; caller logs it. Sender consumes exactly X/Y bytes.
    template <typename Sender>
    bool update(std::uint32_t now, bool pressed, Sender send) {
        if (lost_.exchange(false)) motion_.update(now, false, pressed);
        const auto dx = motion_.update(now, ready(), pressed);
        if (dx == 0) return true;
        const std::uint8_t report[] = {static_cast<std::uint8_t>(dx), 0};
        if (send(report, sizeof(report))) return true;
        motion_.update(now, false, pressed);
        return false;
    }
private:
    Jiggler motion_;
    std::atomic<bool> encrypted_{false}, subscribed_{false}, lost_{false};
};
}
