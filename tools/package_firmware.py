import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zipfile


FLASH_SIZE = 0x800000
EXPECTED_PARTITIONS = {
    "nvs": (1, 2, 0x9000, 0x5000),
    "otadata": (1, 0, 0xE000, 0x2000),
    "app0": (0, 0x10, 0x10000, 0x330000),
    "app1": (0, 0x11, 0x340000, 0x330000),
    "spiffs": (1, 0x82, 0x670000, 0x180000),
    "coredump": (1, 3, 0x7F0000, 0x10000),
}


def validate_partitions(data):
    partitions = {}
    for offset in range(0, len(data) - 31, 32):
        entry = data[offset:offset + 32]
        magic = struct.unpack_from("<H", entry)[0]
        if magic == 0xEBEB:
            if entry[16:] != hashlib.md5(data[:offset]).digest():
                raise ValueError("Partition table checksum mismatch")
            if partitions != EXPECTED_PARTITIONS:
                raise ValueError("Expected the pinned default_8MB partition layout")
            return
        if magic != 0x50AA:
            raise ValueError("Invalid partition table or missing checksum")
        _, kind, subtype, start, size, label, flags = struct.unpack("<HBBII16sI", entry)
        name = label.split(b"\0", 1)[0].decode("ascii")
        if name in partitions or flags:
            raise ValueError("Duplicate or encrypted partition")
        partitions[name] = (kind, subtype, start, size)
    raise ValueError("Missing partition table checksum")


def validate_segments(segments):
    limits = {0: 0x8000, 0x8000: 0x9000, 0xE000: 0x10000, 0x10000: 0x340000}
    if set(segments) != set(limits):
        raise ValueError("Incorrect firmware segment offsets")
    for offset, data in segments.items():
        if not data or offset + len(data) > limits[offset]:
            raise ValueError(f"Empty or oversized segment at {offset:#x}")
    validate_partitions(segments[0x8000])


def verify_merged(merged, segments):
    if not merged or len(merged) > FLASH_SIZE:
        raise ValueError("Invalid merged image size")
    for offset, data in segments.items():
        if merged[offset:offset + len(data)] != data:
            raise ValueError(f"Merged image differs at {offset:#x}")
    if merged[0x9000:0xE000] != b"\xff" * 0x5000:
        raise ValueError("Factory image contains nonempty settings")


