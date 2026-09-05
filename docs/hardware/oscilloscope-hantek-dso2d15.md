# Hantek DSO2D15 — easy bench guide

The DSO2D15 is the bench oscilloscope recorded in
[`elektronik-inventar.md`](../elektronik-inventar.md). This page is the short, practical way to
use it for AtmosMesh work: what the knobs do, how to get a stable picture in under a minute, and
the exact settings for the measurements the project already asks for (rail ripple, I²C rise time,
UART levels, ADC divider checks).

Nominal figures from the model designation, to be confirmed on the boot screen or under
**Utility → System Info** and then copied into the inventory:

| Property | Value |
| --- | --- |
| Channels | 2 analog (CH1 yellow, CH2 blue) |
| Bandwidth | 150 MHz |
| Sample rate | 1 GSa/s (one channel), 500 MSa/s (both channels) |
| Memory | 8 Mpts |
| Display | 7" 800 × 480 |
| Generator (`D` model only) | 1 channel, 25 MHz, sine/square/ramp/pulse/noise/arbitrary, output on the front BNC marked **Gen Out** |
| Serial decode | UART/RS232, I²C, SPI, CAN, LIN |
| Input | 1 MΩ ‖ ~15 pF, BNC; the front panel prints the maximum input voltage. Read it before probing anything above 30 V |
| Ports | Front USB-A host (USB stick for screenshots and setups), rear USB-B device (PC, SCPI), rear BNC trigger-out / pass-fail |

## Safety rules that matter for this bench

These follow directly from [`AGENTS.md`](../../AGENTS.md) and [`power.md`](power.md).

1. **The probe ground clip is protective earth.** Inside the scope every BNC shell is tied to the
   mains earth pin. Clip the ground lead **only to circuit GND**. Clipping it to `3V3`, `5V`, a
   signal or an ESP32 pin shorts that node to earth through the scope, and with a USB-powered
   DevKit it shorts the laptop's USB ground reference too.
2. **Never probe the mains side of anything.** Open mains is out of scope for this project. The
   enclosed AC/DC module may be probed on its **DC output** only, and only after a multimeter has
   shown ~5 V there.
3. **Probe switch and channel setting must match.** A probe on `10×` with the channel set to `1×`
   shows 0.33 V for a 3.3 V rail; the other way round shows 33 V. Set **CH menu → Probe** to the
   real switch position every time you move a probe.
4. **The generator can hurt a GPIO.** When you feed the generator into anything on the ESP32
   side, set the high level to **3.3 V or less** and the low level to **0 V or more** *before*
   turning the output on. The generator can output well beyond 5 V.
5. Take the scope readings **before** energising a new rail or closing a jumper, the same way
   the wiring pages ask for multimeter readings first. One scope reading is a gate, not telemetry
   (decision on the declined ADS1115 in
   [`decisions.md`](../../agent-context/decisions.md)).

## The panel in one minute

The DSO2000-series front panel groups its controls like this. The labels are the ones printed on
the panel.

| Group | Control | What it does |
| --- | --- | --- |
| Screen | 5 soft keys down the right edge, **Menu On/Off** | Pick entries in whatever menu is open. Menu On/Off hides the menu to see the full trace |
| Screen | **Multipurpose** knob (push to confirm) | Scrolls lists, edits numbers, moves cursors |
| Run control | **Auto** | Autoset: finds the signal, sets volts/div, time/div and trigger. Start here |
| Run control | **Run/Stop** | Freeze the display; green = running, red = stopped |
| Run control | **Single** | Arm once, capture one trigger, then stop. For one-shot events |
| Run control | **Default** | Factory setup. Use it whenever the screen makes no sense |
| Vertical | **CH1 / CH2** buttons | Turn a channel on and open its menu (coupling, BW limit, probe factor, invert) |
| Vertical | **Volts/div** knob (push = fine steps) | Vertical scale |
| Vertical | **Position** knob (push = back to centre) | Move the trace up/down |
| Vertical | **Math**, **Ref** | FFT, CH1±CH2, saved reference traces |
| Horizontal | **Time/div** knob | Time base. Push for zoom (window) mode |
| Horizontal | **Position** knob (push = back to zero) | Move the trigger point left/right, i.e. see more before or after the event |
| Trigger | **Level** knob (push = 50 %) | Trigger threshold. Push it when the trace scrolls or flickers |
| Trigger | **Menu** | Edge / pulse / video / slope / … and the source channel |
| Trigger | **Force** | Fire one acquisition now, even without a valid trigger |
| Menus | **Measure**, **Cursor**, **Acquire**, **Display**, **Storage**, **Utility**, **Decode**, **Gen** | Named as they read. Decode is the serial decoder, Gen is the generator |
| Front | **Probe Comp** terminals | 1 kHz ~2 Vpp square wave for probe compensation |

