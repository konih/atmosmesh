# Bench incident 2026-08-17 — DevKit LDO released smoke

> **RESOLVED 2026-08-17.** Operator replaced the AMS1117. Board fully recovered: clean
> `POWERON_RESET`, one boot banner, second-stage bootloader loads, application runs. The ESP32
> die, flash and core clock are all healthy — `ESP32-D0WDQ6 rev v1.0`, MAC `ac:67:b2:37:26:78`.
> The board is usable. The **original wiring fault was never identified**, so nothing here
> exonerates the wiring — see [Still open](#still-open) before reconnecting sensors.
> See [Outcome](#outcome) at the end.

Operator report: visible smoke from the "big flat transistor" on the ESP32 DevKit after a
suspected wiring mistake. No sensors attached at diagnosis time. Board still on USB.

The part described is the **AMS1117-3.3** (SOT-223: three legs plus a large flat tab, next to
the USB connector). It is the onboard 5 V → 3.3 V LDO, not a transistor. See
[power.md](power.md) — this is the regulator every 3.3 V load in the design hangs off.

## Diagnosis (software only, over `/dev/cu.usbserial-0001`)

| Check | Result |
| --- | --- |
| USB-UART bridge enumerates | Yes — `/dev/cu.usbserial-0001` present |
| `esptool.py chip_id` (download mode) | **Fails** — "No serial data received" |
| Raw listen @ 115200, no reset asserted | ROM banner present, but **6 boot banners in 10 s** |
| Reset reason, every cycle | `rst:0x10 (RTCWDT_RTC_RESET), boot:0x13 (SPI_FAST_FLASH_BOOT)` |
| Second-stage bootloader output | **Never reached** |

The USB-UART bridge is powered straight from USB 5 V, so its enumeration proves nothing about
the 3.3 V rail. It only proves the USB connector and the bridge survived.

### Supporting evidence: the ROM output is not byte-stable

An ESP32 with erased or invalid flash also loops on `RTCWDT_RTC_RESET`, so the loop by itself
is not diagnostic. Alongside it, the ROM's output is **not reproducible byte-for-byte**
between cycles:

```text
ets Jun  8 2016 00:22:57      <- correct
evs Jun  8 2016 00:22:57      <- 't' (0x74) -> 'v' (0x76), one bit flipped
rst:0x1800000000 ...          <- corrupted field
SPIWPv0xee                    <- ':' -> 'v'
```

and the ROM additionally dumped fragments of its own `.rodata` string table over the UART:

```text
 ets %s %s
 rst:0x%x (%s),boot:0x%x (%s)
 waiting for sdio host
 waiting for downloa
 SDIO_REI_FEO_V1_BOOT ATE_BOOT
```

**This is suggestive, not proof.** The chip resets roughly every 1.5 s, and every reset chops
the UART mid-frame. A receiver that loses framing on a truncated stream reconstructs bytes that
look exactly like single-bit flips, and `waiting for sdio host` / `SDIO_REI_FEO_V1_BOOT` /
`ATE_BOOT` are legitimate ROM boot-mode strings. So the corruption is *consistent with* an
out-of-spec rail but does not rule out a healthy chip that simply never gets a valid image.

## Conclusion

The ESP32 die is **not** confirmed dead — it boots far enough to talk every cycle. Software has
now been taken as far as it goes; it cannot discriminate between these, and they need opposite
repairs:

| Candidate | Story | ESP32 likely |
| --- | --- | --- |
| LDO open / sagging | Rail dips as the core draws current → brown-out loop | Fine |
| LDO pass element shorted | ~5 V onto the 3.3 V rail, above the 3.6 V abs-max | **Being damaged now** |
| Strapping pin miswired | GPIO0/2/12/15 held wrong → boot loop with ROM boot-mode chatter | Fine |
| ~~Downstream short~~ | ~~Something on `3V3` shorted; the LDO vented as the *victim*~~ — **excluded**: `3V3`↔`GND` later measured 1.3 kΩ, see [Rail resistance](#rail-resistance-measured-after-the-repair) | Fine |

Note the last two: a wiring mistake that shorted the rail or pulled a strapping pin explains
everything observed **with the die intact**. In particular GPIO12 (MTDI) held high tells the
ROM the flash is 1.8 V, which produces exactly this loop on a 3.3 V board. Smoke proves current
went somewhere it should not have; it does not prove the ESP32 was over-volted.

Only the shorted-LDO case is actively destructive — and because it cannot be excluded from
software, the board comes off USB until a meter says otherwise.

## Actions

Do them in this order. Steps 1–2 are unpowered; do not re-apply USB until step 3.

1. **Unplug the board from USB.** Confirm the smoked part's marking while it is out — expect
   `AMS1117-3.3` on the SOT-223 next to the USB connector. If the marking is something else,
   this whole analysis needs revisiting.
2. **Unpowered, resistance mode: `3V3` ↔ `GND`.** You are *not* measuring a resistor here — the
   path is ESD/substrate diodes in the ESP32, the flash, the LDO, and (on most DevKits) the
   power LED with its ~1 kΩ series resistor. All nonlinear, all meter-dependent. So read the
   result by band, not by an exact expected value:

   | Reading | Verdict |
   | --- | --- |
   | **< ~10 Ω** | Real short. Find it before fitting anything. |
   | **~10–200 Ω** | Suspicious. Worth chasing. |
   | **> ~500 Ω** | Normal. At 3.3 V this is ≤ 6.6 mA — not a fault. |

   Measured 2026-08-17 on this board: **1.3 kΩ bare, 1.0 kΩ with sensors. Both normal.**
   Sanity-check any reading with Ohm's law before calling it a short: 3.3 V / 1.3 kΩ = 2.5 mA,
   which is 8 mW — the LDO would not even feel it. Also **swap the probes and re-measure**: a
   genuine resistive short reads the same in both directions, while a semiconductor path reads
   very differently. An asymmetric reading means you are seeing junctions, which is expected.
3. **Only if step 2 is clear:** apply USB *briefly*, DC volts, `3V3` ↔ `GND`, then unplug again.
   - 3.25–3.35 V → rail is fine; the fault is not the regulator, go to step 5.
   - ~0 V → LDO open. Chip most likely fine, board needs replacing.
   - Above ~3.6 V (typically ~5 V) → LDO shorted. Treat the ESP32 and flash as suspect too.
4. Re-read the wiring against [power.md](power.md). The rules most likely to have been broken:
   - `3V3` must never be fed from an external 5 V source, and never paralleled with the SANMIM
     3.3 V AC/DC (power.md line 26).
   - 5 V loads — MQ135 heater, SDS011 — must not hang off the `3V3` pin (power.md line 74).
     The MQ135 heater alone is ~190 mA and would cook an AMS1117 fed the wrong way.
   - `3V3_5V_SEL` on the bench schematic selects the MQ135 rail; verify which way it is strapped.
5. **If the rail measured good**, the boot loop is a strapping or firmware problem, not power.
   Check nothing in the wiring touches GPIO0, 2, 12 or 15 — GPIO12 pulled high is the classic
   cause of this exact loop. With those clear, retry download mode holding BOOT (GPIO0) low:

   ```bash
   pio pkg exec -p tool-esptoolpy -- esptool.py --port /dev/cu.usbserial-0001 chip_id
   ```

   A healthy chip prints its revision and MAC. That would mean the die survived and the board
   only needs reflashing.

## Reproducing the diagnosis

Raw listen, no reset asserted, 115200 — read the `rst:` reason and count banners. PlatformIO's
venv already has pyserial, so no install is needed:

```bash
~/.platformio/penv/bin/python - <<'PY'
import serial, time
s = serial.Serial("/dev/cu.usbserial-0001", 115200, timeout=0.2)
s.setDTR(False); s.setRTS(False)      # lines idle: no reset, run mode
t = time.time(); buf = b""
while time.time() - t < 10:
    buf += s.read(1024)
s.close()
print("bytes:", len(buf), "banners:", buf.count(b"ets Jun  8 2016"))
print(buf.decode("utf-8", "replace"))
PY
```

Six `ets Jun  8 2016` banners in a 10 s window is the ~1.5 s boot loop recorded above. A healthy
board prints one banner and then goes quiet (or hands off to the app).

## Outcome

The operator replaced the AMS1117-3.3. Re-ran the same two probes on the same port.

### Before vs after

| Signal | Smoked LDO | After replacement |
| --- | --- | --- |
| `esptool.py chip_id` | "No serial data received" | `ESP32-D0WDQ6 (revision v1.0)`, MAC `ac:67:b2:37:26:78` |
| Boot banners per 10 s | 6 (≈1.5 s loop) | **1** |
| Reset reason | `rst:0x10 (RTCWDT_RTC_RESET)` | `rst:0x1 (POWERON_RESET)` |
| Second-stage bootloader | never reached | `load:0x40080400,len:3028` / `load:0x40078000,len:13232`, `mode:DIO, clock div:2` |
| UART bytes per 10 s | 740, corrupted | 80 669, byte-clean |
| Application | never started | running, steady loop |

The 80 kB of byte-clean 115200 output settles the question the earlier corruption analysis
could not: **the rail is stable now, and the die, flash and core clock are healthy.**

What this does *not* establish is the root cause. The wiring state changed between the two
probes — the board was opened, a part was replaced, and any external strapping or short could
have been disturbed in the process. So:

- **Supported:** no short is present on `3V3` now; no strapping pin is held wrong now; the
  ESP32, its flash and its clock all survived; the board is usable.
- **Not supported:** that a downstream short never existed, that the strapping was never
  involved, or that the LDO was the only casualty. "It works now" is equally consistent with
  "there was a fault and the repair cleared it."

The original miswiring that killed the first regulator remains unidentified.

### Firmware output with no sensors fitted — all expected

```text
veml7700: not found (ok until fitted)
oled: no i2c device at 0x3C/0x3D on GPIO5/GPIO4
sds011: no AA C0 frame (listening GPIO16/RX2; ...)
am2302: read failed (need 3V3, 10k pull-up to 3V3 if the module has none)
pir: idle
mq135: raw=1360 gpio_mv=1095 aout_mv=1642
```

Every absent-peripheral message is correct for a bare board, and the init lines show each
subsystem reaching its configuration step. Note the limits of that: `pir: idle` on a pulled-down
input proves the pull-down, not the sensor path, and the drifting `mq135` samples
(1246 / 1255 / 1360) are what a *floating* ADC pin does — a faulty ADC returning noise would
look the same. These readings are consistent with working peripherals; they are not proof.
Verify the ADC and PIR against real inputs when the sensors go back on.

### One warning, investigated and dismissed

```text
WARNING: Detected crystal freq 41.01MHz is quite different to normalized freq 40MHz.
```

esptool infers this by timing, and USB latency skews the estimate. It is not corroborated: the
UART baud rate is derived from that same clock, and 80 669 bytes arrived at 115200 with zero
framing errors. A genuine 2.5 % clock error would show up as corruption. No action needed.

### Rail resistance measured after the repair

Operator measured `3V3` ↔ `GND` unpowered: **1.3 kΩ bare board, 1.0 kΩ with sensors fitted.**

Neither is a short. At 3.3 V that is 2.5 mA and 3.3 mA respectively — single-digit milliwatts,
below the board's own idle draw and nowhere near enough to stress an AMS1117. A rail short that
could vent a regulator is a sub-10 Ω, multi-amp affair.

The delta is at least the right *sign*: extra modules in parallel can only lower the reading,
and a drop of that order is ordinary for breakouts carrying pull-ups and power LEDs. No
combined-load figure is derived from the two numbers here — they were taken on different
configurations and are not a controlled pair, so treating them as one would be false precision.

The decisive evidence is empirical, though, not arithmetic: the board booted and produced 80 kB
of error-free serial output *with this exact resistance present*. A rail short severe enough to
matter cannot coexist with a clean `POWERON_RESET` and a running application.

**Conclusion: the 1.3 kΩ is normal and is not the fault.** It does not explain the original
smoke, so it does not close the question below.

### Still open

Nothing blocking. The original wiring mistake that killed the first regulator was never
identified from software — before reconnecting sensors, walk the 5 V/3.3 V split in
[power.md](power.md) so the replacement does not go the same way. The MQ135 heater rail
(`3V3_5V_SEL`) and the "no 5 V loads on `3V3`" rule are the two worth double-checking.
