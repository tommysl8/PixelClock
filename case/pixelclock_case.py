#!/usr/bin/env python3
"""
PixelClock case generator.

Builds the 3D-printable case for:
  - Adafruit 64x64 RGB LED Matrix, 2.5 mm pitch, 45-degree curb-cut (PID 5407)
  - Adafruit Matrix Portal S3 (PID 5778)
  - Adafruit STEMMA Speaker (PID 3885)

and writes STEP (editable CAD) and STL (printable) files.

    case/.venv/Scripts/python case/pixelclock_case.py

Three printed parts:
  display  the frame the panel sits in (front flush with the LEDs)
  foot     the box behind the display that holds the board and speaker
  lid      the foot's lid with the speaker grille

Assembly coordinates (millimetres), used everywhere in this file:
  X  right, as seen from the front
  Y  depth, 0 at the LED face, increasing towards the back
  Z  up, 0 on the desk

Where every dimension comes from (see case/README.md, "Sources"):
  Panel   Adafruit's "64x64 RGB Matrix Bevel" model in Cube_Source_Files.zip
          (RGB LED Matrix Cube guide): 160 x 160 x 15 mm, 3 mm front board,
          150 x 150 housing, 45-degree bevel to a 132 x 132 back face,
          M3 inserts at (+-45, +-61) and (+-61, +-45) mm from centre.
  Board   Adafruit-MatrixPortal-S3-PCB "Adafruit MatrixPortal S3.brd" and
          Adafruit_CAD_Parts "5778 Matrix Portal S3.step".
  Speaker Adafruit-STEMMA-Speaker-PCB "Adafruit STEMMA Speaker.brd" and
          Adafruit_CAD_Parts "3885 STEMMA Speaker rev A.step".
"""
import math
import os
import sys

import gmsh

HERE = os.path.dirname(os.path.abspath(__file__))

# --------------------------------------------------------------------------
# Measured component data (do not change unless the hardware changes)
# --------------------------------------------------------------------------
PANEL_W = 160.0          # front LED board, square
PANEL_DEPTH = 15.0       # LED face to back face of the housing
PANEL_INSERTS = [(sx * 61.0, sy * 45.0) for sx in (-1, 1) for sy in (-1, 1)] + \
                [(sx * 45.0, sy * 61.0) for sx in (-1, 1) for sy in (-1, 1)]
PANEL_IN_OFFSET = 46.5   # input connector centre, from panel centre

BOARD_L, BOARD_W, BOARD_T = 63.5, 44.45, 1.57           # PCB outline (brd)
BOARD_HOLES = [(7.62, 15.875), (7.62, 35.56), (48.26, 35.56), (48.26, 15.875)]  # M2.5, drill 2.5
BOARD_HOLE_DRILL = 2.5
BOARD_KEEPOUT_R = 2.0     # component keep-out around each mounting hole (brd layers 39/40)
BOARD_BOTTOM_PARTS = 5.55  # 2x10 socket JP3 hangs 5.55 below the PCB
BOARD_EDGE_PARTS = (0.57, 5.07)    # USB-C and buttons, height above PCB bottom
BOARD_EDGE_PROTRUDE = 1.072        # side buttons stick out past the PCB edge
BOARD_TALLEST = 12.213             # 2x8 header JP2 top, above PCB bottom

SPK_L, SPK_W, SPK_T = 36.83, 30.48, 1.57   # speaker PCB: Y-length, X-width
SPK_HOLES = [(2.54, 2.54), (27.94, 2.54), (2.54, 34.29), (27.94, 34.29)]  # (sx, sy), M2, drill 2.2
SPK_HOLE_DRILL = 2.2
SPK_KEEPOUT_R = 1.8
SPK_HEIGHT = 6.57                  # top of the speaker above its PCB bottom
SPK_CONE = (0.59, 11.59, 30.19, 31.49)  # speaker body footprint (sx0, sy0, sx1, sy1)
SPK_TRIM = (22.99, 3.05)          # volume trimpot centre (sx, sy)

# --------------------------------------------------------------------------
# Design parameters
# --------------------------------------------------------------------------
WALL = 3.0                # display frame walls
GAP = 0.5                 # clearance around the panel's front board, per side
CAVITY_BACK = 33.0        # inner face of the display back wall (room for cables)
BACK_T = 3.0              # display back wall
DISPLAY_OUT = PANEL_W / 2 + GAP + WALL          # 83.5: half width of the display
PANEL_CZ = WALL + GAP + PANEL_W / 2             # 83.5: panel centre height
CORNER_R = 6.0            # rounded outside corners of the display

