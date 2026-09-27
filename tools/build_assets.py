"""Genera todo el arte, mapas y audio del juego en game/assets.

Uso:  python3 tools/build_assets.py [--preview DIR]
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
OUT = os.path.join(HERE, "..", "game", "assets")

import chars  # noqa: E402
import props  # noqa: E402
import props2  # noqa: E402
import maps  # noqa: E402
import ui  # noqa: E402
import audio  # noqa: E402


def preview(out_dir):
    from PIL import Image
    os.makedirs(out_dir, exist_ok=True)
    for f in os.listdir(os.path.join(OUT, "maps")):
        if not f.endswith(".json"):
            continue
        data = json.load(open(os.path.join(OUT, "maps", f)))
        im = Image.open(os.path.join(OUT, "maps", data["id"] + ".png")).convert("RGBA")
        for p in sorted(data["props"], key=lambda p: p["y"]):
            spr = Image.open(os.path.join(OUT, "sprites", p["sprite"] + ".png"))
            fr = p.get("frames", 1)
            fw = spr.width // fr
            spr = spr.crop((0, 0, fw, spr.height))
            if p.get("flip"):
                spr = spr.transpose(Image.FLIP_LEFT_RIGHT)
            im.alpha_composite(spr, (int(p["x"] - fw // 2), int(p["y"] - spr.height - p.get("lift", 0))))
        im.save(os.path.join(out_dir, "map_" + data["id"] + ".png"))


if __name__ == "__main__":
    chars.build_all(OUT)
    props.build_all(OUT)
    props2.build_all(OUT)
    maps.build_all(OUT)
    ui.build_all(OUT)
    ui.build_v2(OUT)
    audio.build_all(OUT)
    audio.build_v2(OUT)
    if "--preview" in sys.argv:
        preview(sys.argv[sys.argv.index("--preview") + 1])
    print("assets ok")
