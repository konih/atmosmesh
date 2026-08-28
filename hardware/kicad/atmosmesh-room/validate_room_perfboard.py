#!/usr/bin/env python3
"""Structural checks for the ROOM perfboard build plan.

The perfboard doc is the build target, so it needs a gate of its own. Without one it is
prose that can silently drift from the schematic it claims to implement.
"""

from __future__ import annotations

import pathlib
import re
import sys


ROOT = pathlib.Path(__file__).resolve().parent
PERFBOARD = ROOT / "perfboard.md"
PCB = ROOT / "atmosmesh-room.kicad_pcb"

COLUMNS = 31
ROWS = 27
# The ideaspark board is 2x15 pins with the rows 25.4 mm apart, which is exactly 10 pitches.
U1_PINS_PER_ROW = 15
U1_ROW_PITCHES = 10
# D-024: the TFT, the flash strap and the USB-UART own these.
RESERVED_GPIOS = {1, 2, 3, 4, 12, 15, 18, 23, 32}


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    for path in (PERFBOARD, PCB):
        require(path.is_file(), f"missing required file: {path.name}")

    doc = PERFBOARD.read_text(encoding="utf-8")
    pcb = PCB.read_text(encoding="utf-8")

    require(f"**{COLUMNS} × {ROWS} holes**" in doc,
            f"the plan must state the operator's authoritative {COLUMNS} x {ROWS} hole count")

    # The socket must physically fit, with its long axis along the columns.
    require(U1_PINS_PER_ROW <= COLUMNS,
            f"U1 needs {U1_PINS_PER_ROW} columns but the board has {COLUMNS}")
    require(U1_ROW_PITCHES + 1 <= ROWS,
            f"U1 spans {U1_ROW_PITCHES + 1} rows but the board has {ROWS}")

    # Soldering happens mirrored. This warning is load-bearing, not decoration.
    for phrase in ("component side", "mirrored", "authoritative by pin"):
        require(phrase in doc, f"the plan must keep its orientation warning: {phrase}")

    # Dry-fitting settles the row spacing in seconds; the plan must lead with it.
    require("Step 1" in doc and "dry-fit" in doc.lower(),
            "the plan must open with the dry-fit that settles the row spacing")

    # Every part on the board must have somewhere to go.
    references = set(re.findall(r'"Reference" "([A-Z][A-Z0-9_]*)"', pcb))
    references -= {f"H{index}" for index in range(1, 5)}
    missing = sorted(reference for reference in references if reference not in doc)
    require(not missing, f"the plan does not place these schematic parts: {missing}")

    # A reserved pin must not appear as a wiring instruction.
    cited = {int(number) for number in re.findall(r'GPIO(\d+)', doc)}
    clash = sorted(cited & RESERVED_GPIOS)
    require(not clash, f"the plan wires reserved TFT/strap/USB-UART pins: GPIO{clash}")

    print("ROOM perfboard structural validation: PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"ROOM perfboard structural validation: FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