PANEL_BOSS_OD = 10.0      # bosses that the panel screws pass through
PANEL_SCREW_HOLE = 3.4    # M3 clearance
PANEL_SCREW_FLOOR = 2.0   # M3x6 screw: 2 mm floor + 4 mm into the brass insert
PANEL_SCREW_BORE = 7.2    # access bore for the screw head and driver
PANEL_SCREWS = [(sx * 61.0, sy * 45.0) for sx in (-1, 1) for sy in (-1, 1)]

WINDOW = (-16.0, 16.0, 8.0, 30.0)   # cable window between display and foot (x0, x1, z0, z1)

FOOT_Y0 = CAVITY_BACK + BACK_T     # 36: foot front face sits on the display back
FOOT_WALL = 3.0                    # foot front wall
FOOT_SIDE = 2.5                    # other foot walls and floor
FOOT_IN_X = 48.0                   # inner half width
FOOT_TOP = 32.0                    # top of the foot walls; lid sits on top
LID_T = 2.5
FOOT_R = 4.0

BOARD_POST_H = 7.0                 # clears the 5.55 mm socket under the board
FLOOR_TOP = FOOT_SIDE
BOARD_Z = FLOOR_TOP + BOARD_POST_H                  # PCB bottom
BOARD_EDGE_Y = 107.0                                # board edge with the USB port
BOARD_EDGE_INSET = 0.4                              # edge sits this far inside the back face
FOOT_Y1 = BOARD_EDGE_Y + BOARD_EDGE_INSET           # 107.4: back face of the foot
BOARD_X0 = -5.45                                    # board 'y' axis runs along +X from here

SPK_X0 = -39.0
SPK_Y0 = 66.0   # speaker JST faces the front; leaves ~11 mm in front of it for the plug
SPK_Z = FOOT_TOP - 1.5 - SPK_HEIGHT                 # speaker PCB bottom; cone 1.5 mm under the lid

PIN_D_BOARD = 2.2          # in 2.5 mm holes
PIN_D_SPK = 1.9            # in 2.2 mm holes
PIN_LEN = 1.3              # pins stop inside the 1.57 mm PCB, so the lid posts can be solid
PRESS_GAP = 0.2            # lid posts stop this far above the boards

# (X, Z) of the M3x10 screws joining foot and display. Kept outside the ribbon's path
# (board header spans X 2.8..30.8) and below the lid's nut slots (Z 25.1..27.9).
JOINT_SCREWS = [(-36.0, 7.0), (36.0, 7.0), (-36.0, 20.0), (36.0, 20.0)]
LID_SCREWS = [(sx * 44.5, y) for sx in (-1, 1) for y in (FOOT_Y0 + FOOT_WALL + 4.5, FOOT_Y1 - FOOT_SIDE - 4.5)]
LID_BOSS_D = 8.0           # 1.15 mm walls each side of the 5.7 mm nut slot

NUT_AF = 5.7               # M3 nut across flats (5.5) + clearance
NUT_T = 2.7                # M3 nut thickness (2.4) + clearance

# ---- derived board placement (board coords bx, by, bz -> assembly X, Y, Z) ----
def board_xyz(bx, by, bz=0.0):
    return BOARD_X0 + by, BOARD_EDGE_Y - bx, BOARD_Z + bz


def speaker_xyz(sx, sy, sz=0.0):
    return SPK_X0 + sx, SPK_Y0 + sy, SPK_Z + sz


# --------------------------------------------------------------------------
# Small OCC helpers
# --------------------------------------------------------------------------
occ = None


def box(x0, x1, y0, y1, z0, z1):
    return (3, occ.addBox(x0, y0, z0, x1 - x0, y1 - y0, z1 - z0))


def cyl_z(x, y, z0, z1, d):
    return (3, occ.addCylinder(x, y, z0, 0, 0, z1 - z0, d / 2))


def cyl_y(x, z, y0, y1, d):
    return (3, occ.addCylinder(x, y0, z, 0, y1 - y0, 0, d / 2))


def rbox_z(x0, x1, y0, y1, z0, z1, r):
    """Box whose vertical (Z) edges are rounded."""
    s = occ.addRectangle(x0, y0, z0, x1 - x0, y1 - y0, roundedRadius=r)
    out = occ.extrude([(2, s)], 0, 0, z1 - z0)
    return [d for d in out if d[0] == 3][0]