The bottom line of the screen shows, per channel, the coupling and volts/div; the top line shows
the time/div, sample rate and the trigger status (`Trig'd`, `Auto`, `Stop`).

## First-time setup, once per probe

1. Power on, press **Default**.
2. Set the probe switch to **10×** and connect it to CH1. In **CH1 → Probe** choose `10X`.
3. Hook the tip on the **Probe Comp** signal terminal and the ground clip on its ground terminal.
4. Press **Auto**. You should see a square wave, about two divisions high at 1 V/div.
5. Look at the top corners of the square wave. If they overshoot or roll off, turn the small
   trimmer on the probe (at the BNC end, or in the head on some probes) with the plastic tool
   until the top is flat. This is *compensation*; do it for every probe on every channel it will
   be used on.
6. Set the probe to **1×** and check that **CH1 → Probe** reads `1X` and the wave is 10× taller.

Rule of thumb for the two probe positions:

| Position | Use it for | Trade-off |
| --- | --- | --- |
| `10×` | Everything logic-level: UART, I²C, SPI, PWM, rails at DC | 10 MΩ load, full bandwidth, 10× lower sensitivity (min. ~20 mV/div readable) |
| `1×` | Millivolt work: rail ripple, ADC divider outputs, small AC | Better resolution (2 mV/div), but only ~6–10 MHz bandwidth and 1 MΩ ‖ ~100 pF loading |

Probes in stock: the G6000 switchable 1×/10× and the CTK-014 set
([`inventory.md`](inventory.md)). Check the switch position by eye before every measurement.

## Getting a stable picture of anything

1. Ground clip on circuit GND, tip on the node.
2. Press **Auto**.
3. If the trace scrolls or breaks up: press the trigger **Level** knob (sets 50 %) and check in
   **Trigger → Menu** that **Source** is the channel you are looking at.
4. Turn **Time/div** until a few periods fill the screen. Turn **Volts/div** until the signal
   fills 4–6 of the 8 vertical divisions.
5. Press **Run/Stop** to freeze, **Measure** to read numbers, **Storage** to save a screenshot.

Autoset only finds repetitive signals above a few tens of millivolts. For anything one-shot,
slow, or tiny, set up manually as in the recipes below.

## Recipes for this project

### A. Supply rail: DC level, then ripple (SDS011 limit < 20 mV)

The SDS011 wants < 20 mV ripple on its 5 V; the Room carrier sizes `C6` at 470 µF to reach it but
the value is unproven until measured
([`atmosmesh-room/wiring.md`](../../hardware/kicad/atmosmesh-room/wiring.md)).

Step 1, the DC level:

| Setting | Value |
| --- | --- |
| Probe | `10×`, channel Probe = `10X` |
| CH coupling | **DC** |
| Volts/div | 1 V, position so 0 V is on the bottom division |
| Time/div | 1 ms |
| Trigger | Mode **Auto** (so the trace draws without an edge) |
| Measure | `Vavg` (or `Mean`) and `Vmax`, `Vmin` |

`Vavg` must be within the target (4.7–5.3 V for the SDS011; ~3.3 V for the LDO output). Do this
first, because AC coupling in step 2 hides a wrong DC level completely.

Step 2, the ripple:

| Setting | Value |
| --- | --- |
| Probe | `1×`, channel Probe = `1X`. Pull the long ground lead off and use the short ground spring on the probe barrel, tip and spring straight across the capacitor (`C6`) |
| CH coupling | **AC** |
| CH → BW Limit | **20 MHz** on |
| Volts/div | 10 mV, trace centred |
| Time/div | 10 µs (switching noise), then 2 ms (fan / load steps) |
| Acquire | **Peak Detect** to catch spikes, then **Average** (16) to see the underlying ripple |
| Trigger | Mode Auto, source that channel, level 0 |
| Measure | `Vpp` |

