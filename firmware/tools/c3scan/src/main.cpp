// I2C pin-pair scanner for the ESP32-C3 SuperMini OLED board.
// Tries the plausible SDA/SCL pairs, scans 0x08-0x77 on each, prints what answers.
// With nothing plugged into the module the only expected hit is the on-board OLED at 0x3C.
#include <Arduino.h>
#include <Wire.h>

struct Pair { int sda; int scl; const char* note; };
static const Pair kPairs[] = {
    {5, 6, "expected for the 0.42in OLED boards"},
    {6, 5, "swapped"},
    {4, 3, ""}, {3, 4, ""},
    {7, 10, ""}, {10, 7, ""},
    {2, 1, ""}, {1, 2, ""},
    {0, 1, ""}, {1, 0, ""},
    {20, 21, "UART0 pins"}, {21, 20, "UART0 pins"},
    {8, 9, "strapping pins"}, {9, 8, "strapping pins"},
};

static int scanPair(const Pair& p) {
    Wire.end();
    Wire.begin(p.sda, p.scl, 100000);
    Wire.setTimeOut(50);
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; ++addr) {
        Wire.beginTransmission(addr);
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            Serial.printf("  SDA=GPIO%d SCL=GPIO%d -> device at 0x%02X%s\n", p.sda, p.scl, addr,
                          addr == 0x3C ? " (SSD1306-class OLED)" : addr == 0x44 ? " (SHT4x)" :
                          addr == 0x10 ? " (VEML7700)" : addr == 0x76 ? " (BME/BMP280)" : "");
            ++found;
        }
    }
    Wire.end();
    return found;
}

void setup() {
    Serial.begin(115200);
    delay(2500);  // give the USB CDC host time to attach
    Serial.println();
    Serial.println("atmosmesh c3scan: ESP32-C3 SuperMini OLED I2C pin-pair scan");
    Serial.printf("chip: %s rev %d, %d core(s), flash %u MB, MAC %012llX\n", ESP.getChipModel(),
                  ESP.getChipRevision(), ESP.getChipCores(), ESP.getFlashChipSize() / (1024 * 1024),
                  ESP.getEfuseMac());
}

void loop() {
    Serial.println("--- scan pass ---");
    int total = 0;
    for (const Pair& p : kPairs) {
        int n = scanPair(p);
        if (n == 0) {
            Serial.printf("  SDA=GPIO%d SCL=GPIO%d -> nothing", p.sda, p.scl);
            if (p.note[0]) Serial.printf("  [%s]", p.note);
            Serial.println();
        }
        total += n;
    }
    Serial.printf("--- %d device(s) total; next pass in 15 s ---\n", total);
    delay(15000);
}
