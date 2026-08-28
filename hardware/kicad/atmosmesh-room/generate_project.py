#!/usr/bin/env python3
"""Generate the deterministic KiCad source project for ROOM-01.

The generated KiCad files remain editable in KiCad. This generator exists so the provisional
carrier can be reproduced and structurally checked before exact board photographs are available.
"""

from __future__ import annotations

import json
import pathlib
import subprocess
import uuid
import xml.etree.ElementTree as ET
from copy import deepcopy
from dataclasses import dataclass

from kiutils.items.common import Effects, Font, Justify, Position, Property
from kiutils.schematic import (
    GlobalLabel,
    HierarchicalSheetInstance,
    NoConnect,
    Schematic,
    SymbolProjectInstance,
    SymbolProjectPath,
    Text,
    TitleBlock,
)
from kiutils.symbol import SymbolLib


ROOT = pathlib.Path(__file__).resolve().parent
NS = uuid.UUID("80876e59-a826-4d7b-8a94-c1547d3238c0")


def uid(name: str) -> str:
    return str(uuid.uuid5(NS, name))


@dataclass(frozen=True)
class Part:
    ref: str
    value: str
    lib: str
    footprint: str
    sx: int
    sy: int
    pin_nets: tuple[str | None, ...]
    bx: float
    by: float
    rotation: float = 0


# Confirmed from docs/hardware/ideaspark-esp32-tft-pinout.png (operator evidence, 2026-08-28).
# Pads 1-15 are the left column and 16-30 the right column, both counted from the
# USB/button end, matching the pad geometry in Ideaspark_30Pin_Provisional.kicad_mod.
# The bench project's socket symbol is a DevKit V1, whose pin names run from the EN end and
# disagree with this board in almost every position. Rename it and relabel every pin, so the
# schematic cannot show "3V3" against the pad that is really GPIO23 / TFT MOSI.
SOCKET_SOURCE_SYMBOL = "ESP32_DevKit_V1_Socket"
SOCKET_SYMBOL = "Ideaspark_ESP32_1V14_TFT_30Pin"

IDEASPARK_PINS = {
    1: "VIN", 2: "GND", 3: "GPIO13", 4: "GPIO12", 5: "GPIO14", 6: "GPIO27",
    7: "GPIO26", 8: "GPIO25", 9: "GPIO33", 10: "GPIO32", 11: "GPIO35",
    12: "GPIO34", 13: "GPIO39", 14: "GPIO36", 15: "EN",
    16: "3V3", 17: "GND", 18: "GPIO15", 19: "GPIO2", 20: "GPIO4",
    21: "GPIO16", 22: "GPIO17", 23: "GPIO5", 24: "GPIO18", 25: "GPIO19",
    26: "GPIO21", 27: "GPIO3", 28: "GPIO1", 29: "GPIO22", 30: "GPIO23",
}

# The 1.14 inch TFT is wired to these on the dev board itself. Reaching any of them
# from the carrier fights the display driver for the pin.
LCD_RESERVED_PINS = {"GPIO23", "GPIO18", "GPIO15", "GPIO2", "GPIO4", "GPIO32"}
# GPIO12 is the flash-voltage strap (high at boot can stop the board starting),
# GPIO1/GPIO3 are the CP2102 USB-UART used by flash/monitor, EN is the reset line.
BOARD_RESERVED_PINS = LCD_RESERVED_PINS | {"GPIO12", "GPIO1", "GPIO3", "EN"}

U1_NETS = {
    "VIN": "+5V_USB_CONFIRMED",
    "3V3": "+3V3",
    "GND": "GND",
    "GPIO21": "GPIO21_SDA",
    "GPIO22": "GPIO22_SCL",
    "GPIO16": "GPIO16_SDS_RX",
    "GPIO17": "GPIO17_SDS_TX",
    "GPIO25": "GPIO25_BEEP",
    "GPIO33": "GPIO33_PIR_N",
}


def ideaspark_pin_nets() -> tuple:
    """Expand U1_NETS into the socket's 30 pad positions, refusing reserved pins."""
    result = []
    for index in range(1, len(IDEASPARK_PINS) + 1):
        name = IDEASPARK_PINS[index]
        net = U1_NETS.get(name)
        if net is not None and name in BOARD_RESERVED_PINS:
            raise RuntimeError(
                f"pad {index} ({name}) is reserved by the dev board and must stay unconnected")
        result.append(net)
    return tuple(result)


