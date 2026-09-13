import argparse
from pathlib import Path
from PIL import Image, ImageChops, ImageDraw

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
args = parser.parse_args()
files = sorted(args.directory.glob("*.ppm"))
for file in files:
    with Image.open(file) as source:
        source.save(file.with_suffix(".png"))
for group, subset in [("performance", [p for p in files if not p.name.startswith(("menu-", "seq-"))]),
                      ("menus", [p for p in files if p.name.startswith("menu-")])]:
    if not subset:
        continue
    sheet = Image.new("RGB", (744, ((len(subset) + 2) // 3) * 158), "#101820")
    draw = ImageDraw.Draw(sheet)
    for index, file in enumerate(subset):
        x, y = (index % 3) * 248, (index // 3) * 158
        draw.text((x + 4, y + 3), file.stem, fill="#a8b8c8")
        with Image.open(file) as source:
            sheet.paste(source, (x + 4, y + 19))
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.Resampling.NEAREST).save(args.directory / f"{group}.png")
for name in ["block", "strum", "strum2", "slop", "arp", "arp2", "pattern", "harp"]:
    sequence = sorted(args.directory.glob(f"seq-{name}-*.png"))
    if not sequence:
        continue
    frames = []
    reference = None
    for file in sequence:
        with Image.open(file) as source:
            if reference is None:
                reference = source.copy()
            for bounds in [(8, 3, 117, 20), (8, 82, 232, 114)]:
                if ImageChops.difference(reference.crop(bounds), source.crop(bounds)).getbbox():
                    raise RuntimeError(f"Unstable static UI region in {file.name}: {bounds}")
            frames.append(source.resize((720, 405), Image.Resampling.NEAREST))
    frames[0].save(args.directory / f"live-{name}.gif", save_all=True, append_images=frames[1:], duration=20, loop=0, disposal=2)
    indices = [0, 3, 6, 9, 13, 19, 25, 38, 49]
    sheet = Image.new("RGB", (744, 474), "black")
    draw = ImageDraw.Draw(sheet)
    for index, sample in enumerate(indices):
        if sample >= len(sequence):
            continue
        x, y = index % 3 * 248, index // 3 * 158
        draw.text((x + 4, y + 3), f"{name.upper()} / {sample * 20} ms", fill="#b6ffaa")
        with Image.open(sequence[sample]) as source:
            sheet.paste(source, (x + 4, y + 19))
    sheet.resize((1488, 948), Image.Resampling.NEAREST).save(args.directory / f"timeline-{name}.png")
