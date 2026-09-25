"""AtmosMesh Aura: parametric desk enclosure for an ESP32-2432S028 (CYD, USB-C).

Portrait, standing upright on a removable tilted cradle. Four printed parts:

    front   bezel + walls; the CYD screws into it from behind (M3)
    baffle  thermal divider between CYD and sensor bay; carries the 2x8 cm perfboard
    rear    sensor bay walls + back grille + LED "halo" window
    cradle  tilted stand with a cable channel for a 90-degree USB-C plug

Frame used throughout (assembled, before tilt):
    X  width, left -> right as seen from the front
    Y  depth, 0 = front face, +Y towards the back
    Z  height, 0 = case bottom, +Z up (USB-C end of the CYD is at the bottom)

Every dimension marked VERIFY is a guess or a reading from a photo; measure it
on the real board before the final print. Run:  python aura_case.py  (writes ./out)
"""

from __future__ import annotations

import math
import zipfile
from pathlib import Path

from build123d import (
    Align,
    Axis,
    Box,
    Cylinder,
    Plane,
    Pos,
    Rectangle,
    RectangleRounded,
    Rot,
    FontStyle,
    Text,
    export_step,
    export_stl,
    extrude,
    fillet,
    loft,
)

# ---------------------------------------------------------------- board facts
PCB_W = 50.5  # measured (operator)
PCB_L = 85.5  # measured (operator)
PCB_T = 1.6
HOLE_DX = 42.0  # M3 hole centres across the width  (reference case; VERIFY)
HOLE_DZ = 78.0  # M3 hole centres along the length (reference case; VERIFY)
DISP_H = 4.3  # PCB front face -> top of touch glass (VERIFY with calipers)
BACK_H = 7.0  # tallest part on the back incl. plugged pigtail (VERIFY)
ACTIVE_W = 43.2  # 2.8" 240x320 active area
ACTIVE_L = 57.6
ACTIVE_SHIFT = 2.0  # active-area centre above PCB centre, away from USB (VERIFY)
WIN_MARGIN = 1.0  # extra bezel opening around the lit pixels, per side
LDR_X, LDR_Z = 10.8, 82.2  # LDR centre from PCB left edge / bottom edge (photo)
LED_X, LED_Z = 12.0, 55.5  # back RGB LED centre, same reference (VERIFY)
CN1_Z = 50.0  # pigtail header centre from PCB bottom edge, left side (photo)
USB_X_OFF = 0.0  # USB-C centre offset from PCB centre line
USB_W, USB_H = 12.5, 7.0  # opening for the plug overmold

# ----------------------------------------------------------------- perfboard
PB_W, PB_L, PB_T = 20.0, 80.0, 1.6
PB_HOLE_DX, PB_HOLE_DZ = 16.0, 76.0  # 14 / 74 inner edge + 2 mm holes (operator)
PB_STANDOFF = 4.0  # solder-side clearance
PB_CLEAR = 10.0  # component-side clearance to the back wall (operator)
PB_X_OFF = 5.0  # shift right of centre, clear of the LED light path

# ----------------------------------------------------------------- enclosure
CLR = 0.4  # PCB-to-wall clearance per side
GUTTER = 4.5  # side gutters for the pigtail and the case screws
WALL = 2.0
FRONT_T = 2.0
GLASS_GAP = 0.3
BAFFLE_T = 2.0
CORNER_R = 6.0  # outline corner radius (seen from the front)
EDGE_R = 1.6  # front and back edge rounding
M3_PILOT = 2.6  # thread-forming M3 into PLA/PETG
M3_CLEAR = 3.4
M3_HEAD_D, M3_HEAD_H = 6.2, 3.0
M2_PILOT = 1.7
SCREW_Z_INSET = 10.0  # case screws: distance from top/bottom outer faces

# ----------------------------------------------------------------- lettering
WORDMARK = "AtmosMesh Aura"  # front chin (product name, D-037)
MAKER_LINE = "Made by Konrad Heimel"  # back plate
DATE_LINE = "2026-09-25"
INLAY_DEPTH = 0.6

