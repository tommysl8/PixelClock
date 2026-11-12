#!/usr/bin/env python3
"""
Renders preview images of the case with Adafruit's component models inside.

    case/.venv/Scripts/python case/render_previews.py

Needs case/reference/ (run check_fit.py once first). Writes case/images/*.png.
"""
import json
import math
import os
import subprocess
import sys

import gmsh
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.collections import PolyCollection  # noqa: E402

import pixelclock_case as pc  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, "reference")
OUT = os.path.join(HERE, "images")

CASE_COLOR = (0.17, 0.17, 0.19)
LID_COLOR = (0.21, 0.21, 0.23)
BOARD_COLOR = (0.10, 0.10, 0.10)
METAL = (0.70, 0.70, 0.72)
SPEAKER_COLOR = (0.15, 0.15, 0.16)
PANEL_COLOR = (0.08, 0.08, 0.08)
ACRYLIC_COLOR = (0.05, 0.05, 0.06)


def mesh_tris(tags, size=6.0, curv=10):
    """Mesh the given volumes' surfaces and return an (n, 3, 3) triangle array."""
    gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", curv)
    gmsh.option.setNumber("Mesh.MeshSizeMax", size)
    gmsh.option.setNumber("Mesh.MeshSizeMin", 0.3)
    gmsh.model.occ.synchronize()
    gmsh.model.mesh.clear()
    surfs = gmsh.model.getBoundary(tags, oriented=False, combined=False)
    gmsh.model.mesh.generate(2)
    nodes, coords, _ = gmsh.model.mesh.getNodes()
    lookup = dict(zip(nodes.tolist(), coords.reshape(-1, 3)))
    tris = []
    for _, s in surfs:
        types, _, enodes = gmsh.model.mesh.getElements(2, abs(s))
        for t, en in zip(types, enodes):
            if t == 2:
                en = en.reshape(-1, 3)
                tris.append(np.array([[lookup[a], lookup[b], lookup[c]] for a, b, c in en]))
    return np.concatenate(tris) if tris else np.zeros((0, 3, 3))


def import_step(path):
    before = set(gmsh.model.occ.getEntities(3))
    gmsh.model.occ.importShapes(path)
    gmsh.model.occ.synchronize()
    return [d for d in gmsh.model.occ.getEntities(3) if d not in before]


def clock_frame():
    """64x64 frame from the simulator (christmas, 9 pm) for the LED face."""
    js = (
        "require('./tools/render.js');const PC=globalThis.PixelClock;"
        "const t=PC.themeById('christmas');const r=new PC.Rng(7);const i=t.create(r);"
        "const fb=new PC.Canvas();for(let k=0;k<90;k++)i.update(1/30,{hours:21,minutes:41,seconds:0,ms:0,hourFloat:21.7,hour12:true,year:2026,month:12,day:20,shake:false});"
        "i.draw(fb,{hours:21,minutes:41,seconds:0,ms:0,hourFloat:21.7,hour12:true,year:2026,month:12,day:20});"
        "console.log(JSON.stringify(Array.from(fb.px)))"
    )
    root = os.path.dirname(HERE)
    try:
        out = subprocess.run(["node", "-e", js], cwd=root, capture_output=True, text=True, check=True).stdout
        return np.array(json.loads(out), dtype=np.uint32).reshape(64, 64)
    except Exception:
        return None


def box_tris(x0, x1, y0, y1, z0, z1):
    """Triangles for an axis-aligned box (used for the acrylic sheet)."""
    p = np.array([[x, y, z] for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)], float)
    faces = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    tris = []
    for a, b, c, d in faces:
        tris += [[p[a], p[b], p[c]], [p[a], p[c], p[d]]]
    return np.array(tris)