def rbox_y(x0, x1, y0, y1, z0, z1, r):
    """Box whose depth (Y) edges are rounded: rounded outline seen from the front."""
    # Draw the outline in XY (its Y standing in for Z), extrude along Z by the depth,
    # then rotate +90 deg about X: (x, y, z) -> (x, -z, y), and mirror y -> -y so the
    # depth runs along +Y. Result: X x0..x1, Y y0..y1, Z z0..z1.
    s = occ.addRectangle(x0, z0, 0, x1 - x0, z1 - z0, roundedRadius=r)
    vol = [d for d in occ.extrude([(2, s)], 0, 0, y1 - y0) if d[0] == 3][0]
    occ.rotate([vol], 0, 0, 0, 1, 0, 0, math.pi / 2)
    occ.mirror([vol], 0, 1, 0, 0)
    occ.translate([vol], 0, y0, 0)
    return vol


def hex_prism_y(x, z, y0, y1, af):
    """Hex nut pocket with its axis along Y, flats top and bottom."""
    r = af / math.sqrt(3)  # circumradius from across-flats
    pts = []
    for i in range(6):
        a = math.radians(60 * i)  # vertex at 0 deg -> flats at top/bottom
        pts.append(occ.addPoint(x + r * math.cos(a), y0, z + r * math.sin(a)))
    lines = [occ.addLine(pts[i], pts[(i + 1) % 6]) for i in range(6)]
    loop = occ.addCurveLoop(lines)
    s = occ.addPlaneSurface([loop])
    return [d for d in occ.extrude([(2, s)], 0, y1 - y0, 0) if d[0] == 3][0]


def fuse(parts):
    out, _ = occ.fuse([parts[0]], parts[1:])
    return out


def cut(obj, tools):
    out, _ = occ.cut(obj if isinstance(obj, list) else [obj], tools)
    return out


# --------------------------------------------------------------------------
# Parts
# --------------------------------------------------------------------------
def build_display():
    """Frame + back wall that the panel sits in, LED face flush with the front."""
    o = DISPLAY_OUT
    top = PANEL_CZ + PANEL_W / 2 + GAP + WALL  # 167
    shell = rbox_y(-o, o, 0, CAVITY_BACK + BACK_T, 0, top, CORNER_R)
    inner = box(-(PANEL_W / 2 + GAP), PANEL_W / 2 + GAP, -1, CAVITY_BACK, WALL, top - WALL)
    body = cut(shell, [inner])

    # bosses the panel screws through, touching the panel back face (Y = 15)
    bosses = [cyl_y(x, PANEL_CZ + z, PANEL_DEPTH, CAVITY_BACK + 0.01, PANEL_BOSS_OD) for x, z in PANEL_SCREWS]
    body = fuse(body + bosses)

    tools = []
    for x, z in PANEL_SCREWS:
        zz = PANEL_CZ + z
        tools.append(cyl_y(x, zz, PANEL_DEPTH - 1, CAVITY_BACK + BACK_T + 1, PANEL_SCREW_HOLE))
        tools.append(cyl_y(x, zz, PANEL_DEPTH + PANEL_SCREW_FLOOR, CAVITY_BACK + BACK_T + 1, PANEL_SCREW_BORE))
    # cable window into the foot
    x0, x1, z0, z1 = WINDOW
    tools.append(box(x0, x1, CAVITY_BACK - 1, CAVITY_BACK + BACK_T + 1, z0, z1))
    # joint screws to the foot (heads inside the display cavity)
    for x, z in JOINT_SCREWS:
        tools.append(cyl_y(x, z, CAVITY_BACK - 1, CAVITY_BACK + BACK_T + 1, PANEL_SCREW_HOLE))
    # ventilation slots high on the back
    for i in range(9):
        x = -40 + 10 * i
        tools.append(box(x - 1.25, x + 1.25, CAVITY_BACK - 1, CAVITY_BACK + BACK_T + 1, 132, 152))
    return cut(body, tools)