parts = [
    Part("U1", "IDEASPARK_ESP32_1V14_TFT_30PIN", "atmosmesh:" + SOCKET_SYMBOL, "room:Ideaspark_30Pin_Provisional", 3900, 3000,
         ideaspark_pin_nets(), 124.0, 68.0),
    Part("J_VEML", "VEML7700 VIN/3Vo/GND/SCL/SDA", "atmosmesh:Conn_01x05", "room:PinHeader_1x05_P2.54mm", 1200, 2100,
         ("+3V3", None, "GND", "SCL_EXT", "SDA_EXT"), 103.5, 70.0),
    # Operator-confirmed 2026-08-28: the SHT41 header reads VIN / GND / SCL / SDA.
    Part("J_SHT", "SHT41 VIN/GND/SCL/SDA", "Connector_Generic:Conn_01x04", "room:PinHeader_1x04_P2.54mm", 1200, 4200,
         ("+3V3", "GND", "SCL_EXT", "SDA_EXT"), 103.5, 91.0),
    # Operator-confirmed 2026-08-28: the D-SUN header reads GND / OUT / VCC.
    Part("J_PIR", "DSUN_PIR GND/OUT/VCC", "Connector_Generic:Conn_01x03", "room:PinHeader_1x03_P2.54mm", 8200, 4200,
         ("GND", "PIR_OUT", "PIR_5V_PROTECTED"), 176.0, 105.0),
    # Operator-confirmed 2026-08-28: no-name Keyes 3-pin breakout, header order S / VCC / -.
    Part("J_BEEP", "KEYES BUZZER S/VCC/-", "Connector_Generic:Conn_01x03", "room:PinHeader_1x03_P2.54mm", 8200, 1800,
         ("BEEP_S", "+3V3", "GND"), 176.0, 73.0),
    Part("JP_PIR_5V", "DEFAULT_OPEN", "atmosmesh:R", "room:Jumper_2_Open_P2.54mm", 6900, 3500,
         ("+5V_USB_CONFIRMED", "PIR_5V_RAW"), 158.0, 108.0),
    Part("D_PIR", "1N5819", "atmosmesh:D_Schottky", "room:D_Axial_P7.62mm", 7400, 3500,
         ("PIR_5V_PROTECTED", "PIR_5V_RAW"), 165.0, 108.0),
    Part("Q_PIR", "2N3904 C/B/E", "atmosmesh:Q_NPN_CBE", "room:TO92_CBE", 6800, 4300,
         ("GPIO33_PIR_N", "PIR_BASE", "GND"), 160.0, 99.0),
    Part("R_PIR_IN", "10k", "Device:R", "room:R_Axial_P7.62mm", 6000, 4300,
         ("PIR_OUT", "PIR_BASE"), 170.0, 99.0),
    Part("R_PIR_PD", "100k", "Device:R", "room:R_Axial_P7.62mm", 6800, 4800,
         ("PIR_BASE", "GND"), 160.0, 115.0),
    Part("R_PIR_PU", "10k", "Device:R", "room:R_Axial_P7.62mm", 6800, 3800,
         ("+3V3", "GPIO33_PIR_N"), 154.0, 94.0),
    Part("C5", "100uF", "Device:C_Polarized", "room:C_Radial_P2.00mm", 7800, 3800,
         ("PIR_5V_PROTECTED", "GND"), 172.0, 113.0),
    Part("R_BEEP_S", "100R", "Device:R", "room:R_Axial_P7.62mm", 6000, 2100,
         ("GPIO25_BEEP", "BEEP_S"), 155.0, 80.0),
    Part("R_SDA", "330R", "Device:R", "room:R_Axial_P7.62mm", 2300, 2700,
         ("SDA_EXT", "GPIO21_SDA"), 111.0, 83.0),
    Part("R_SCL", "330R", "Device:R", "room:R_Axial_P7.62mm", 2300, 3200,
         ("SCL_EXT", "GPIO22_SCL"), 111.0, 87.0),
    Part("R_PU_SDA", "3.3k", "Device:R", "room:R_Axial_P7.62mm", 2300, 3700,
         ("SDA_EXT", "+3V3"), 104.0, 108.0),
    Part("R_PU_SCL", "3.3k", "Device:R", "room:R_Axial_P7.62mm", 2300, 4200,
         ("SCL_EXT", "+3V3"), 115.0, 108.0),
    Part("C3", "100nF", "Device:C", "room:C_Disc_P5.00mm", 1900, 2200,
         ("+3V3", "GND"), 108.0, 73.0),
    Part("C4", "100nF", "Device:C", "room:C_Disc_P5.00mm", 1900, 4500,
         ("+3V3", "GND"), 108.0, 94.0),
    Part("C2", "100uF", "Device:C_Polarized", "room:C_Radial_P2.00mm", 5400, 4200,
         ("+3V3", "GND"), 143.0, 112.0),
    Part("C1", "100nF", "Device:C", "room:C_Disc_P5.00mm", 5400, 4500,
         ("+3V3", "GND"), 149.0, 112.0),
    # SDS011 (Nova PM). 5 V fan/laser, 3.3 V TTL UART, crossed: sensor TXD -> GPIO16/RX2.
    Part("J_SDS", "SDS011 5V/GND/RXD/TXD", "Connector_Generic:Conn_01x04", "room:PinHeader_1x04_P2.54mm", 8200, 5400,
         ("SDS_5V_PROTECTED", "GND", "SDS_RXD", "SDS_TXD"), 168.0, 84.0),
    Part("JP_SDS_5V", "DEFAULT_OPEN", "atmosmesh:R", "room:Jumper_2_Open_P2.54mm", 6900, 5400,
         ("+5V_USB_CONFIRMED", "SDS_5V_PROTECTED"), 120.0, 112.0),
    Part("R_SDS_RX", "1k", "Device:R", "room:R_Axial_P7.62mm", 6000, 5400,
         ("SDS_TXD", "GPIO16_SDS_RX"), 108.0, 100.0),
    Part("R_SDS_TX", "1k", "Device:R", "room:R_Axial_P7.62mm", 6000, 5900,
         ("GPIO17_SDS_TX", "SDS_RXD"), 108.0, 103.0),
    Part("C6", "470uF", "Device:C_Polarized", "room:C_Radial_P2.00mm", 7800, 5400,
         ("SDS_5V_PROTECTED", "GND"), 126.0, 112.0),
    Part("C7", "100nF", "Device:C", "room:C_Disc_P5.00mm", 7800, 5900,
         ("SDS_5V_PROTECTED", "GND"), 131.0, 108.0),
    Part("TP_3V3", "3V3/GND", "atmosmesh:Conn_01x02", "room:TestPoint_2Pin_THT", 5800, 4700, ("+3V3", "GND"), 134.0, 116.0),
    Part("TP_5V", "5V/GND", "atmosmesh:Conn_01x02", "room:TestPoint_2Pin_THT", 6200, 4700, ("+5V_USB_CONFIRMED", "GND"), 141.0, 116.0),
    Part("TP_GND", "GND/GND", "atmosmesh:Conn_01x02", "room:TestPoint_2Pin_THT", 6600, 4700, ("GND", "GND"), 148.0, 116.0),
    Part("TP_SDA", "SDA/GND", "atmosmesh:Conn_01x02", "room:TestPoint_2Pin_THT", 2600, 3900, ("SDA_EXT", "GND"), 103.5, 83.0),
    Part("TP_SCL", "SCL/GND", "atmosmesh:Conn_01x02", "room:TestPoint_2Pin_THT", 3000, 3900, ("SCL_EXT", "GND"), 118.0, 93.0),
]


