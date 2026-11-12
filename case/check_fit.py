#!/usr/bin/env python3
"""
Fit check: places Adafruit's own 3D models of the panel, Matrix Portal S3 and
STEMMA Speaker inside the case and verifies nothing collides.

    case/.venv/Scripts/python case/check_fit.py

1. Downloads the reference models into case/reference/ (not committed).
2. Builds the three printed parts in their assembled positions.
3. Intersects every printed part with every component solid and reports any
   overlap volume (a collision), plus overlaps between the printed parts.
4. Prints the key clearances and alignments.
Exit code 1 if anything collides.
"""
import io
import math
import os
import sys
import urllib.request
import zipfile

import gmsh

import pixelclock_case as pc

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "reference")

SOURCES = {
    "board.step": "https://raw.githubusercontent.com/adafruit/Adafruit_CAD_Parts/main/5778%20Matrix%20Portal%20S3/5778%20Matrix%20Portal%20S3.step",
    "speaker.step": "https://raw.githubusercontent.com/adafruit/Adafruit_CAD_Parts/main/3885%20STEMMA%20Speaker/3885%20STEMMA%20Speaker%20rev%20A.step",
    "cube.zip": "https://cdn-learn.adafruit.com/assets/assets/000/111/306/original/Cube_Source_Files.zip?1651610020",
}
TOL = 1e-3  # mm^3: anything larger is a real overlap


def fetch():
    os.makedirs(REF, exist_ok=True)
    for name, url in SOURCES.items():
        path = os.path.join(REF, name)
        if not os.path.exists(path):
            print(f"downloading {name} ...")
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0 (PixelClock fit check)"})
            with urllib.request.urlopen(req) as r:
                data = r.read()
            with open(path, "wb") as f:
                f.write(data)
    panel = os.path.join(REF, "Big Cube.step")
    if not os.path.exists(panel):
        with zipfile.ZipFile(os.path.join(REF, "cube.zip")) as z:
            z.extract("Big Cube.step", REF)
    return panel


def import_step(path):
    before = set(gmsh.model.occ.getEntities(3))
    gmsh.model.occ.importShapes(path)
    gmsh.model.occ.synchronize()
    return [d for d in gmsh.model.occ.getEntities(3) if d not in before]


def bbox(dt):
    return gmsh.model.occ.getBoundingBox(*dt)


def overlaps(a, b, eps=0.01):
    return all(a[i] - eps < b[i + 3] and b[i] - eps < a[i + 3] for i in range(3))


def overlap_volume(a, b):
    occ = gmsh.model.occ
    try:
        out, _ = occ.intersect([a], [b], removeObject=False, removeTool=False)
    except Exception as e:  # boolean failure: report, don't hide
        return float("nan"), str(e)
    vol = sum(occ.getMass(*d) for d in out if d[0] == 3)
    if out:
        occ.remove(out, recursive=True)
    return vol, ""