def build_foot():
    """Box behind the display holding the board (USB at the back) and speaker."""
    xo = FOOT_IN_X + FOOT_SIDE
    shell = rbox_z(-xo, xo, FOOT_Y0, FOOT_Y1, 0, FOOT_TOP, FOOT_R)
    inner = box(-FOOT_IN_X, FOOT_IN_X, FOOT_Y0 + FOOT_WALL, FOOT_Y1 - FOOT_SIDE, FLOOR_TOP, FOOT_TOP + 1)
    body = cut(shell, [inner])

    adds = []
    # board posts with locating pins
    for bx, by in BOARD_HOLES:
        x, y, _ = board_xyz(bx, by)
        adds.append(cyl_z(x, y, FLOOR_TOP - 0.01, BOARD_Z, 5.0))
        adds.append(cyl_z(x, y, BOARD_Z - 0.01, BOARD_Z + PIN_LEN, PIN_D_BOARD))
    # speaker posts with locating pins
    for sx, sy in SPK_HOLES:
        x, y, _ = speaker_xyz(sx, sy)
        adds.append(cyl_z(x, y, FLOOR_TOP - 0.01, SPK_Z, 5.0))
        adds.append(cyl_z(x, y, SPK_Z - 0.01, SPK_Z + PIN_LEN, PIN_D_SPK))
    # joint pillars on the front wall (nuts for the display screws)
    for x, z in JOINT_SCREWS:
        adds.append(box(x - 4.5, x + 4.5, FOOT_Y0 + FOOT_WALL - 0.01, FOOT_Y0 + FOOT_WALL + 3.5, FLOOR_TOP - 0.01, z + 4.5))
    # lid screw bosses in the corners
    for x, y in LID_SCREWS:
        adds.append(cyl_z(x, y, FLOOR_TOP - 0.01, FOOT_TOP, LID_BOSS_D))
        # tie the boss to the side wall
        wx = FOOT_IN_X + 0.01 if x > 0 else -FOOT_IN_X - 0.01
        adds.append(box(min(x, wx), max(x, wx), y - LID_BOSS_D / 2, y + LID_BOSS_D / 2, FLOOR_TOP - 0.01, FOOT_TOP))
    body = fuse(body + adds)

    tools = []
    # cable window (matches the display's)
    x0, x1, z0, z1 = WINDOW
    tools.append(box(x0, x1, FOOT_Y0 - 1, FOOT_Y0 + FOOT_WALL + 1, z0, z1))
    # board-edge slot in the back wall, open at the top (lid tongue closes it)
    sx0, _, _ = board_xyz(0, 0)
    sx1, _, _ = board_xyz(0, BOARD_W)
    tools.append(box(sx0 - 0.4, sx1 + 0.4, FOOT_Y1 - FOOT_SIDE - 1, FOOT_Y1 + 1, BOARD_Z - 0.4, FOOT_TOP + 1))
    # joint screws + nut pockets
    for x, z in JOINT_SCREWS:
        tools.append(cyl_y(x, z, FOOT_Y0 - 1, FOOT_Y0 + FOOT_WALL + 4, PANEL_SCREW_HOLE))
        tools.append(hex_prism_y(x, z, FOOT_Y0 + FOOT_WALL + 3.5 - NUT_T, FOOT_Y0 + FOOT_WALL + 3.6, NUT_AF))
    # lid screws: hole from the top, nut slot entering from the inside. A nut slides in
    # flats-first, so a corner leads: the slot runs a nut's corner radius (+0.2) past the
    # screw axis so a nut pushed to the end sits centred on the hole.
    for x, y in LID_SCREWS:
        tools.append(cyl_z(x, y, FOOT_TOP - 9.0, FOOT_TOP + 1, PANEL_SCREW_HOLE))
        nz = FOOT_TOP - 5.5
        inward = -1 if x > 0 else 1
        a, b = x - inward * (5.5 / math.sqrt(3) + 0.2), x + inward * 6.5
        tools.append(box(min(a, b), max(a, b), y - NUT_AF / 2, y + NUT_AF / 2, nz - NUT_T / 2, nz + NUT_T / 2))
    return cut(body, tools)