def file_record(path, offset):
    data = path.read_bytes()
    return {"file": path.name, "offset": hex(offset), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest()}


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description="Package the universal Cardputer build without reading a device")
    parser.add_argument("--build-dir", type=Path, default=root / ".pio/build/cardputer-universal")
    parser.add_argument("--core-dir", type=Path, default=root / ".pio")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"[0-9]+\.[0-9]+(?:\.[0-9]+)?", args.version):
        parser.error("Version must be numeric, for example 1.0")
    app_info = (root / "src/common/AppInfo.h").read_text()
    if f'AppVersion[] = "{args.version}"' not in app_info:
        parser.error("Package version must match src/common/AppInfo.h")
    commit = subprocess.run(["git", "-C", str(root), "rev-parse", "HEAD"], check=True,
                            capture_output=True, text=True).stdout.strip()
    subprocess.run(["git", "-C", str(root), "diff", "--quiet", "HEAD", "--"], check=True)
    paths = {
        0: args.build_dir / "bootloader.bin",
        0x8000: args.build_dir / "partitions.bin",
        0xE000: args.core_dir / "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin",
        0x10000: args.build_dir / "firmware.bin",
    }
    segments = {offset: path.read_bytes() for offset, path in paths.items()}
    validate_segments(segments)
    esptool = args.core_dir / "packages/tool-esptoolpy/esptool.py"
    if not esptool.is_file():
        raise FileNotFoundError(esptool)
    args.output.mkdir(parents=True, exist_ok=False)
    prefix = f"M5Chord-v{args.version}"
    factory = args.output / f"{prefix}-universal.bin"
    command = [sys.executable, str(esptool), "--chip", "esp32s3", "merge_bin", "-o", str(factory)]
    for offset, path in paths.items():
        command.extend([hex(offset), str(path)])
    subprocess.run(command, check=True)
    verify_merged(factory.read_bytes(), segments)
    application = args.output / f"{prefix}-app.bin"
    shutil.copyfile(paths[0x10000], application)
    shutil.copyfile(root / "platformio.ini", args.output / "platformio.ini")
    shutil.copyfile(root / "LICENSE", args.output / "LICENSE")
    shutil.copyfile(root / "THIRD_PARTY_NOTICES.md", args.output / "THIRD_PARTY_NOTICES.md")
    shutil.copytree(root / "licenses", args.output / "licenses")
    shutil.copyfile(root / "README.md", args.output / "README.md")
    shutil.copytree(root / "docs", args.output / "docs")
    update_segments = []
    for offset, path in paths.items():
        target = application if offset == 0x10000 else args.output / path.name
        if offset != 0x10000:
            shutil.copyfile(path, target)
        update_segments.append(file_record(target, offset))
    manifest = {
        "schema": 1,
        "name": "M5Chord",
        "version": args.version,
        "source_commit": commit,
        "source_url": f"https://github.com/jrdntnnr/M5Chord/tree/{commit}",
        "chip": "esp32s3",
        "flash_bytes": FLASH_SIZE,
        "models": ["Cardputer 1.0", "Cardputer 1.1", "Cardputer ADV"],
        "hardware_acceptance": "Pre-rename 1.0-device compatibility reported working; release-specific full acceptance pending; see docs/SMK37_TEST_RESULTS.md",
        "known_issues": ["SMK-37 BLE can connect without key input. Switch keyboard off/on twice; fix planned for a coming version."],
        "factory": {**file_record(factory, 0), "resets_nvs": True},
        "application": {**file_record(application, 0x10000), "requires_matching_partition_layout": True},
        "source_segments": [file_record(path, offset) for offset, path in paths.items()],
        "settings_preserving_update": update_segments,
        "m5burner_publication": "Not published; this manifest is project metadata, not an M5Burner schema",
    }
    (args.output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    records = [manifest["factory"], *update_segments]
    (args.output / "SHA256SUMS").write_text("".join(f"{item['sha256']}  {item['file']}\n" for item in records))
    (args.output / "INSTALL.md").write_text(
        f"# M5Chord {args.version}\n\n"
        "One binary for Cardputer 1.0, 1.1 and ADV. See README.md for full instructions.\n\n"
        "Known SMK-37 Bluetooth bug: the keyboard may appear connected without key input. "
        "Switch the SMK-37 off and back on twice while M5Chord stays running. "
        "A fix is planned for a coming version; it is not included here.\n\n"
        "## Fresh install / M5Burner upload candidate\n\n"
        f"Write {factory.name} at 0x0000 on an ESP32-S3 with 8 MB flash. "
        "This resets saved NVS settings and the active OTA selection. Back up settings first. "
        "It does not erase the SD card and is not a whole-chip wipe. "
        "The image is merged from build outputs, not dumped from a user's device.\n\n"
        "## Application update\n\n"
        f"Write {application.name} at 0x10000 only on this project's matching "
        "default_8MB partition layout with app0 selected. NVS is not overwritten. "
        "For source-based updates, prefer pio run -e cardputer-universal -t upload.\n\n"
        "For a settings-preserving binary update, use all four separate components "
        "with the exact command in README.md. This selects app0 without writing NVS.\n\n"
        "## First boot\n\n"
        "Release G0 and power-cycle. Geek view (V) should show 1.0/1.1 or ADV. "
        "Test Tab Options and Q/W/E/R plus A/S/D/F press/release. "
        "On an empty settings store the initial mode is BYPASS. "
        "The controller produces no audio; use Unit MIDI in SEPARATE mode for DIN output. "
        "SMK-37 can use BLE MIDI. External USB hubs remain unsupported.\n\n"
        "M5Burner account publication and catalog validation have not been performed. "
        "manifest.json is release metadata, not an M5Burner import manifest. "
        "See docs/COMPATIBILITY.md and docs/SMK37_TEST_RESULTS.md. "
        "The release contains no private device settings.\n"
    )
    archive = args.output / f"{prefix}.zip"
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED) as bundle:
        for path in sorted(args.output.rglob("*")):
            if path.is_file() and path != archive:
                bundle.write(path, path.relative_to(args.output))
    with zipfile.ZipFile(archive) as bundle:
        if bundle.testzip() is not None:
            raise ValueError("Release archive CRC check failed")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