def main():
    panel_path = fetch()
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    occ = pc.init()
    gmsh.model.add("fit")

    parts = {}
    for name in pc.PARTS:
        parts[name] = pc.PARTS[name]()
        occ.synchronize()

    # ---- panel: Adafruit's model, volume "64x64 RGB Matrix Bevel v6:1" ----
    vols = import_step(panel_path)
    keep = None
    for d in vols:
        if gmsh.model.getEntityName(*d).endswith("64x64 RGB Matrix Bevel v6:1/64x64 RGB Matrix Bevel v6"):
            keep = d
    occ.remove([d for d in vols if d != keep], recursive=True)
    # model: LED face at z=-84 facing -z, back at z=-69 -> assembly: Y = z + 84, panel centred at (0, PANEL_CZ)
    occ.rotate([keep], 0, 0, 0, 1, 0, 0, math.pi / 2)
    occ.rotate([keep], 0, 0, 0, 0, 0, 1, math.pi)
    occ.translate([keep], 0, 84, pc.PANEL_CZ)
    occ.synchronize()
    pb = bbox(keep)
    print("panel model placed: X %.2f..%.2f  Y %.2f..%.2f  Z %.2f..%.2f" % (pb[0], pb[3], pb[1], pb[4], pb[2], pb[5]))
    components = {"panel": [keep]}

    # ---- Matrix Portal S3: board coords -> X = X0 + by, Y = EDGE - bx, Z = BOARD_Z + bz ----
    board = import_step(os.path.join(REF, "board.step"))
    occ.rotate(board, 0, 0, 0, 0, 0, 1, -math.pi / 2)
    occ.translate(board, pc.BOARD_X0, pc.BOARD_EDGE_Y, pc.BOARD_Z)
    components["board"] = board

    # ---- STEMMA Speaker: identity rotation ----
    spk = import_step(os.path.join(REF, "speaker.step"))
    occ.translate(spk, pc.SPK_X0, pc.SPK_Y0, pc.SPK_Z)
    components["speaker"] = spk
    occ.synchronize()

    problems = 0
    print("\n== collisions: printed parts vs Adafruit component models ==")
    for pname, ptags in parts.items():
        for p in ptags:
            pb = bbox(p)
            for cname, ctags in components.items():
                hits = []
                for c in ctags:
                    if not overlaps(pb, bbox(c)):
                        continue
                    v, err = overlap_volume(p, c)
                    if err or v > TOL or v != v:
                        hits.append((gmsh.model.getEntityName(*c)[-60:] or f"solid {c[1]}", v, err))
                if hits:
                    problems += len(hits)
                    for n, v, err in hits:
                        print(f"  COLLISION {pname} x {cname}: {n}  overlap {v:.4f} mm^3 {err}")
                else:
                    print(f"  ok  {pname:8s} vs {cname:8s} ({len(ctags)} solids)")

    print("\n== printed parts vs each other ==")
    names = list(parts)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            v = 0.0
            for a in parts[names[i]]:
                for b in parts[names[j]]:
                    if overlaps(bbox(a), bbox(b)):
                        vv, err = overlap_volume(a, b)
                        v += vv
            ok = v <= TOL
            problems += 0 if ok else 1
            print(f"  {'ok ' if ok else 'COLLISION'} {names[i]} vs {names[j]}: overlap {v:.4f} mm^3")

    print("\n== self-test: misplaced parts must be caught ==")

    def collides(part_names, comp_tags):
        for pn in part_names:
            for p in parts[pn]:
                for c in comp_tags:
                    if overlaps(bbox(p), bbox(c)):
                        v, _ = overlap_volume(p, c)
                        if v > TOL:
                            return True
        return False

    pcb = [d for d in components["board"] if gmsh.model.getEntityName(*d).endswith("Board:1/Board")]
    spk_pcb = [d for d in components["speaker"] if gmsh.model.getEntityName(*d).endswith("Board:1/Board")]
    tests = [
        ("board shifted 0.5 mm sideways (pins must hit the PCB)", pcb, ["foot"], (0.5, 0, 0)),
        ("board raised 0.3 mm (lid posts must hit it)", pcb, ["lid"], (0, 0, 0.3)),
        ("speaker shifted 0.5 mm (pins must hit its PCB)", spk_pcb, ["foot"], (0, 0.5, 0)),
        ("panel shifted 0.6 mm sideways (frame must hit it)", components["panel"], ["display"], (0.6, 0, 0)),
        ("panel pushed 0.3 mm back (screw bosses must hit it)", components["panel"], ["display"], (0, 0.3, 0)),
    ]
    for label, tags, pnames, (dx, dy, dz) in tests:
        occ.translate(tags, dx, dy, dz)
        caught = collides(pnames, tags)
        occ.translate(tags, -dx, -dy, -dz)
        problems += 0 if caught else 1
        print(f"  {'caught' if caught else 'MISSED'}: {label}")

    print("\n== alignments and clearances ==")
    # panel screws hit inserts
    for x, z in pc.PANEL_SCREWS:
        hit = (x, z) in [(a, b) for a, b in pc.PANEL_INSERTS]
        print(f"  panel screw at ({x:+.0f}, {z:+.0f}) from centre -> insert {'yes' if hit else 'NO'}")
        problems += 0 if hit else 1
    o = pc.PANEL_W / 2 + pc.GAP
    print(f"  panel front board 160.0 mm in a {2*o:.1f} mm opening: {pc.GAP:.2f} mm per side")
    print(f"  panel back face at Y={pc.PANEL_DEPTH}, screw bosses end at Y={pc.PANEL_DEPTH}: LED face flush with the front")
    print(f"  M3x6 screw: {pc.PANEL_SCREW_FLOOR:.1f} mm floor + {6 - pc.PANEL_SCREW_FLOOR:.1f} mm into the insert (insert hole 10 mm deep)")
    hdr_top = pc.BOARD_Z + pc.BOARD_TALLEST
    print(f"  board header top Z={hdr_top:.2f}, lid underside Z={pc.FOOT_TOP:.2f}: {pc.FOOT_TOP - hdr_top:.2f} mm for the ribbon plug")
    sock = pc.BOARD_Z - pc.BOARD_BOTTOM_PARTS
    print(f"  socket under the board bottoms at Z={sock:.2f}, floor at Z={pc.FLOOR_TOP:.2f}: {sock - pc.FLOOR_TOP:.2f} mm")
    edge_out = pc.FOOT_Y1
    print(f"  board edge {pc.BOARD_EDGE_INSET:.2f} mm inside the back face; buttons stick out {pc.BOARD_EDGE_PROTRUDE - pc.BOARD_EDGE_INSET:.2f} mm")
    print(f"  USB-C receptacle face is {pc.BOARD_EDGE_INSET - 0.03:.2f} mm inside the back face: any plug size fits")
    spk_top = pc.SPK_Z + pc.SPK_HEIGHT
    print(f"  speaker top Z={spk_top:.2f}, lid underside Z={pc.FOOT_TOP:.2f}: {pc.FOOT_TOP - spk_top:.2f} mm")
    print(f"  panel input connector centre at Z={pc.PANEL_CZ - pc.PANEL_IN_OFFSET:.1f}, cable window top Z={pc.WINDOW[3]:.1f}")

    # cable plugs aren't in Adafruit's models: check the space they need is empty
    print("\n== room for cable plugs (not in the component models) ==")
    occ = gmsh.model.occ
    sx, sy = pc.speaker_xyz(10.29, 0.27)[:2]
    sx1 = pc.speaker_xyz(20.19, 0.27)[0]
    zone_tests = [
        # speaker JST faces the front: JST PH plug + wire bend needs ~11 mm in front of it
        ("speaker cable plug", (sx - 1, sx1 + 1, sy - 11, sy, pc.SPK_Z + 1.57, pc.SPK_Z + 6.4), ["foot", "lid"]),
        # board JST (A0) faces -X: plug needs ~11 mm beside it
        ("board A0 cable plug", (pc.board_xyz(0, 0.18)[0] - 11, pc.board_xyz(0, 0.18)[0],
                                 pc.board_xyz(53.85, 0)[1], pc.board_xyz(43.94, 0)[1],
                                 pc.BOARD_Z + 1.57, pc.BOARD_Z + 6.4), ["foot", "lid"]),
        # ribbon plug on the board header (up to 6 mm above the header) and its bend
        ("ribbon plug on board header", (pc.board_xyz(0, 8.25)[0], pc.board_xyz(0, 36.2)[0],
                                         pc.board_xyz(61.9, 0)[1], pc.board_xyz(52.4, 0)[1],
                                         pc.BOARD_Z + pc.BOARD_TALLEST, pc.FOOT_TOP - 0.01), ["foot", "lid"]),
        # ribbon path from the header forward to the window
        ("ribbon path to window", (pc.WINDOW[0], pc.WINDOW[1], pc.FOOT_Y0, pc.board_xyz(52.4, 0)[1],
                                   pc.WINDOW[2], pc.WINDOW[3]), ["foot", "lid"]),
        # spade lugs and screws on the board's power terminals (~4 mm above the standoffs)
        ("power lugs on board", (pc.board_xyz(0, 1.7)[0], pc.board_xyz(0, 42.88)[0],
                                 pc.board_xyz(61.07, 0)[1], pc.board_xyz(55.58, 0)[1],
                                 pc.BOARD_Z + 4.57, pc.BOARD_Z + 8.6), ["foot", "lid"]),
    ]
    # Real hardware shapes: M3 nut = 5.5 mm across flats x 2.4 mm; M3 shank = 3.0 mm round;
    # pan head = 5.6 mm round x 2.4 mm. Each must sit in its place without touching the case.
    def nut_y(x, z, y0):  # nut on a screw along Y, flats top/bottom like the printed pocket
        return lambda: pc.hex_prism_y(x, z, y0, y0 + 2.4, 5.5)

    def nut_z(x, y, z0):  # nut in a lid-screw slot (axis Z), flats against the slot sides
        r = 5.5 / math.sqrt(3)
        def make():
            pts = [occ.addPoint(x + r * math.cos(math.radians(60 * i)), y + r * math.sin(math.radians(60 * i)), z0) for i in range(6)]
            lines = [occ.addLine(pts[i], pts[(i + 1) % 6]) for i in range(6)]
            s = occ.addPlaneSurface([occ.addCurveLoop(lines)])
            return [d for d in occ.extrude([(2, s)], 0, 0, 2.4) if d[0] == 3][0]
        return make

    for x, z in pc.JOINT_SCREWS:
        seat = pc.FOOT_Y0 + pc.FOOT_WALL + 3.5 - pc.NUT_T  # bottom of the pocket
        zone_tests.append((f"M3 nut, joint screw X={x:+.0f} Z={z:.0f}", nut_y(x, z, seat + 0.01), ["foot"]))
        zone_tests.append((f"M3x10 shank, joint screw X={x:+.0f} Z={z:.0f}",
                           (lambda x=x, z=z: pc.cyl_y(x, z, pc.CAVITY_BACK - 0.01, pc.CAVITY_BACK + 10, 3.0)), ["display", "foot"]))
        zone_tests.append((f"M3 pan head inside display, X={x:+.0f} Z={z:.0f}",
                           (lambda x=x, z=z: pc.cyl_y(x, z, pc.CAVITY_BACK - 2.4, pc.CAVITY_BACK - 0.01, 5.6)), ["display"]))
    for x, y in pc.LID_SCREWS:
        nz = pc.FOOT_TOP - 5.5
        zone_tests.append((f"M3 nut, lid screw X={x:+.1f} Y={y:.1f}", nut_z(x, y, nz - 1.2), ["foot"]))
        zone_tests.append((f"M3x10 shank, lid screw X={x:+.1f} Y={y:.1f}",
                           (lambda x=x, y=y: pc.cyl_z(x, y, pc.FOOT_TOP + pc.LID_T - 10, pc.FOOT_TOP + pc.LID_T + 0.01, 3.0)), ["foot", "lid"]))
    for x, z in pc.PANEL_SCREWS:
        zz = pc.PANEL_CZ + z
        zone_tests.append((f"M3x6 shank, panel screw ({x:+.0f}, {z:+.0f})",
                           (lambda x=x, zz=zz: pc.cyl_y(x, zz, pc.PANEL_DEPTH - 4.0, pc.PANEL_DEPTH + pc.PANEL_SCREW_FLOOR + 0.01, 3.0)), ["display"]))
        zone_tests.append((f"M3 pan head + driver, panel screw ({x:+.0f}, {z:+.0f})",
                           (lambda x=x, zz=zz: pc.cyl_y(x, zz, pc.PANEL_DEPTH + pc.PANEL_SCREW_FLOOR + 0.01, pc.CAVITY_BACK + pc.BACK_T + 5, 6.0)), ["display"]))

    for label, zone_def, pnames in zone_tests:
        if callable(zone_def):
            zone = zone_def()
        else:
            x0, x1, y0, y1, z0, z1 = zone_def
            zone = (3, occ.addBox(min(x0, x1), min(y0, y1), z0, abs(x1 - x0), abs(y1 - y0), z1 - z0))
        hit = 0.0
        for pn in pnames:
            for p in parts[pn]:
                if overlaps(bbox(p), bbox(zone)):
                    v, _ = overlap_volume(p, zone)
                    hit += v
        occ.remove([zone], recursive=True)
        ok = hit <= TOL
        problems += 0 if ok else 1
        print(f"  {'ok ' if ok else 'BLOCKED'} {label}: {hit:.3f} mm^3 of case in the way")

    gmsh.finalize()
    print("\nRESULT:", "PASS - nothing collides" if problems == 0 else f"FAIL - {problems} problem(s)")
    return 0 if problems == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