def build_lid():
    """Lid with speaker grille, volume-knob hole, and posts that clamp the boards."""
    xo = FOOT_IN_X + FOOT_SIDE
    plate = rbox_z(-xo, xo, FOOT_Y0, FOOT_Y1, FOOT_TOP, FOOT_TOP + LID_T, FOOT_R)
    adds = []
    # tongue that closes the board-edge slot above the USB port and buttons
    sx0, _, _ = board_xyz(0, 0)
    sx1, _, _ = board_xyz(0, BOARD_W)
    tongue_z0 = BOARD_Z + BOARD_EDGE_PARTS[1] + 0.5
    adds.append(box(sx0 - 0.25, sx1 + 0.25, FOOT_Y1 - FOOT_SIDE - 0.0, FOOT_Y1, tongue_z0, FOOT_TOP + 0.01))
    # press posts onto the board around its mounting holes
    for bx, by in BOARD_HOLES:
        x, y, _ = board_xyz(bx, by)
        adds.append(cyl_z(x, y, BOARD_Z + BOARD_T + PRESS_GAP, FOOT_TOP + 0.01, 2 * BOARD_KEEPOUT_R - 0.3))
    for sx, sy in SPK_HOLES:
        x, y, _ = speaker_xyz(sx, sy)
        adds.append(cyl_z(x, y, SPK_Z + SPK_T + PRESS_GAP, FOOT_TOP + 0.01, 2 * SPK_KEEPOUT_R - 0.2))
    lid = fuse([plate] + adds)

    tools = []
    # lid screws
    for x, y in LID_SCREWS:
        tools.append(cyl_z(x, y, FOOT_TOP - 1, FOOT_TOP + LID_T + 1, PANEL_SCREW_HOLE))
    # speaker grille: hex pattern of 2.2 mm holes inside the speaker outline
    cx0, cy0, cx1, cy1 = SPK_CONE
    gx0, gy0, _ = speaker_xyz(cx0 + 1.5, cy0 + 1.5)
    gx1, gy1, _ = speaker_xyz(cx1 - 1.5, cy1 - 1.5)
    ex, ey = (gx0 + gx1) / 2, (gy0 + gy1) / 2
    ax, ay = (gx1 - gx0) / 2, (gy1 - gy0) / 2
    pitch = 3.6
    row = 0
    y = ey - ay
    while y <= ey + ay + 1e-6:
        x = ex - ax + (pitch / 2 if row % 2 else 0)
        while x <= ex + ax + 1e-6:
            if ((x - ex) / ax) ** 2 + ((y - ey) / ay) ** 2 <= 1.0:
                tools.append(cyl_z(x, y, FOOT_TOP - 1, FOOT_TOP + LID_T + 1, 2.2))
            x += pitch
        y += pitch * math.sqrt(3) / 2
        row += 1
    # volume knob access
    tx, ty, _ = speaker_xyz(*SPK_TRIM)
    tools.append(cyl_z(tx, ty, FOOT_TOP - 1, FOOT_TOP + LID_T + 1, 3.2))
    return cut(lid, tools)


PARTS = {"display": build_display, "foot": build_foot, "lid": build_lid}

# How each part should sit on the print bed: rotation (axis, angle) then it is
# dropped to Z=0 and centred.
PRINT_POSE = {
    "display": ((1, 0, 0), -math.pi / 2),  # back face down, front rim up
    "foot": None,                          # floor down
    "lid": ((1, 0, 0), math.pi),           # top face down, posts up
}


def init():
    global occ
    if not gmsh.isInitialized():
        gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    occ = gmsh.model.occ
    return occ


def build(name):
    """Build one part in the current model and return its dimTags (assembly position)."""
    init()
    tags = PARTS[name]()
    occ.synchronize()
    return tags


def export(out_dir):
    init()
    os.makedirs(os.path.join(out_dir, "stl"), exist_ok=True)
    os.makedirs(os.path.join(out_dir, "step"), exist_ok=True)
    # assembly STEP with all parts in place
    gmsh.clear()
    gmsh.model.add("assembly")
    for name in PARTS:
        build(name)
    gmsh.write(os.path.join(out_dir, "step", "pixelclock_case_assembly.step"))

    for name in PARTS:
        gmsh.clear()
        gmsh.model.add(name)
        tags = build(name)
        gmsh.write(os.path.join(out_dir, "step", f"pixelclock_{name}.step"))
        pose = PRINT_POSE[name]
        if pose:
            occ.rotate(tags, 0, 0, 0, *pose[0], pose[1])
        occ.synchronize()
        bb = gmsh.model.getBoundingBox(-1, -1)
        occ.translate(tags, -(bb[0] + bb[3]) / 2, -(bb[1] + bb[4]) / 2, -bb[2])
        occ.synchronize()
        gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 36)
        gmsh.option.setNumber("Mesh.MeshSizeMax", 4.0)
        gmsh.option.setNumber("Mesh.MeshSizeMin", 0.05)
        gmsh.option.setNumber("Mesh.Algorithm", 6)
        gmsh.option.setNumber("Mesh.Binary", 1)
        gmsh.model.mesh.generate(2)
        gmsh.write(os.path.join(out_dir, "stl", f"pixelclock_{name}.stl"))
        bb = gmsh.model.getBoundingBox(-1, -1)
        print(f"{name:8s} print size {bb[3]-bb[0]:.1f} x {bb[4]-bb[1]:.1f} x {bb[5]-bb[2]:.1f} mm")


if __name__ == "__main__":
    export(sys.argv[1] if len(sys.argv) > 1 else HERE)
    gmsh.finalize()