Read `Vpp` with the fan / sensor **running**. The number must stay below 20 mV. If it is
noisy without the DUT connected, the noise is probe pickup: shorten the ground path, or check
that the trace is flat with tip and spring both on GND.

### B. I²C rise time and decode (Room carrier pull-ups, Room v2 bus load)

The Room pull-ups (3.3 kΩ) assume ~200 pF of bus capacitance; Room v2 with six or seven devices
and the SPS30 on a cable will be higher. Measure, then choose the pull-up.

| Setting | Value |
| --- | --- |
| Probes | Both `10×`, both channels Probe = `10X`. CH1 = SDA, CH2 = SCL, grounds on GND |
| Coupling | DC on both, 1 V/div, both zero lines one division from the bottom |
| Time/div | 1 µs to see clocks; 200 ns to measure an edge |
| Trigger | Edge, source CH2 (SCL), **falling**, level ~1.6 V |
| Run | **Single**, then trigger traffic (boot the ESP32, or let the firmware poll) |

Rise time the I²C way: the spec measures from 30 % to 70 % of VDD, not the 10–90 % the
automatic `Rise` measurement uses. Open **Cursor**, type **Voltage**, put the lines at
**0.99 V** and **2.31 V** (30 % and 70 % of 3.3 V), note where they cross a rising edge, then
switch to **Time** cursors and put one on each crossing. Δt is the rise time.

| Bus mode | Limit (30–70 %) |
| --- | --- |
| Standard 100 kHz | 1000 ns |
| Fast 400 kHz | 300 ns |

Too slow → lower the pull-up (the parallel total across all breakouts must stay ≥ ~1.1 kΩ so
the bus sinks no more than 3 mA at 3.3 V). Also check the **low** level: with strong pull-ups
`Vmin` on SDA should still be under 0.4 V.

Decode, to confirm which address answered: **Decode → Type I²C**, SCL = CH2, SDA = CH1,
threshold 1.6 V, display **Hex**. Expect `0x3C` for the OLED and the sensor addresses listed in
the story tables. A `~A` / `NACK` after the address means nobody is home at that address.

### C. UART: confirm 3.3 V levels before a signal reaches a GPIO

Every UART module goes through this before its TX is wired to an ESP32 RX pin. It is the check
that stops a 5 V TTL signal from reaching a 3.3 V GPIO.

| Setting | Value |
| --- | --- |
| Probe | `10×` on the module's **TX** pin, ground on the module's GND; module powered as it will be in the build |
| Coupling | DC, 1 V/div, zero line one division from the bottom |
| Trigger | Edge, **falling**, level ~1.6 V, mode **Normal** |
| Time/div | 9600 baud (SDS011): 200 µs. 115200 baud (HLK-LD2410S): 20 µs |
| Measure | `Vmax` and `Vmin` |

Idle must be **high**, `Vmax` must read ~3.3 V and never ~5 V. Only then does the line go to
GPIO16 (SDS011, UART2) or the radar's RX/TX pins as documented for the Spot. A module that idles
at 5 V needs a level shifter or divider, measured again after it is fitted.

Decode: **Decode → Type UART/RS232**, source the channel, baud 9600 or 115200, 8 data bits, no
parity, 1 stop, polarity **normal** (idle high), format Hex. SDS011 frames start with `AA C0`;
the LD2410S minimal report is `6E … 62`.

### D. ADC divider output (MQ135 on GPIO34)

| Setting | Value |
| --- | --- |
| Probe | `1×` on the divider output node, **before** it is wired to GPIO34 |
| Coupling | DC, 500 mV/div, zero at the bottom |
| Time/div | 10 ms, trigger Auto |
| Measure | `Vmax`, `Vavg` |

