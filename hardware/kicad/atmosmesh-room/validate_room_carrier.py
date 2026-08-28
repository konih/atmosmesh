#!/usr/bin/env python3
"""Deterministic structural checks for the provisional ROOM-01 carrier."""

from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parent
PCB = ROOT / "atmosmesh-room.kicad_pcb"
SCH = ROOT / "atmosmesh-room.kicad_sch"
PRO = ROOT / "atmosmesh-room.kicad_pro"
README = ROOT / "README.md"
GENERATOR = ROOT / "generate_project.py"
WIRING = ROOT / "wiring.md"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    for path in (PCB, SCH, PRO, README):
        require(path.is_file(), f"missing required project file: {path.name}")

    pcb = PCB.read_text(encoding="utf-8")
    sch = SCH.read_text(encoding="utf-8")
    readme = README.read_text(encoding="utf-8")

    require(re.search(r'\(general\s+\(thickness 1\.6\)', pcb) is not None,
            "PCB must be 1.6 mm")
    require('(0 "F.Cu" signal)' in pcb and '(2 "B.Cu" signal)' in pcb,
            "PCB must have two copper layers")
    for edge in (
        r'\(start 100 60\)\s+\(end 180 60\)',
        r'\(start 180 60\)\s+\(end 180 120\)',
        r'\(start 180 120\)\s+\(end 100 120\)',
        r'\(start 100 120\)\s+\(end 100 60\)',
    ):
        require(re.search(edge, pcb) is not None, "PCB outline must be exactly 80 x 60 mm")

    require(pcb.count('MountingHole_3.2mm_M3') == 4,
            "PCB must have exactly four M3 mounting holes")
    require(pcb.count('(attr through_hole)') >= 24,
            "all populated electrical parts must use THT footprints")
    for token in (
        'IDEASPARK 1.14in TFT - VERIFY ROW SPACING',
        'PARKED - PERFBOARD BUILD - DO NOT ORDER',
        'NEVER WIRE: 2 4 15 18 23 32 TFT / 12 STRAP / 1 3 UART',
        'VERIFY 30-PIN / 25.4mm ROWS',
        'NO COPPER / PARTS - ANTENNA',
        '5V DOMAIN',
        'SDS011 5V - NO DIODE - UART CROSSED',
        'BUZZER S/VCC/-',
        'SHT41 - KEEP FROM HEAT',
        'SHIELD FROM LCD',
    ):
        require(token in pcb, f"missing safety silkscreen: {token}")

    required_nets = {
        '+3V3', '+5V_USB_CONFIRMED', '+5V_EXT', '+5V_DOMAIN', 'GND', 'SDA_EXT', 'SCL_EXT',
        'GPIO21_SDA', 'GPIO22_SCL', 'GPIO25_BEEP', 'GPIO33_PIR_N',
        'PIR_BASE', 'BEEP_S', 'PIR_5V_PROTECTED',
        'SDS_5V_PROTECTED', 'SDS_RXD', 'SDS_TXD', 'GPIO16_SDS_RX', 'GPIO17_SDS_TX',
    }
    pcb_nets = set(re.findall(r'\(net \d+ "([^"]+)"\)', pcb))
    require(required_nets <= pcb_nets,
            f"PCB is missing nets: {sorted(required_nets - pcb_nets)}")

    for reference in (
        'U1', 'J_VEML', 'J_SHT', 'J_PIR', 'J_BEEP', 'JP_PIR_5V',
        'Q_PIR', 'D_PIR', 'R_SDA', 'R_SCL', 'R_BEEP_S',
        'J_SDS', 'JP_SDS_5V', 'R_SDS_RX', 'R_SDS_TX', 'C6', 'C7', 'J_5V_EXT', 'JP_5V_SRC',
        'TP_3V3', 'TP_5V', 'TP_GND', 'TP_SDA', 'TP_SCL',
    ):
        require(f'"Reference" "{reference}"' in pcb,
                f"PCB is missing {reference}")

    require('GPIO21_SDA' in sch and 'GPIO22_SCL' in sch,
            "schematic must document the external I2C bus")

    # The 1.14 inch TFT owns GPIO23/18/15/2/4/32 on the dev board itself, GPIO12 is the
    # flash-voltage strap and GPIO1/GPIO3 are the CP2102 USB-UART. Net names encode the
    # GPIO number, so a carrier net reaching any of them is visible here. Checked against
    # net names only: the silkscreen and notes name these pins deliberately.
    reserved_gpios = {1, 2, 3, 4, 12, 15, 18, 23, 32}
    used_gpios = {int(match) for net in pcb_nets
                  for match in re.findall(r'^GPIO(\d+)', net)}
    clash = sorted(used_gpios & reserved_gpios)
    require(not clash,
            f"carrier nets must not reach TFT/strap/USB-UART pins: GPIO{clash}")

    # The socket symbol was inherited from a DevKit V1, whose pin names run from the other end of
    # the row. KiCad writes each unconnected pad's own pin name into a placeholder net, so these
    # assert that the schematic labels the pad with the name the real board prints on it, and that
    # the display-owned pin is still unconnected.
    require('ESP32_DevKit_V1_Socket' not in sch,
            "U1 must not claim to be a DevKit V1 socket: its pin order differs in almost every position")
    for pad, name in ((30, 'GPIO23'), (24, 'GPIO18'), (4, 'GPIO12'), (10, 'GPIO32')):
        require(f'unconnected-(U1-{name}-Pad{pad})' in pcb,
                f"U1 pad {pad} must be labelled {name} and left unconnected")

    require('R_PU_SDA' in sch and 'R_PU_SCL' in sch and '3.3k' in sch,
            "external I2C needs its own 3.3k pullups: the TFT board has no onboard I2C device")
    require('330R' in sch and '1N5819' in sch and '2N3904' in sch,
            "schematic must contain the specified protection components")
    require(sch.count('100nF') >= 4,
            "C1, C3, C4 and C7 must use 100 nF, the confirmed ceramic stock value")
    require('470uF' in sch,
            "C6 must be the 470 uF SDS011 bulk: 10 uF was never shown to meet the 20 mV ripple limit")
    require('R_VEML' not in sch and 'R_SHT' not in sch,
            "sensor power must be direct 3V3 without series supply resistors")
    require('VEML7700 VIN/3Vo/GND/SCL/SDA' in sch,
            "schematic must use the confirmed five-pin VEML7700 header")
    require('BEEP_BASE' not in sch and 'BEEP_LOW' not in sch,
            "the buzzer low-side driver is gone: the Keyes module carries its own")

    upper = readme.upper()
    for warning in (
        'DO NOT ENERGISE', 'MEASURE THE ROW SPACING',
        'NEVER RECEIVE 5 V', 'NO ZENER',
    ):
        require(warning in upper, f"README is missing warning: {warning}")
    require(re.search(r'DEFAULT\s+OPEN', upper) is not None,
            "README is missing warning: DEFAULT OPEN")

    # wiring.md's pad table is hand-authored and is what a perfboard builder reads, since the
    # build rule is "authoritative by pin name". Nothing else keeps it honest against the code.
    generator = GENERATOR.read_text(encoding="utf-8")
    start = generator.index("IDEASPARK_PINS = {")
    code_pins = {int(pad): name for pad, name
                 in re.findall(r'(\d+): "([A-Z0-9]+)"', generator[start:generator.index("}", start)])}
    wiring = WIRING.read_text(encoding="utf-8")
    header = "| Pad | Name | Pad | Name |"
    require(header in wiring, "wiring.md must carry the socket pad table")
    table = wiring[wiring.index(header):]
    table = table[:table.index("\n\n")]
    doc_pins = {}
    for left, left_name, right, right_name in re.findall(
            r'^\|\s*(\d+)\s*\|\s*(.+?)\s*\|\s*(\d+)\s*\|\s*(.+?)\s*\|$', table, re.M):
        for pad, name in ((left, left_name), (right, right_name)):
            doc_pins[int(pad)] = name.replace("**", "").split("—")[0].split("/")[0].strip()
    for pad, name in sorted(code_pins.items()):
        documented = doc_pins.get(pad)
        require(documented is not None, f"wiring.md pad table is missing pad {pad}")
        require(documented == name or name.startswith(documented) or documented.startswith(name),
                f"wiring.md pad {pad} says {documented!r}, the generator says {name!r}")

    # D-025's transducer type is INFERRED from v1 driving this part with DC, not measured. The
    # ohmmeter table is what stops that inference quietly becoming a stated fact, so it is a gate.
    for reading in ("open / megohms", "16–42 Ω", "internal oscillator"):
        require(reading in wiring,
                f"wiring.md must keep the buzzer ohmmeter discriminator row for {reading!r}")
    require("different when the leads are swapped" in wiring,
            "wiring.md must keep the swapped-lead tell that distinguishes an active can")

    # A larger R_BEEP_S is silence, not protection, on the active and magnetic loads. The rule is
    # counter-intuitive enough that a later editor could 'improve' it into a fault.
    require("**Never raise it**" in wiring,
            "wiring.md must forbid raising R_BEEP_S")
    require(re.search(r'\|\s*`R_BEEP_S`\s*\|\s*100 Ω\s*\|', wiring) is not None,
            "wiring.md's resistor table must still size R_BEEP_S at 100 Ω")

    # D-026a: the SDS011 arrives on its adapter cable, whose colours vary between kits. Recording a
    # colour would restore exactly the guess that caused the 2026-08-17 straight-through UART.
    sds = wiring[wiring.index("#### The UART must be crossed") - 2500:
                 wiring.index("#### The UART must be crossed")]
    require("continuity" in sds,
            "wiring.md must tell the builder to ring the SDS011 cable out by continuity")
    for colour in ("red", "black", "green", "blue", "yellow", "white",
                   "rot", "schwarz", "grün", "blau", "gelb", "weiß"):
        require(not re.search(rf'\b{colour}\b', sds, re.I),
                f"wiring.md binds the SDS011 wire colour {colour!r}; colours vary between kits (D-026a)")

    print("ROOM-01 structural validation: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"ROOM-01 structural validation: FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