DESCRIPTION_BY_REF = {
    "U1": "Ideaspark ESP32-WROOM-32 1.14 inch TFT board, 30-pin socket; pin names confirmed, row spacing still to measure",
    "J_VEML": "Operator-confirmed VEML7700 header: VIN, 3Vo NC, GND, SCL, SDA",
    "J_SHT": "Confirmed SHT41 header order: VIN, GND, SCL, SDA",
    "J_PIR": "Confirmed D-SUN PIR header order: GND, OUT, protected 5V",
    "J_BEEP": "Confirmed Keyes 3-pin buzzer header: S, VCC, minus",
    "R_BEEP_S": "100 ohm series protection between GPIO25 and the buzzer S input; see wiring.md if the buzzer stays silent",
    "JP_PIR_5V": "Default-open PIR 5V enable jumper",
    "D_PIR": "1N5819 PIR supply diode; pin 1 K to protected rail, pin 2 A to raw 5V",
    "Q_PIR": "2N3904 active-low PIR interface; symbol and footprint pins 1/2/3 are C/B/E",
    "R_SDA": "330 ohm external SDA fault-current limiter and edge damper",
    "R_SCL": "330 ohm external SCL fault-current limiter and edge damper",
    "R_PU_SDA": "3.3 kilohm SDA_EXT pullup to 3V3; sized for perfboard wiring capacitance at 100 kHz",
    "R_PU_SCL": "3.3 kilohm SCL_EXT pullup to 3V3; sized for perfboard wiring capacitance at 100 kHz",
    "R_PIR_IN": "10 kilohm PIR-output-to-NPN-base current limiter",
    "R_PIR_PD": "100 kilohm PIR NPN base-emitter pulldown",
    "R_PIR_PU": "10 kilohm GPIO33 pullup to 3V3 for active-low PIR input",
    "C1": "100 nF controller 3V3 local decoupling",
    "C2": "100 uF controller 3V3 bulk capacitor; observe polarity",
    "C3": "100 nF VEML7700 VIN input decoupling",
    "C4": "100 nF SHT41 3V3 local decoupling",
    "C5": "100 uF protected PIR 5V bulk capacitor; observe polarity",
    "J_SDS": "SDS011 header: protected 5V, GND, sensor RXD, sensor TXD. VERIFY the module connector order",
    "JP_SDS_5V": "Default-open SDS011 5V enable jumper; no series diode, see D-026",
    "R_SDS_RX": "1 kilohm between the SDS011 TXD output and GPIO16/RX2; bounds a driver fight to about 3 mA",
    "R_SDS_TX": "1 kilohm between GPIO17/TX2 and the SDS011 RXD input",
    "C6": "470 uF protected SDS011 5V bulk; sized for the fan against the 20 mV ripple limit; observe polarity",
    "C7": "100 nF SDS011 5V high-frequency decoupling",
    "TP_3V3": "3V3 and GND paired through-hole measurement points",
    "TP_5V": "Confirmed USB 5V and GND paired through-hole measurement points",
    "TP_GND": "Paired GND through-hole continuity points",
    "TP_SDA": "External SDA and GND paired through-hole measurement points",
    "TP_SCL": "External SCL and GND paired through-hole measurement points",
}


def description_for(part: Part) -> str:
    return DESCRIPTION_BY_REF[part.ref]


def pin_offsets(symbol) -> dict[str, Position]:
    result = {}
    for unit in symbol.units:
        for pin in unit.pins:
            result[pin.number] = pin.position
    return result


def stock_symbol(name: str, library: str):
    candidates = [
        pathlib.Path(f"/Applications/KiCad/KiCad.app/Contents/SharedSupport/symbols/{library}.kicad_sym"),
        pathlib.Path(f"/usr/share/kicad/symbols/{library}.kicad_sym"),
    ]
    path = next((candidate for candidate in candidates if candidate.is_file()), None)
    if path is None:
        raise RuntimeError(f"KiCad {library}.kicad_sym not found; KiCad 10 is required")
    symbols = SymbolLib().from_file(str(path))
    result = deepcopy(next(item for item in symbols.symbols if item.entryName == name))
    result.libraryNickname = "atmosmesh"
    return result