`Vmax` over a full warm-up and with the sensor in clean and dirty air must stay below the limit
the divider is designed for (3.0 V on the carrier's 10 k / 15 k, per the README). If it can reach
3.3 V, the divider is wrong, not the firmware.

### E. Catching a one-shot event (boot droop, brown-out, reset loop)

Use this when a board resets under load, like the 2026-08-17 LDO incident
([`incident-2026-08-17-ldo.md`](incident-2026-08-17-ldo.md)) or a Wi-Fi burst pulling the
3.3 V rail down.

1. Probe `10×` on the `3V3` pin, DC coupling, 500 mV/div, zero line two divisions from the
   bottom, so 3.3 V sits mid-screen.
2. **Trigger → Menu**: Edge, source CH1, **falling**, level **2.9 V**, mode **Normal**.
3. Turn horizontal **Position** so the trigger marker sits about 2 divisions from the left, so
   you get some history before the droop. Time/div 200 µs (Wi-Fi TX burst) or 50 ms (boot).
4. Press **Single**. Reset the board / start Wi-Fi. The scope stops on the first dip below 2.9 V.
5. Read `Vmin` and the dip width with **Measure** or **Cursor**.

Add CH2 on `EN` or on the UART TX to line the droop up with the reset or the boot banner.

### F. Using the generator (DSO2D15 only)

**Gen** opens the generator menu. Set **Waveform**, **Frequency**, **Amplitude**, **Offset**,
then **Output On**. The output BNC is on the front (Gen Out). Amplitude and offset are stated
into high impedance; do not load it below 50 Ω.

Bench uses that stay within the safety rules:

- **Check a divider or an ADC input:** 1 kHz sine, high level 3.0 V, low level 0 V. Compare what
  the scope reads at the ESP32 pin against what the firmware reports.
- **Fake a sensor UART idle level:** DC/square with high 3.3 V and low 0 V while you test a level
  shifter, without the real module attached.
- **Loop the generator into CH1** on a fresh board to check the probe and channel before trusting
  them on a real measurement.

Rule 4 above applies: high level ≤ 3.3 V and low level ≥ 0 V before the output goes on, whenever
the output is connected to anything on the ESP32 side.

## Saving evidence for a story

Stories want measurement evidence, not a description of one.

1. Put a FAT32 USB stick in the **front** USB port.
2. Freeze the trace (**Run/Stop**), turn on the measurements you want visible.
3. **Storage → Type: Picture**, target the USB stick, **Save**. Some firmware also maps the
   dedicated save/camera button to this; check once.
4. Name the file after the story and node, e.g. `R2-01_5V_ripple_fan_on.png`, and put it under
   `agent-context/stories/` evidence or `docs/assets/` with the reading in the story's table.

**Storage → Type: Setup** saves the full instrument state (`.set`), so a recipe above can be
recalled instead of dialled in again. Save one per recipe on the stick.

PC connection: the rear USB-B port speaks SCPI over USBTMC. On Linux it appears with `lsusb`; a
`*IDN?` query returns the model, serial and firmware. Screenshots and CSV exports can be pulled
that way, but the USB stick route is simpler and needs no drivers.

## When it does not work

| Symptom | Fix |
| --- | --- |
| Flat line, no signal | Channel off? Coupling on **GND**? Press **Auto**. Ground clip really on GND? |
| Reading is 10× too big or too small | Probe switch and **CH → Probe** disagree |
| Trace scrolls, flickers, or shows two overlaid signals | Push the trigger **Level** knob (50 %), trigger **Source** = this channel, mode **Auto** → **Normal** |
| `Trig?` and nothing draws | Trigger mode is Normal and the level is outside the signal. Push Level, or press **Force** once |
| Square wave has rounded or spiky corners | Probe not compensated, see first-time setup |
| Everything is fuzzy | **BW Limit 20 MHz** on, short ground spring instead of the lead, `Acquire → Average` |
| Measurement shows `****` | Signal not fully on screen, or too few periods. Adjust volts/div and time/div |
| Screen is nonsense after someone else used it | **Default**, then start again |
| Save fails | Stick not FAT32, or plugged into the rear (device) port instead of the front |

## What to add to the inventory after the first session

- Bandwidth, sample rate and generator range as printed on the boot screen / System Info.
- Firmware version (**Utility → System Info**).
- Which probe was compensated on which channel, and the ground-spring accessory if the CTK-014
  set contains one.