def led_quads(px):
    """Small squares on the LED face (Y = -0.05), one per pixel."""
    quads, cols = [], []
    if px is None:
        return np.zeros((0, 4, 3)), []
    pitch = pc.PANEL_W / 64
    s = pitch * 0.42
    for r in range(64):
        for c in range(64):
            v = int(px[r, c])
            rgb = ((v >> 16) & 255) / 255, ((v >> 8) & 255) / 255, (v & 255) / 255
            if sum(rgb) < 0.08:
                continue
            x = -pc.PANEL_W / 2 + (c + 0.5) * pitch
            z = pc.PANEL_CZ + pc.PANEL_W / 2 - (r + 0.5) * pitch
            quads.append([[x - s, -0.05, z - s], [x + s, -0.05, z - s], [x + s, -0.05, z + s], [x - s, -0.05, z + s]])
            cols.append(rgb)
    return np.array(quads), cols


def render(scene, cam_dir, fname, title=None, size=(9, 7), ortho_up=(0, 0, 1)):
    """Painter's-algorithm render. scene: list of (polys (n,k,3), color or list, emissive)."""
    d = np.array(cam_dir, float)
    d /= np.linalg.norm(d)
    up = np.array(ortho_up, float)
    right = np.cross(d, up)
    right /= np.linalg.norm(right)
    up = np.cross(right, d)
    light = -d + 0.6 * up - 0.3 * right
    light /= np.linalg.norm(light)

    all_polys, all_cols, all_depth = [], [], []
    for polys, color, emissive in scene:
        if len(polys) == 0:
            continue
        p = np.asarray(polys, float)
        e1, e2 = p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]
        n = np.cross(e1, e2)
        nl = np.linalg.norm(n, axis=1)
        good = nl > 1e-12
        p, n = p[good], n[good] / nl[good][:, None]
        cols = np.array(color)[good] if isinstance(color, list) else np.tile(color, (len(p), 1))
        facing = n @ d
        n = np.where(facing[:, None] > 0, -n, n)  # two-sided
        if emissive:
            shade = np.ones(len(p))
        else:
            shade = 0.35 + 0.65 * np.clip(n @ light, 0, 1)
        c = np.clip(cols * shade[:, None], 0, 1)
        proj = np.stack([p @ right, p @ up], axis=-1)
        depth = (p @ d).mean(axis=1)
        all_polys.append(proj)
        all_cols.append(c)
        all_depth.append(depth)
    maxk = max(pp.shape[1] for pp in all_polys)
    polys = []
    for pp in all_polys:
        if pp.shape[1] < maxk:
            pp = np.concatenate([pp, np.repeat(pp[:, -1:, :], maxk - pp.shape[1], axis=1)], axis=1)
        polys.append(pp)
    polys = np.concatenate(polys)
    cols = np.concatenate(all_cols)
    depth = np.concatenate(all_depth)
    order = np.argsort(-depth)
    fig, ax = plt.subplots(figsize=size, dpi=110)
    fig.patch.set_facecolor("#f4f2ee")
    ax.set_facecolor("#f4f2ee")
    pcoll = PolyCollection(polys[order], facecolors=cols[order], edgecolors=cols[order], linewidths=0.15)
    ax.add_collection(pcoll)
    lo, hi = polys.reshape(-1, 2).min(0), polys.reshape(-1, 2).max(0)
    pad = 0.06 * (hi - lo).max()
    ax.set_xlim(lo[0] - pad, hi[0] + pad)
    ax.set_ylim(lo[1] - pad, hi[1] + pad)
    ax.set_aspect("equal")
    ax.axis("off")
    if title:
        ax.set_title(title, fontsize=13, color="#333")
    fig.tight_layout()
    fig.savefig(os.path.join(OUT, fname), facecolor=fig.get_facecolor())
    plt.close(fig)
    print("wrote", fname)