def set_symbol_description(symbol, description: str) -> None:
    for prop in symbol.properties:
        if prop.key == "Description":
            prop.value = description


def write_local_symbol_library(lib_symbols: list) -> None:
    """Write the project library from the exact symbols the schematic embeds.

    KiCad's lib_symbol_mismatch ERC check compares the schematic's embedded copy
    against the library. Building the two independently drifts on the first
    description or pin-type edit, so both come from this one list.
    """
    symbols = []
    for item in lib_symbols:
        symbol = deepcopy(item)
        symbol.libraryNickname = None
        symbols.append(symbol)
    SymbolLib(symbols=symbols).to_file(str(ROOT / "atmosmesh.kicad_sym"))


def write_native_schematic() -> pathlib.Path:
    source_path = ROOT.parent / "atmosmesh-bench" / "atmosmesh-bench.kicad_sch"
    source = Schematic().from_file(str(source_path))
    root_uuid = uid("schematic-root")
    sheet = Schematic(uuid=root_uuid)
    sheet.paper.paperSize = "A3"
    sheet.titleBlock = TitleBlock(
        title="AtmosMesh Room protected carrier", date="2026-08-28",
        revision="PROVISIONAL A", company="AtmosMesh",
        comments={1: "DO NOT ENERGISE - row spacing, SDS011 connector order and all 5 V measurements still open",
                  2: "GPIO21 SDA / GPIO22 SCL; no 5 V on any GPIO or 3V3",
                  3: "Built by hand on a 31x27 perfboard; the PCB is parked", 4: "ROOM-01 / 02 / 03"},
    )
    conn_01x05 = stock_symbol("Conn_01x05", "Connector_Generic")
    diode = stock_symbol("D", "Device")
    schottky = stock_symbol("D_Schottky", "Device")
    npn_cbe = stock_symbol("Q_NPN_CBE", "Transistor_BJT")
    extra_symbols = [conn_01x05, diode, schottky, npn_cbe]

    needed_names = {part.lib.split(":")[-1] for part in parts}
    needed_names.discard(SOCKET_SYMBOL)
    needed_names.add(SOCKET_SOURCE_SYMBOL)
    sheet.libSymbols = [deepcopy(item) for item in source.libSymbols if item.entryName in needed_names]
    for item in sheet.libSymbols:
        if item.entryName == SOCKET_SOURCE_SYMBOL:
            item.entryName = SOCKET_SYMBOL
            # KiCad will not load the sheet unless each unit's own entry name is renamed with it.
            for unit in item.units:
                unit.entryName = unit.entryName.replace(SOCKET_SOURCE_SYMBOL, SOCKET_SYMBOL, 1)
            for prop in item.properties:
                if prop.key == "Value" and prop.value == SOCKET_SOURCE_SYMBOL:
                    prop.value = SOCKET_SYMBOL
    sheet.libSymbols.extend(deepcopy(item) for item in extra_symbols)
    for item in sheet.libSymbols:
        set_symbol_description(item, f"ROOM-01 symbol: {item.entryName}; see schematic instance description")
    lib_by_name = {item.entryName: item for item in sheet.libSymbols}
    for unit in lib_by_name[SOCKET_SYMBOL].units:
        for pin in unit.pins:
            pin.electricalType = "passive"
            pin.name = IDEASPARK_PINS[int(pin.number)]
    write_local_symbol_library(sheet.libSymbols)
    template_by_name = {}
    for item in source.schematicSymbols:
        template_by_name.setdefault(item.entryName, item)
    socket_template = deepcopy(template_by_name[SOCKET_SOURCE_SYMBOL])
    socket_template.entryName = SOCKET_SYMBOL
    socket_template.libraryNickname = "atmosmesh"
    template_by_name[SOCKET_SYMBOL] = socket_template
    conn_01x05_template = deepcopy(template_by_name["Conn_01x04"])
    conn_01x05_template.entryName = "Conn_01x05"
    conn_01x05_template.pins = {str(index): "" for index in range(1, 6)}
    template_by_name["Conn_01x05"] = conn_01x05_template
    for entry, pin_count, base in (("D", 2, "R"), ("D_Schottky", 2, "R"),
                                   ("Q_NPN_CBE", 3, "Conn_01x03")):
        template = deepcopy(template_by_name[base])
        template.libraryNickname = "atmosmesh"
        template.entryName = entry
        template.pins = {str(index): "" for index in range(1, pin_count + 1)}
        template_by_name[entry] = template

    non_controller_index = 0
    for part in parts:
        entry = part.lib.split(":")[-1]
        symbol = deepcopy(template_by_name[entry])
        symbol.uuid = uid(f"sch-{part.ref}")
        if part.ref == "U1":
            symbol.position = Position(81.28, 104.14, 0)
        else:
            column = non_controller_index % 5
            row = non_controller_index // 5
            symbol.position = Position(142.24 + 55.88 * column, 45.72 + 38.1 * row, 0)
            non_controller_index += 1
        for prop in symbol.properties:
            if prop.key == "Reference":
                prop.value = part.ref
                offset = 23 if part.ref == "U1" else 6
                prop.position = Position(symbol.position.X, symbol.position.Y - offset, 0)
            elif prop.key == "Value":
                prop.value = part.value
                offset = 23 if part.ref == "U1" else 6
                prop.position = Position(symbol.position.X, symbol.position.Y + offset, 0)
            elif prop.key == "Footprint":
                prop.value = part.footprint
                prop.position = Position(symbol.position.X, symbol.position.Y, 0)
                prop.effects.hide = True
            elif prop.key == "Description":
                prop.value = description_for(part)
                prop.position = Position(symbol.position.X, symbol.position.Y, 0)
                prop.effects.hide = True
            else:
                prop.position = Position(symbol.position.X, symbol.position.Y, 0)
                if prop.effects is not None:
                    prop.effects.hide = True
        symbol.pins = {str(index): uid(f"sch-pin-{part.ref}-{index}") for index in range(1, len(part.pin_nets) + 1)}
        symbol.instances = [SymbolProjectInstance(
            name="atmosmesh-room",
            paths=[SymbolProjectPath(sheetInstancePath=f"/{root_uuid}", reference=part.ref, unit=1)],
        )]
        sheet.schematicSymbols.append(symbol)

        offsets = pin_offsets(lib_by_name[entry])
        for pin_number, net in enumerate(part.pin_nets, start=1):
            offset = offsets[str(pin_number)]
            position = Position(symbol.position.X + offset.X, symbol.position.Y - offset.Y, 0)
            if net is None:
                sheet.noConnects.append(NoConnect(position=position, uuid=uid(f"nc-{part.ref}-{pin_number}")))
            else:
                sheet.globalLabels.append(GlobalLabel(
                    text=net, shape="bidirectional", position=position,
                    effects=Effects(font=Font(height=1.0, width=1.0), justify=Justify()),
                    uuid=uid(f"label-{part.ref}-{pin_number}"),
                ))

    notes = [
        (18, 15, "PERFBOARD BUILD - PIN NAMES CONFIRMED; ROW SPACING, SDS011 CONNECTOR AND ALL 5V MEASUREMENTS STILL OPEN"),
        (18, 20, "Display is SPI, not I2C: GPIO21/22 carry no onboard device. R_PU_SDA/R_PU_SCL 4k7 are the only bus pullups."),
        (18, 25, "NEVER WIRE: GPIO23/18/15/2/4/32 drive the 1.14in TFT; GPIO12 is the flash strap; GPIO1/3 are the USB-UART."),
        (18, 270, "No Zener clamps: known inventory starts at 5.1V and is unsuitable for 3.3V GPIO protection."),
        (210, 15, "PIR: default-open 5V power + 1N5819; NPN interface is ACTIVE LOW."),
        (210, 30, "SDS011: default-open 5V, NO series diode (4.7V minimum). UART CROSSED: sensor TXD -> GPIO16/RX2."),
        (210, 20, "Buzzer: Keyes 3-pin S/VCC/-. GPIO25 drives S directly through 100R, active HIGH as on AtmosMesh v1."),
    ]
    for index, (x, y, note) in enumerate(notes):
        sheet.texts.append(Text(text=note, position=Position(x, y, 0),
                                effects=Effects(font=Font(height=1.5, width=1.5), justify=Justify()),
                                uuid=uid(f"note-{index}")))
    sheet.sheetInstances = [HierarchicalSheetInstance(instancePath="/", page="1")]
    output = ROOT / "atmosmesh-room.kicad_sch"
    sheet.to_file(str(output))
    return output


