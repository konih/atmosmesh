#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "atmosmesh/jiggler.hpp"

#ifndef ATMOSMESH_JIGGLER_BUTTON_GPIO
#define ATMOSMESH_JIGGLER_BUTTON_GPIO 3
#endif
static_assert(ATMOSMESH_JIGGLER_BUTTON_GPIO == 3 || ATMOSMESH_JIGGLER_BUTTON_GPIO == 4,
              "Use free GPIO3 or GPIO4 for the external normally-open button");
namespace {
constexpr int kButton = ATMOSMESH_JIGGLER_BUTTON_GPIO;
constexpr int kBootButton = 9;
U8G2_SSD1306_72X40_ER_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
atmosmesh::JigglerHid jiggler;
NimBLECharacteristic* input = nullptr;
bool display_ok = false;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
        Serial.printf("jiggler: connect handle=%u\n", info.getConnHandle());
        if (!info.isEncrypted()) NimBLEDevice::startSecurity(info.getConnHandle());
    }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
        Serial.printf("jiggler: disconnect reason=0x%02x\n", reason);
        jiggler.disconnected();
    }
    void onAuthenticationComplete(NimBLEConnInfo& info) override {
        Serial.printf("jiggler: auth enc=%d bonded=%d\n",
                      info.isEncrypted(), info.isBonded());
        jiggler.authenticated(info.isEncrypted());
    }
} server_callbacks;
class InputCallbacks : public NimBLECharacteristicCallbacks {
    void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo&, uint16_t value) override {
        Serial.printf("jiggler: subscribe=%u\n", value);
        jiggler.subscribed((value & 1) != 0);
    }
} input_callbacks;

void draw(bool ready) {
    if (!display_ok) return;
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x7_tf);
    oled.drawStr(0, 8, "JIGGLER");
    oled.drawStr(0, 19, ready ? "BT connected" : "BT pairing...");
    oled.drawStr(0, 30, jiggler.enabled() ? "ON  tiny move" : "OFF");
    oled.drawStr(0, 39, "Press: toggle");
    oled.sendBuffer();
}
}

void setup() {
    Serial.begin(115200);
    pinMode(kButton, INPUT_PULLUP);
    pinMode(kBootButton, INPUT_PULLUP);
    // Initialize policy with actual button state: a held boot button must not enable motion.
    jiggler.update(millis(), digitalRead(kButton) == LOW || digitalRead(kBootButton) == LOW,
                   [](const uint8_t*, unsigned) { return false; });
    Wire.begin(5, 6);
    Wire.setTimeOut(25);
    Wire.beginTransmission(0x3c);
    display_ok = Wire.endTransmission() == 0;
    if (display_ok) {
        // Pass no pins to U8g2: Wire already owns the Spot OLED's GPIO5/6 bus.
        oled.begin();
        draw(false);
    }
    NimBLEDevice::init("AtmosMesh Jiggler");
    // Bonding + Just Works. Secure Connections made Linux/macOS request a
    // passkey confirm the board cannot show, which bounced Mac Bluetooth.
    NimBLEDevice::setSecurityAuth(true, false, false);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    auto* server = NimBLEDevice::createServer();
    server->setCallbacks(&server_callbacks);
    server->advertiseOnDisconnect(true);
    auto* hid = new NimBLEHIDDevice(server);
    input = hid->getInputReport(1);
    input->setCallbacks(&input_callbacks);
    hid->setManufacturer("AtmosMesh");
    hid->setPnp(0x02, 0x303A, 0x6A01, 0x0100);
    hid->setHidInfo(0, 0x02);
    hid->setReportMap(const_cast<uint8_t*>(atmosmesh::kMouseReportMap),
                      sizeof(atmosmesh::kMouseReportMap));
    hid->setBatteryLevel(100);
    hid->startServices();
    auto* adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_MOUSE);
    adv->addServiceUUID(hid->getHidService()->getUUID());
    adv->setName("AtmosMesh Jiggler");
    adv->enableScanResponse(true);
    if (!adv->start()) Serial.println("jiggler: advertising failed");
    Serial.printf("jiggler: ready oled=%s button=GPIO%d boot=GPIO9 mode=off\n",
                  display_ok ? "ok" : "missing", kButton);
}

void loop() {
    const auto now = millis();
    const bool pressed = digitalRead(kButton) == LOW || digitalRead(kBootButton) == LOW;
    if (!jiggler.update(now, pressed, [](const uint8_t* data, unsigned size) {
            return input->notify(data, size);
        })) Serial.println("jiggler: notify failed; disabled");
    const bool ready = jiggler.ready();
    static uint32_t last_display = 0;
    static bool last_enabled = false, last_ready = false;
    if (now - last_display >= 500 || last_enabled != jiggler.enabled() || last_ready != ready) {
        if (last_enabled != jiggler.enabled() || last_ready != ready)
            Serial.printf("jiggler: connected=%d enabled=%d\n", ready, jiggler.enabled());
        draw(ready);
        last_display = now;
        last_enabled = jiggler.enabled();
        last_ready = ready;
    }
    delay(2);
}