# ------------------------------------------------------------------ cradle
TILT_DEG = 8.0  # lean back; 0 = bolt upright
SOCKET_H = 5.0  # how far the case sits down into the cradle
CRADLE_WALL = 3.0
CRADLE_CLR = 0.3
PLUG_DROP = 15.0  # room below the case for a 90-degree USB-C plug
CRADLE_FLOOR = 2.0

# ------------------------------------------------------------------ derived
W_IN = PCB_W + 2 * CLR + 2 * GUTTER
H_IN = PCB_L + 2 * CLR
W = W_IN + 2 * WALL
H = H_IN + 2 * WALL
PCB_X0 = WALL + GUTTER + CLR  # PCB left edge
PCB_Z0 = WALL + CLR  # PCB bottom edge
Y_PCB_F = FRONT_T + GLASS_GAP + DISP_H
Y_PCB_B = Y_PCB_F + PCB_T
Y1 = Y_PCB_B + BACK_H + 0.8  # front part ends / baffle starts
Y2 = Y1 + BAFFLE_T
Y_PB = Y2 + PB_STANDOFF  # perfboard solder side
Y3 = Y_PB + PB_T + PB_CLEAR  # back wall inner face
D = Y3 + WALL
CX = W / 2
WIN_CZ = PCB_Z0 + PCB_L / 2 + ACTIVE_SHIFT
WIN_W = ACTIVE_W + 2 * WIN_MARGIN
WIN_L = ACTIVE_L + 2 * WIN_MARGIN
SCREW_X = (WALL + GUTTER / 2 - 0.05, W - WALL - GUTTER / 2 + 0.05)
SCREW_Z = (SCREW_Z_INSET, H - SCREW_Z_INSET)
SCREWS = [(x, z) for x in SCREW_X for z in SCREW_Z]
CYD_HOLES = [
    (PCB_X0 + PCB_W / 2 + sx * HOLE_DX / 2, PCB_Z0 + PCB_L / 2 + sz * HOLE_DZ / 2)
    for sx in (-1, 1)
    for sz in (-1, 1)
]
LED = (PCB_X0 + LED_X, PCB_Z0 + LED_Z)
PB_CX, PB_CZ = CX + PB_X_OFF, H / 2
USB_CX = PCB_X0 + PCB_W / 2 + USB_X_OFF
USB_CY = Y_PCB_B + 1.6  # receptacle sits on the back of the PCB


# ------------------------------------------------------------------ helpers
def front_plane(y: float) -> Plane:
    """Sketch plane parallel to the front face at depth y: local x = X, local y = Z,
    normal -Y (towards the viewer), so text reads correctly from the front."""
    return Plane(origin=(0, y, 0), x_dir=(1, 0, 0), z_dir=(0, -1, 0))


def slab(w: float, h: float, r: float, y0: float, y1: float, cx=None, cz=None):
    """Rounded rectangle (seen from the front) extruded from y0 to y1."""
    cx = CX if cx is None else cx
    cz = H / 2 if cz is None else cz
    b = Pos(cx, y0, cz) * Box(w, y1 - y0, h, align=(Align.CENTER, Align.MIN, Align.CENTER))
    return fillet(b.edges().filter_by(Axis.Y), r) if r > 0 else b


def ycyl(x: float, z: float, d: float, y0: float, y1: float):
    return Pos(x, (y0 + y1) / 2, z) * Rot(90, 0, 0) * Cylinder(d / 2, abs(y1 - y0))


def box(x0, x1, y0, y1, z0, z1):
    return Pos(x0, y0, z0) * Box(x1 - x0, y1 - y0, z1 - z0, align=(Align.MIN,) * 3)