def write_project_and_tables() -> None:
    project = {
        "board": {}, "boards": [], "cvpcb": {}, "erc": {}, "libraries": {},
        "meta": {"filename": "atmosmesh-room.kicad_pro", "version": 1},
        "net_settings": {"classes": [], "meta": {"version": 3}},
        "pcbnew": {}, "schematic": {}, "sheets": [], "text_variables": {}
    }
    (ROOT / "atmosmesh-room.kicad_pro").write_text(json.dumps(project, indent=2) + "\n", encoding="utf-8")
    (ROOT / "fp-lib-table").write_text(
        '(fp_lib_table\n  (version 7)\n  (lib (name "room")(type "KiCad")'
        '(uri "${KIPRJMOD}/room.pretty")(options "")(descr "ROOM-01 THT footprints"))\n)\n', encoding="utf-8")
    (ROOT / "sym-lib-table").write_text(
        '(sym_lib_table\n  (version 7)\n  (lib (name "atmosmesh")(type "KiCad")'
        '(uri "${KIPRJMOD}/atmosmesh.kicad_sym")(options "")(descr "Vendored ROOM-01 symbols"))\n)\n',
        encoding="utf-8")


def board_pad_positions(part: Part) -> list[tuple[float, float]]:
    count = len(part.pin_nets)
    if part.footprint.endswith("Ideaspark_30Pin_Provisional"):
        return ([(part.bx, part.by + 2.54 * index) for index in range(15)] +
                [(part.bx + 25.4, part.by + 2.54 * index) for index in range(15)])
    if "PinHeader_1x" in part.footprint:
        return [(part.bx, part.by + 2.54 * index) for index in range(count)]
    if part.footprint.endswith("TO92_CBE"):
        return [(part.bx, part.by), (part.bx + 2.54, part.by), (part.bx + 5.08, part.by)]
    if part.footprint.endswith("TestPoint_2Pin_THT"):
        return [(part.bx, part.by), (part.bx + 2.54, part.by)]
    pitch = 2.0 if part.footprint.endswith("C_Radial_P2.00mm") else 5.0 if part.footprint.endswith("C_Disc_P5.00mm") else 2.54 if "Jumper" in part.footprint else 7.62
    return [(part.bx, part.by), (part.bx + pitch, part.by)]