def main():
    os.makedirs(OUT, exist_ok=True)
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    occ = pc.init()

    tris = {}
    for name in pc.PARTS:
        gmsh.clear()
        gmsh.model.add(name)
        tags = pc.build(name)
        tris[name] = mesh_tris(tags, size=5.0, curv=12)

    gmsh.clear()
    gmsh.model.add("board")
    board = import_step(os.path.join(REF, "board.step"))
    occ.rotate(board, 0, 0, 0, 0, 0, 1, -math.pi / 2)
    occ.translate(board, pc.BOARD_X0, pc.BOARD_EDGE_Y, pc.BOARD_Z)
    tris["board"] = mesh_tris(board, size=3.0, curv=8)

    gmsh.clear()
    gmsh.model.add("speaker")
    spk = import_step(os.path.join(REF, "speaker.step"))
    occ.translate(spk, pc.SPK_X0, pc.SPK_Y0, pc.SPK_Z)
    tris["speaker"] = mesh_tris(spk, size=3.0, curv=8)

    gmsh.clear()
    gmsh.model.add("panel")
    vols = import_step(os.path.join(REF, "Big Cube.step"))
    keep = [d for d in vols if gmsh.model.getEntityName(*d).endswith("64x64 RGB Matrix Bevel v6:1/64x64 RGB Matrix Bevel v6")]
    occ.remove([d for d in vols if d not in keep], recursive=True)
    occ.rotate(keep, 0, 0, 0, 1, 0, 0, math.pi / 2)
    occ.rotate(keep, 0, 0, 0, 0, 0, 1, math.pi)
    occ.translate(keep, 0, 84 + pc.PANEL_FRONT_Y, pc.PANEL_CZ)
    tris["panel"] = mesh_tris(keep, size=6.0, curv=8)
    gmsh.finalize()

    leds, led_cols = led_quads(clock_frame())
    half = pc.PANEL_W / 2
    acrylic = box_tris(-half, half, 0, pc.ACRYLIC_T, pc.PANEL_CZ - half, pc.PANEL_CZ + half) if pc.ACRYLIC else np.zeros((0, 3, 3))

    def shifted(t, dy=0.0, dz=0.0):
        t = t.copy()
        t[..., 1] += dy
        t[..., 2] += dz
        return t

    base = [
        (tris["display"], CASE_COLOR, False),
        (tris["foot"], CASE_COLOR, False),
        (tris["lid"], LID_COLOR, False),
        (tris["panel"], PANEL_COLOR, False),
        (acrylic, ACRYLIC_COLOR, False),
        (tris["board"], BOARD_COLOR, False),
        (tris["speaker"], SPEAKER_COLOR, False),
        (leds, led_cols, True),
    ]
    render(base, (0.55, 1.0, -0.38), "front.png", "PixelClock case: front")
    render(base, (-0.75, -1.0, -0.45), "back.png", "Back: USB-C, buttons, speaker grille")
    render([(tris["display"], CASE_COLOR, False), (tris["foot"], CASE_COLOR, False), (tris["lid"], LID_COLOR, False),
            (tris["panel"], PANEL_COLOR, False), (leds, led_cols, True)],
           (1.0, 0.0001, 0.0), "side.png", "Side view", size=(9, 5))
    exploded = [
        (tris["display"], CASE_COLOR, False),
        (shifted(acrylic, dy=-40), ACRYLIC_COLOR, False),
        (shifted(tris["panel"], dy=-75), PANEL_COLOR, False),
        (shifted(leds, dy=-75 + pc.PANEL_FRONT_Y), led_cols, True),
        (shifted(tris["foot"], dy=45), CASE_COLOR, False),
        (shifted(tris["board"], dy=45, dz=35), BOARD_COLOR, False),
        (shifted(tris["speaker"], dy=45, dz=35), SPEAKER_COLOR, False),
        (shifted(tris["lid"], dy=45, dz=80), LID_COLOR, False),
    ]
    render(exploded, (-0.9, -1.0, -0.55), "exploded.png", "Exploded view", size=(10, 8))
    inside = [
        (tris["foot"], CASE_COLOR, False),
        (tris["board"], BOARD_COLOR, False),
        (tris["speaker"], SPEAKER_COLOR, False),
    ]
    render(inside, (0.0001, 0.0001, -1.0), "foot_inside.png", "Inside the foot (lid off), from above",
           size=(8, 6), ortho_up=(0, -1, 0))


if __name__ == "__main__":
    sys.exit(main())