def zslots(x0, x1, y0, y1, z_wall0, z_wall1, width=2.0, pitch=4.0):
    """Slots through a top/bottom wall, long along Y (prints bridge-free face-down)."""
    n = int((x1 - x0 - width) // pitch) + 1
    start = (x0 + x1) / 2 - (n - 1) * pitch / 2
    out = None
    for i in range(n):
        x = start + i * pitch
        s = box(x - width / 2, x + width / 2, y0, y1, z_wall0, z_wall1)
        out = s if out is None else out + s
    return out


def xslots(z0, z1, y0, y1, x_wall0, x_wall1, width=2.0, pitch=4.0):
    """Slots through a side wall, long along Y."""
    n = int((z1 - z0 - width) // pitch) + 1
    start = (z0 + z1) / 2 - (n - 1) * pitch / 2
    out = None
    for i in range(n):
        z = start + i * pitch
        s = box(x_wall0, x_wall1, y0, y1, z - width / 2, z + width / 2)
        out = s if out is None else out + s
    return out


# -------------------------------------------------------------------- parts
def make_front():
    body = slab(W, H, CORNER_R, 0, Y1)
    body = fillet(body.edges().filter_by(lambda e: e.center().Y < 1e-3), EDGE_R)
    body -= slab(W_IN, H_IN, CORNER_R - WALL, FRONT_T, Y1 + 1)

    # display window, 45-degree chamfer out to the front face
    inner = front_plane(FRONT_T + 0.01) * Pos(CX, WIN_CZ) * Rectangle(WIN_W, WIN_L)
    outer = front_plane(-0.01) * Pos(CX, WIN_CZ) * Rectangle(
        WIN_W + 2 * FRONT_T, WIN_L + 2 * FRONT_T
    )
    body -= loft([outer, inner])

    # CYD bosses (stop at the PCB front face), pilot does not break the front
    for x, z in CYD_HOLES:
        body += ycyl(x, z, 5.5, FRONT_T - 0.01, Y_PCB_F)
        body -= ycyl(x, z, M3_PILOT, FRONT_T, Y_PCB_F + 0.1)
    # case-screw bosses in the gutters
    for x, z in SCREWS:
        body += ycyl(x, z, 5.0, FRONT_T - 0.01, Y1)
        body -= ycyl(x, z, M3_PILOT, FRONT_T + 1.2, Y1 + 0.1)

    # LDR light hole with a small front chamfer
    lx, lz = PCB_X0 + LDR_X, PCB_Z0 + LDR_Z
    body -= ycyl(lx, lz, 2.6, -0.1, FRONT_T + 0.1)
    # USB-C through the bottom wall
    body -= box(
        USB_CX - USB_W / 2, USB_CX + USB_W / 2,
        USB_CY - USB_H / 2, USB_CY + USB_H / 2, -0.1, WALL + 0.1,
    )
    # CYD chamber exhaust: top wall, behind the PCB
    body -= zslots(CX - 18, CX + 18, Y_PCB_B + 0.5, Y1 - 0.8, H - WALL - 0.1, H + 0.1)
    # CYD chamber intake: bottom wall either side of the USB opening
    body -= zslots(CX - 26, USB_CX - USB_W / 2 - 2, Y_PCB_B + 0.5, Y1 - 0.8, -0.1, WALL + 0.1)
    body -= zslots(USB_CX + USB_W / 2 + 2, CX + 26, Y_PCB_B + 0.5, Y1 - 0.8, -0.1, WALL + 0.1)
    return body


def make_front_inlay():
    """Accent ring around the window + wordmark, 0.6 mm deep in the front face."""
    depth = INLAY_DEPTH
    ring_off = FRONT_T + 1.8
    ring = extrude(
        front_plane(depth) * Pos(CX, WIN_CZ) * RectangleRounded(
            WIN_W + 2 * (ring_off + 1.2), WIN_L + 2 * (ring_off + 1.2), 3.0
        ),
        amount=depth,
    ) - extrude(
        front_plane(depth + 0.05) * Pos(CX, WIN_CZ) * RectangleRounded(
            WIN_W + 2 * ring_off, WIN_L + 2 * ring_off, 1.8
        ),
        amount=depth + 0.1,
    )
    chin_top = WIN_CZ - WIN_L / 2 - ring_off - 1.2
    chin_mid = (SOCKET_H + chin_top) / 2
    txt = Text(WORDMARK, font_size=4.2, font_style=FontStyle.BOLD, align=(Align.CENTER, Align.CENTER))
    word = extrude(front_plane(depth) * Pos(CX, chin_mid) * txt, amount=depth)
    return ring, word


def make_baffle():
    b = slab(W, H, CORNER_R, Y1, Y2)
    # registration lips into both halves (light and air seal)
    lip_o = (W_IN - 0.4, H_IN - 0.4, CORNER_R - WALL - 0.2)
    lip_i = (W_IN - 2.8, H_IN - 2.8, max(CORNER_R - WALL - 1.4, 0.5))
    for y0, y1 in ((Y2 - 0.01, Y2 + 1.5),):
        lip = slab(*lip_o, y0, y1) - slab(*lip_i, y0 - 0.1, y1 + 0.1)
        for x, z in SCREWS:
            lip -= ycyl(x, z, 7.0, y0 - 0.1, y1 + 0.1)
        b += lip
    for x, z in SCREWS:
        b -= ycyl(x, z, M3_CLEAR, Y1 - 2, Y2 + 2)
    # pigtail pass-through in the left gutter
    gx0, gx1 = WALL + 0.6, PCB_X0 - 0.2
    cz = PCB_Z0 + CN1_Z
    b -= box(gx0, gx1, Y1 - 2, Y2 + 2, cz - 7, cz + 7)
    # LED light path
    b -= ycyl(*LED, 9.0, Y1 - 2, Y2 + 2)
    # perfboard standoffs on the back side
    for sx in (-1, 1):
        for sz in (-1, 1):
            x, z = PB_CX + sx * PB_HOLE_DX / 2, PB_CZ + sz * PB_HOLE_DZ / 2
            b += ycyl(x, z, 5.0, Y2 - 0.01, Y_PB)
            b -= ycyl(x, z, M2_PILOT, Y2 - 1.0, Y_PB + 0.1)
    return b


HALO_D = 13.0
HALO_RING = (16.0, 18.4)


def make_rear():
    body = slab(W, H, CORNER_R, Y2, D)
    body = fillet(body.edges().filter_by(lambda e: e.center().Y > D - 1e-3), EDGE_R)
    body -= slab(W_IN, H_IN, CORNER_R - WALL, Y2 - 0.1, Y3)
    for x, z in SCREWS:
        body += ycyl(x, z, 6.6, Y2, D - 0.01)
        body -= ycyl(x, z, M3_CLEAR, Y2 - 0.1, D + 0.1)
        body -= ycyl(x, z, M3_HEAD_D, D - M3_HEAD_H, D + 0.1)
    # sensor bay intake low, exhaust high: side walls, top wall, back grille
    ys = (Y2 + 2.5, Y3 - 1.5)
    for xw in ((-0.1, WALL + 0.1), (W - WALL - 0.1, W + 0.1)):
        body -= xslots(16, 34, *ys, *xw)
        body -= xslots(58, 76, *ys, *xw)
    body -= zslots(CX - 20, CX + 20, *ys, H - WALL - 0.1, H + 0.1)
    for z0, z1 in ((14, 32), (70, 82)):
        for i in range(int((z1 - z0) // 4) + 1):
            z = z0 + i * 4
            body -= box(CX - 16 + PB_X_OFF, CX + 16 + PB_X_OFF, Y3 - 0.1, D + 0.1, z - 1, z + 1)
    # halo window pockets (filled by the translucent inlay)
    return body


def back_plane(y: float) -> Plane:
    """Sketch plane on the back face: reads correctly from behind, normal +Y."""
    return Plane(origin=(0, y, 0), x_dir=(-1, 0, 0), z_dir=(0, 1, 0))


def make_back_text():
    """Maker line and date, inlaid in the back plate between lower grille and halo."""
    d = INLAY_DEPTH
    lines = ((MAKER_LINE, 3.4, 44.0), (DATE_LINE, 3.0, 38.5))
    out = None
    for text, size, z in lines:
        t = Text(text, font_size=size, font_style=FontStyle.BOLD, align=(Align.CENTER, Align.CENTER))
        # back_plane local x runs along -X, so the case centre is at local x = -CX
        solid = extrude(back_plane(D - d) * Pos(-CX, z) * t, amount=d)
        out = solid if out is None else out + solid
    return out


def make_halo():
    disk = ycyl(*LED, HALO_D, Y3 - 0.01, D)
    ring = ycyl(*LED, HALO_RING[1], D - 0.8, D) - ycyl(*LED, HALO_RING[0], D - 0.9, D + 0.1)
    return disk + ring


def ground_z(y: float, y_back: float, h_back: float) -> float:
    return -h_back - (y_back - y) * math.tan(math.radians(TILT_DEG))


def make_cradle():
    c = CRADLE_CLR
    wo, do = W + 2 * (c + CRADLE_WALL), D + 2 * (c + CRADLE_WALL)
    y_front, y_back = -c - CRADLE_WALL, D + c + CRADLE_WALL
    t = math.tan(math.radians(TILT_DEG))
    # back height chosen so the plug pocket under the USB is PLUG_DROP deep
    h_back = PLUG_DROP + CRADLE_FLOOR - (y_back - USB_CY - USB_H) * t
    z_lo = ground_z(y_front, y_back, h_back) - 1
    blk = Pos(CX, D / 2, z_lo) * Box(wo, do, SOCKET_H - z_lo, align=(Align.CENTER, Align.CENTER, Align.MIN))
    blk = fillet(blk.edges().filter_by(Axis.Z), CRADLE_WALL + 2)
    # cut below the tilted ground plane
    ang = TILT_DEG
    cutter = Pos(CX, y_back, -h_back) * Rot(ang, 0, 0) * Box(
        wo * 2, do * 4, 80, align=(Align.CENTER, Align.CENTER, Align.MAX)
    )
    blk -= cutter
    # socket for the case
    sock = Pos(CX, D / 2, 0) * Box(W + 2 * c, D + 2 * c, SOCKET_H + 1, align=(Align.CENTER, Align.CENTER, Align.MIN))
    blk -= fillet(sock.edges().filter_by(Axis.Z), EDGE_R + c)
    # plug pocket under the USB opening, and a cable channel out the back
    pocket_w = 22.0
    floor = Pos(CX, y_back, -h_back) * Rot(ang, 0, 0) * Pos(0, 0, CRADLE_FLOOR)
    pocket = Pos(USB_CX, 0, 0) * box(-pocket_w / 2, pocket_w / 2, 1.0, y_back + 5, -60, 1)
    pocket -= floor * Box(wo * 2, do * 4, 80, align=(Align.CENTER, Align.CENTER, Align.MAX))
    blk -= pocket
    # rubber-foot recesses (8 mm bumpons)
    for fx in (-1, 1):
        for fy in (0.18, 0.82):
            y = y_front + fy * do
            x = CX + fx * (wo / 2 - 9)
            foot = Pos(x, y, ground_z(y, y_back, h_back)) * Rot(ang, 0, 0) * Cylinder(4.2, 1.6)
            blk -= foot
    return blk, h_back



def make_stand_ins():
    """Board stand-ins (not printed): CYD PCB, display, perfboard, for fit checks in CAD."""
    pcb = Pos(PCB_X0, Y_PCB_F, PCB_Z0) * Box(PCB_W, PCB_T, PCB_L, align=(Align.MIN,) * 3)
    glass = Pos(CX, Y_PCB_F - DISP_H, PCB_Z0 + PCB_L / 2) * Box(
        49, DISP_H, 69, align=(Align.CENTER, Align.MIN, Align.CENTER)
    )
    pb = Pos(PB_CX, Y_PB, PB_CZ) * Box(PB_W, PB_T, PB_L, align=(Align.CENTER, Align.MIN, Align.CENTER))
    return {"standin_cyd_pcb": pcb, "standin_cyd_display": glass, "standin_perfboard": pb}



def write_3mf(path: Path, parts: dict[str, object]) -> None:
    """One 3MF object made of named components, so a slicer loads the group as a
    single multi-part object (assign a filament per part in Bambu Studio)."""
    objs, comps = [], []
    for i, (name, shape) in enumerate(parts.items(), start=1):
        verts, tris = shape.tessellate(0.02, 0.15)
        v = "".join(f'<vertex x="{p.X:.4f}" y="{p.Y:.4f}" z="{p.Z:.4f}"/>' for p in verts)
        t = "".join(f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in tris)
        objs.append(
            f'<object id="{i}" name="{name}" type="model"><mesh><vertices>{v}</vertices>'
            f"<triangles>{t}</triangles></mesh></object>"
        )
        comps.append(f'<component objectid="{i}"/>')
    top = len(parts) + 1
    model = (
        '<?xml version="1.0" encoding="UTF-8"?>'
        '<model unit="millimeter" xml:lang="en-US" '
        'xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">'
        f"<resources>{''.join(objs)}"
        f'<object id="{top}" name="{path.stem}" type="model"><components>{"".join(comps)}'
        f'</components></object></resources><build><item objectid="{top}"/></build></model>'
    )
    rels = (
        '<?xml version="1.0" encoding="UTF-8"?><Relationships '
        'xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Target="/3D/3dmodel.model" Id="rel0" '
        'Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>'
    )
    types = (
        '<?xml version="1.0" encoding="UTF-8"?><Types '
        'xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
        "</Types>"
    )
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", types)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", model)


# -------------------------------------------------------------------- build
def tilt(shape, h_back):
    """Lean the assembly back so the cradle's ground plane is horizontal."""
    y_back = D + CRADLE_CLR + CRADLE_WALL
    return shape.rotate(Axis((0, y_back, -h_back), (1, 0, 0)), -TILT_DEG)


PRINT_GROUPS = {
    "aura_front.3mf": ("front_body", "front_accent_ring", "front_accent_text"),
    "aura_rear.3mf": ("rear_body", "rear_halo_translucent", "rear_accent_text"),
    "aura_baffle.3mf": ("baffle",),
    "aura_cradle.3mf": ("cradle",),
}


def main():
    out = Path(__file__).parent / "out"
    out.mkdir(exist_ok=True)

    front = make_front()
    ring, word = make_front_inlay()
    front = front - ring - word
    baffle = make_baffle()
    rear = make_rear()
    halo = make_halo()
    back_text = make_back_text()
    rear = rear - halo - back_text
    cradle, h_back = make_cradle()

    parts = {
        "front_body": front,
        "front_accent_ring": ring,
        "front_accent_text": word,
        "baffle": baffle,
        "rear_body": rear,
        "rear_halo_translucent": halo,
        "rear_accent_text": back_text,
        "cradle": cradle,
    }

    # assembled STEP (upright, untilted), plus board stand-ins for fit checks
    for name, p in {**parts, **make_stand_ins()}.items():
        export_step(p, str(out / f"{name}.step"))

    # print orientation
    face_down = Rot(90, 0, 0)  # front face (y=0) onto the bed
    back_down = Rot(-90, 0, 0)  # back face (y=D) onto the bed
    printable = {
        "front_body": face_down * front,
        "front_accent_ring": face_down * ring,
        "front_accent_text": face_down * word,
        "baffle": face_down * baffle,
        "rear_body": back_down * rear,
        "rear_halo_translucent": back_down * halo,
        "rear_accent_text": back_down * back_text,
        "cradle": tilt(cradle, h_back),
    }
    # drop each print group onto the bed together, so multi-colour parts stay aligned
    for group in PRINT_GROUPS.values():
        z0 = min(printable[g].bounding_box().min.Z for g in group)
        for g in group:
            printable[g] = Pos(0, 0, -z0) * printable[g]
    for name, p in printable.items():
        export_stl(p, str(out / f"{name}.stl"), tolerance=0.02, angular_tolerance=0.15)

    for fname, group in PRINT_GROUPS.items():
        write_3mf(out / fname, {g: printable[g] for g in group})

    print(f"outer W x H x D = {W:.1f} x {H:.1f} x {D:.1f} mm, cradle back height {h_back:.1f} mm")
    print(f"window centre z={WIN_CZ:.2f}, size {WIN_W:.1f} x {WIN_L:.1f}")
    print(f"case screw: head seat to boss top {D - M3_HEAD_H - Y1:.1f} mm -> 4x M3x25; CYD 4x M3x6; perfboard 4x M2x5")
    return parts, h_back


if __name__ == "__main__":
    main()