def property_block(ref: str, value: str, description: str) -> str:
    return f'''\n\t\t(property "Reference" "{ref}"\n\t\t\t(at 0 -2 0)\n\t\t\t(layer "F.Fab") (hide yes)\n\t\t\t(uuid "{uid(ref + '-ref')}")\n\t\t\t(effects (font (size 0.9 0.9) (thickness 0.15)))\n\t\t)\n\t\t(property "Value" "{value}"\n\t\t\t(at 0 2 0)\n\t\t\t(layer "F.Fab")\n\t\t\t(uuid "{uid(ref + '-value')}")\n\t\t\t(effects (font (size 0.8 0.8) (thickness 0.12)))\n\t\t)\n\t\t(property "Description" "{description}"\n\t\t\t(at 0 0 0) (layer "F.Fab") (hide yes)\n\t\t\t(uuid "{uid(ref + '-description')}")\n\t\t\t(effects (font (size 1 1)))\n\t\t)'''


def semantic_footprint_markings(name: str, identity: str, indent: str = "\t\t") -> str:
    # Continuation lines extend the caller's own indent character. Appending a literal tab
    # to the space-indented .kicad_mod writer produced "space before tab" and failed task check.
    cont = indent + ("\t" if indent.startswith("\t") else "  ")
    if name == "D_Axial_P7.62mm":
        return f'''
{indent}(fp_text user "K" (at 0 -2.1) (layer "F.SilkS") (uuid "{uid(identity + '-mark-k')}")
{cont}(effects (font (size 0.8 0.8) (thickness 0.14))) )
{indent}(fp_text user "A" (at 7.62 -2.1) (layer "F.SilkS") (uuid "{uid(identity + '-mark-a')}")
{cont}(effects (font (size 0.8 0.8) (thickness 0.14))) )
{indent}(fp_line (start 1.4 -1.25) (end 1.4 1.25) (stroke (width 0.35) (type solid))
{cont}(layer "F.SilkS") (uuid "{uid(identity + '-cathode-band')}"))'''
    if name == "TO92_CBE":
        labels = (("C", 0), ("B", 2.54), ("E", 5.08))
        return "\n" + "\n".join(
            f'''{indent}(fp_text user "{label}" (at {x} -2.1) (layer "F.SilkS") (uuid "{uid(identity + '-mark-' + label.lower())}")
{cont}(effects (font (size 0.8 0.8) (thickness 0.14))) )'''
            for label, x in labels
        )
    return ""


def write_board(netlist_path: pathlib.Path) -> None:
    tree = ET.parse(netlist_path)
    paths = {c.attrib["ref"]: c.findtext("tstamps") for c in tree.findall("./components/comp")}
    nets: dict[str, int] = {}
    pin_nets: dict[tuple[str, str], str] = {}
    for net in tree.findall("./nets/net"):
        name = net.attrib["name"]
        nets[name] = int(net.attrib["code"])
        for node in net.findall("node"):
            pin_nets[(node.attrib["ref"], node.attrib["pin"])] = name

    lines = [f'''(kicad_pcb
\t(version 20260206)
\t(generator "atmosmesh-room-generator")
\t(generator_version "10.0")
\t(general (thickness 1.6) (legacy_teardrops no))
\t(paper "A4")
\t(title_block
\t\t(title "AtmosMesh Room protected carrier")
\t\t(date "2026-08-28")
\t\t(rev "PROVISIONAL A")
\t\t(company "AtmosMesh")
\t\t(comment 1 "PARKED - the build target is a 31x27 perfboard, not this board")
\t)
\t(layers
\t\t(0 "F.Cu" signal)
\t\t(2 "B.Cu" signal)
\t\t(9 "F.Adhes" user "F.Adhesive")
\t\t(11 "B.Adhes" user "B.Adhesive")
\t\t(13 "F.Paste" user)
\t\t(15 "B.Paste" user)
\t\t(5 "F.SilkS" user "F.SilkS")
\t\t(7 "B.SilkS" user "B.Silkscreen")
\t\t(1 "F.Mask" user)
\t\t(3 "B.Mask" user)
\t\t(17 "Dwgs.User" user "Dwgs.User")
\t\t(19 "Cmts.User" user "User.Comments")
\t\t(21 "Eco1.User" user "User.Eco1")
\t\t(23 "Eco2.User" user "User.Eco2")
\t\t(25 "Edge.Cuts" user)
\t\t(27 "Margin" user)
\t\t(31 "F.CrtYd" user "F.Courtyard")
\t\t(29 "B.CrtYd" user "B.Courtyard")
\t\t(35 "F.Fab" user)
\t\t(33 "B.Fab" user)
\t)
\t(setup (pad_to_mask_clearance 0) (allow_soldermask_bridges_in_footprints no))''']
    def pcb_net_name(name: str) -> str:
        return name.replace("/", "{slash}") if name.startswith("unconnected-(") else name

    for name, code in sorted(nets.items(), key=lambda item: item[1]):
        lines.append(f'\t(net {code} "{pcb_net_name(name)}")')

    pad_locations: dict[str, list[tuple[float, float, str]]] = {}
    for part in parts:
        positions = board_pad_positions(part)
        relative = [(x - part.bx, y - part.by) for x, y in positions]
        lines.append(f'''\t(footprint "{part.footprint}"
\t\t(layer "F.Cu")
\t\t(uuid "{uid('fp-' + part.ref)}")
\t\t(at {part.bx} {part.by})
\t\t{property_block(part.ref, part.value, description_for(part))}
\t\t(path "/{paths[part.ref]}")
\t\t(sheetname "/")
\t\t(sheetfile "atmosmesh-room.kicad_sch")
\t\t(attr through_hole)
\t\t(fp_rect (start {min(x for x, _ in relative)-1.25} {min(y for _, y in relative)-1.25}) (end {max(x for x, _ in relative)+1.25} {max(y for _, y in relative)+1.25})
\t\t\t(stroke (width 0.15) (type solid)) (fill no) (layer "F.Fab") (uuid "{uid('silk-' + part.ref)}")){semantic_footprint_markings(part.footprint.split(':', 1)[1], 'pcb-' + part.ref)}''')
        for index, ((x, y), (rx, ry)) in enumerate(zip(positions, relative), start=1):
            net_name = pin_nets.get((part.ref, str(index)))
            net_clause = f' (net {nets[net_name]} "{pcb_net_name(net_name)}")' if net_name else ""
            shape = "rect" if index == 1 else "circle"
            lines.append(f'''\t\t(pad "{index}" thru_hole {shape} (at {rx:.3f} {ry:.3f}) (size 1.7 1.7) (drill 0.8)
\t\t\t(layers "*.Cu" "*.Mask"){net_clause} (uuid "{uid(f'pad-{part.ref}-{index}')}") )''')
            if net_name:
                pad_locations.setdefault(net_name, []).append((x, y, part.ref))
        lines.append("\t)")

    for index, (x, y) in enumerate(((104, 64), (176, 64), (104, 116), (176, 116)), start=1):
        lines.append(f'''\t(footprint "room:MountingHole_3.2mm_M3" (layer "F.Cu") (uuid "{uid(f'mh-{index}')}") (at {x} {y})
\t\t(property "Reference" "H{index}" (at 0 -4 0) (layer "F.Fab") (hide yes) (uuid "{uid(f'mhr-{index}')}") (effects (font (size 1 1))))
\t\t(property "Value" "M3" (at 0 4 0) (layer "F.Fab") (uuid "{uid(f'mhv-{index}')}") (effects (font (size 1 1))))
\t\t(attr board_only through_hole exclude_from_pos_files exclude_from_bom)
\t\t(fp_circle (center 0 0) (end 4 0) (stroke (width 0.25) (type solid)) (fill no) (layer "F.Fab") (uuid "{uid(f'mhc-{index}')}") )
\t\t(pad "" np_thru_hole circle (at 0 0) (size 3.2 3.2) (drill 3.2) (layers "*.Cu" "*.Mask") (uuid "{uid(f'mhp-{index}')}") )
\t)''')

    # Copper remains intentionally unrouted until exact board/module photos establish the physical
    # socket spacing, orientation and connector pin order.

    texts = [
        (140, 61.5, "IDEASPARK 1.14in TFT - VERIFY ROW SPACING", 1.2),
        (140, 63.5, "VERIFY 30-PIN / 25.4mm ROWS", 1.0),
        (136.5, 66, "NO COPPER / PARTS - ANTENNA", 0.9),
        (140, 107, "NEVER WIRE: 2 4 15 18 23 32 TFT / 12 STRAP / 1 3 UART", 0.8),
        (169, 102, "5V DOMAIN", 1.0),
        (120, 118, "SDS011 5V - NO DIODE - UART CROSSED", 0.8),
        (166, 65, "BUZZER S/VCC/-", 0.8),
        (108, 89, "SHT41 - KEEP FROM HEAT", 0.8),
        (113, 66, "SHIELD FROM LCD", 0.8),
        (140, 118.5, "PARKED - PERFBOARD BUILD - DO NOT ORDER", 1.0),
    ]
    for index, (x, y, value, size) in enumerate(texts):
        layer = "F.SilkS" if index in {0, 1, 8} else "Dwgs.User"
        lines.append(f'''\t(gr_text "{value}" (at {x} {y}) (layer "{layer}") (uuid "{uid(f'text-{index}')}")
\t\t(effects (font (size {size} {size}) (thickness 0.16)) (justify)) )''')
    # The antenna region is a visible placement keepout; final copper keepout awaits exact photos.
    lines.append(f'''\t(gr_rect (start 119 60.5) (end 155 67) (stroke (width 0.3) (type dash)) (fill none)
\t\t(layer "Dwgs.User") (uuid "{uid('antenna-box')}") )''')
    for index, (x1, y1, x2, y2) in enumerate(((100, 60, 180, 60), (180, 60, 180, 120), (180, 120, 100, 120), (100, 120, 100, 60))):
        lines.append(f'''\t(gr_line (start {x1} {y1}) (end {x2} {y2}) (stroke (width 0.2) (type solid))
\t\t(layer "Edge.Cuts") (uuid "{uid(f'edge-{index}')}") )''')
    lines.append(")")
    pcb_text = "\n".join(line.rstrip() for line in "\n".join(lines).splitlines()) + "\n"
    (ROOT / "atmosmesh-room.kicad_pcb").write_text(pcb_text, encoding="utf-8")


def write_minimal_footprint_library() -> None:
    lib = ROOT / "room.pretty"
    lib.mkdir(exist_ok=True)
    representatives = {}
    for part in parts:
        representatives.setdefault(part.footprint.split(":", 1)[1], part)
    for name, part in sorted(representatives.items()):
        positions = board_pad_positions(part)
        relative = [(x - part.bx, y - part.by) for x, y in positions]
        body = [f'''(footprint "{name}" (version 20260206) (generator "atmosmesh-room-generator")
  (layer "F.Cu")
  (descr "ROOM-01 provisional THT footprint; verify exact physical part")
  (attr through_hole)
  (fp_rect (start {min(x for x, _ in relative)-1.25} {min(y for _, y in relative)-1.25})
    (end {max(x for x, _ in relative)+1.25} {max(y for _, y in relative)+1.25})
    (stroke (width 0.15) (type solid)) (fill no) (layer "F.Fab")
    (uuid "{uid('lib-silk-' + name)}")){semantic_footprint_markings(name, 'lib-' + name, '  ')}''']
        for index, (x, y) in enumerate(relative, start=1):
            shape = "rect" if index == 1 else "circle"
            body.append(f'''  (pad "{index}" thru_hole {shape} (at {x:.3f} {y:.3f})
    (size 1.7 1.7) (drill 0.8) (layers "*.Cu" "*.Mask")
    (uuid "{uid(f'lib-pad-{name}-{index}')}") )''')
        body.append(")\n")
        text = "\n".join(body)
        (lib / f"{name}.kicad_mod").write_text(text, encoding="utf-8")
    name = "MountingHole_3.2mm_M3"
    (lib / f"{name}.kicad_mod").write_text(f'''(footprint "{name}" (version 20260206) (generator "atmosmesh-room-generator")
  (layer "F.Cu") (attr board_only through_hole exclude_from_pos_files exclude_from_bom)
  (fp_circle (center 0 0) (end 4 0) (stroke (width 0.25) (type solid)) (fill no)
    (layer "F.Fab") (uuid "{uid('lib-mh-circle')}"))
  (pad "" np_thru_hole circle (at 0 0) (size 3.2 3.2) (drill 3.2)
    (layers "*.Cu" "*.Mask") (uuid "{uid('lib-mh-pad')}"))
)\n''', encoding="utf-8")


def assert_no_series_diode_on_sds_rail() -> None:
    """Keep the PIR's 1N5819 idiom off the SDS011 supply.

    The SDS011 minimum is 4.7 V (docs/hardware/spec-comparison.md). A Schottky drops
    roughly 0.3-0.4 V at fan current, which lands under that minimum before any USB
    droop is counted. The default-open jumper gives the same isolation at zero volts.
    """
    diodes = {"D", "D_Schottky"}
    for part in parts:
        if part.lib.split(":")[-1] in diodes and "SDS_5V_PROTECTED" in part.pin_nets:
            raise RuntimeError(
                f"{part.ref} puts a series diode on the SDS011 rail; its minimum is 4.7 V")


def assert_sds011_uart_crossed() -> None:
    """Refuse to emit a straight-through SDS011 UART.

    The 2026-08-17 bench fault was exactly that: the sensor's push-pull TX landed on the
    ESP32's push-pull TX2 and the two fought on one net, roughly 10 ms in every second.
    Only "sensor TX -> an ESP32 receive pin" actually matters, so encode the crossing here
    rather than trusting a wiring table nobody re-reads.
    """
    by_ref = {part.ref: part for part in parts}
    _, _, sensor_rxd, sensor_txd = by_ref["J_SDS"].pin_nets
    rx_leg = by_ref["R_SDS_RX"].pin_nets
    tx_leg = by_ref["R_SDS_TX"].pin_nets
    if sensor_txd not in rx_leg or U1_NETS["GPIO16"] not in rx_leg:
        raise RuntimeError("SDS011 TXD must reach GPIO16/RX2 through R_SDS_RX")
    if sensor_rxd not in tx_leg or U1_NETS["GPIO17"] not in tx_leg:
        raise RuntimeError("SDS011 RXD must reach GPIO17/TX2 through R_SDS_TX")
    if sensor_txd in tx_leg or sensor_rxd in rx_leg:
        raise RuntimeError("SDS011 UART is wired straight through; it must be crossed")


def main() -> None:
    write_project_and_tables()
    write_minimal_footprint_library()
    assert_no_series_diode_on_sds_rail()
    assert_sds011_uart_crossed()
    schematic = write_native_schematic()
    netlist = ROOT / "atmosmesh-room.net"
    subprocess.run(["kicad-cli", "sch", "export", "netlist", "--format", "kicadxml", "-o", str(netlist), str(schematic)], check=True, cwd=ROOT)
    write_board(netlist)
    netlist.unlink()
    print("Generated ROOM-01 KiCad project")


if __name__ == "__main__":
    main()
