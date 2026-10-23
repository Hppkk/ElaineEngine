"""Verify migrated payloads and identities against a migration backup (read-only)."""
import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path


def read_asset(path):
    with path.open("rb") as stream:
        if stream.read(3) != b"ELA":
            stream.seek(0)
            document = json.load(stream)
            return document["resource"], {"data": json.dumps(document["data"]).encode("utf-8")}
        stream.seek(0)
        header = stream.read(48)
        if len(header) != 48:
            raise ValueError(f"Truncated asset: {path}")
        magic, version, size, metadata_size, offset, length, reserved = struct.unpack("<8sIIQQQQ", header)
        if (magic, version, size, reserved) != (b"ELASSET\0", 1, 48, 0):
            raise ValueError(f"Invalid asset header: {path}")
        if offset != 48 + metadata_size or offset + length != path.stat().st_size:
            raise ValueError(f"Invalid asset boundaries: {path}")
        metadata = json.loads(stream.read(metadata_size))
        sections = {}
        for name, section in metadata["sections"].items():
            start, count = section["offset"], section["size"]
            if start < 0 or count < 0 or start + count > length:
                raise ValueError(f"Invalid section {name}: {path}")
            stream.seek(offset + start)
            sections[name] = stream.read(count)
        return metadata["resource"], sections


def audit(root, backup):
    manifest = json.loads((backup / "manifest.json").read_text(encoding="utf-8-sig"))
    plan = {entry["source"]: entry for entry in manifest["assets"]}
    expected = {entry["destination"] for entry in plan.values()}
    actual = {path.relative_to(root).as_posix() for path in root.rglob("*")
              if path.is_file() and path.suffix in (".asset", ".shader")}
    if expected != actual:
        raise ValueError(f"Inventory mismatch: missing={expected - actual}, extra={actual - expected}")

    def convert(value):
        if isinstance(value, str):
            entry = plan.get(value, plan.get("shader/vulkan/" + value))
            return {"id": entry["id"], "path": entry["destination"]} if entry else value
        if isinstance(value, list):
            return [convert(item) for item in value]
        if isinstance(value, dict):
            return {key: convert(item) for key, item in value.items()}
        return value

    result = {"assets": [], "embeddedSources": []}
    identities, references = set(), set()
    for source, entry in plan.items():
        target = root / entry["destination"]
        if entry["type"] == "Shader":
            shader = json.loads(target.read_text(encoding="utf-8-sig"))
            descriptor = shader["resource"]
            if shader["source"] != source or not (root / source).is_file():
                raise ValueError(f"Shader source changed: {source}")
        else:
            descriptor, sections = read_asset(target)
            original = backup / "originals" / source
            if (root / source).exists():
                raise ValueError(f"Legacy asset remains: {source}")
            if entry["type"] == "Mesh":
                if sections["data"] != original.read_bytes():
                    raise ValueError(f"Mesh payload changed: {source}")
            elif entry["type"] == "Texture":
                texture = json.loads(original.read_text(encoding="utf-8-sig").rstrip("\0"))
                images = texture["source"]
                dimension = texture["dimension"]
                paths = ([images["path"]] if dimension == "2d" else
                         [images[face] for face in ("px", "nx", "py", "ny", "pz", "nz")]
                         if dimension == "cube" else images["paths"])
                for index, name in enumerate(paths):
                    original_image = (backup / "originals" / name).read_bytes()
                    if sections[f"source{index}"] != original_image:
                        raise ValueError(f"Embedded original changed: {name}")
                    if (root / name).exists():
                        raise ValueError(f"External image remains: {name}")
                    result["embeddedSources"].append({"asset": entry["destination"], "source": name,
                        "bytes": len(original_image), "sha256": hashlib.sha256(original_image).hexdigest()})
                layout = json.loads(sections["texture"])
                if layout["format"] == "RGBA32F":
                    maximum = max(value[0] for value in struct.iter_unpack("<f", sections["data"]))
                    result.setdefault("hdr", []).append({"asset": entry["destination"], "maximum": maximum})
            else:
                old_json = json.loads(original.read_text(encoding="utf-8-sig").rstrip("\0"))
                if json.loads(sections["data"]) != convert(old_json):
                    raise ValueError(f"Serialized content changed: {source}")
                data = json.loads(sections["data"])
                expected_dependencies = {}

                def collect(node, kind="hard"):
                    if isinstance(node, dict):
                        if "id" in node and "path" in node:
                            old_kind = expected_dependencies.get(node["id"])
                            expected_dependencies[node["id"]] = "hard" if old_kind == "hard" else kind
                        else:
                            for value in node.values():
                                collect(value, kind)
                    elif isinstance(node, list):
                        for value in node:
                            collect(value, kind)

                for key, value in data.items():
                    if entry["type"] == "World" and key == "Chunks":
                        for chunk in value:
                            for field, child in chunk.items():
                                collect(child, "soft" if field in ("Actors", "Bundle") else "hard")
                    else:
                        collect(value)
                actual_dependencies = {ref["id"]: ref["kind"] for ref in descriptor["dependencies"]}
                if len(actual_dependencies) != len(descriptor["dependencies"]) or actual_dependencies != expected_dependencies:
                    raise ValueError(f"Dependency summary or classification mismatch: {source}")
                document = json.loads(target.read_text(encoding="utf-8-sig"))
                if set(document) != {"resource", "data"} or not target.read_bytes().startswith(b"{\n"):
                    raise ValueError(f"Business asset is not readable text: {source}")
                result.setdefault("readableBusinessAssets", []).append(entry["destination"])
        if descriptor["id"] != entry["id"] or descriptor["type"] != entry["type"]:
            raise ValueError(f"Identity/type changed: {source}")
        if descriptor["id"] in identities:
            raise ValueError(f"Duplicate ID: {descriptor['id']}")
        identities.add(descriptor["id"])
        references.update(reference["id"] for reference in descriptor["dependencies"])
        result["assets"].append({"path": entry["destination"], "id": descriptor["id"], "type": descriptor["type"]})
    if references - identities:
        raise ValueError(f"Missing references: {references - identities}")
    result["counts"] = dict(Counter(asset["type"] for asset in result["assets"]))
    result["total"] = len(identities)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("contents", type=Path)
    parser.add_argument("backup", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = audit(args.contents.resolve(), args.backup.resolve())
    text = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(text, encoding="utf-8")
    print(f"Verified {report['total']} assets, all IDs/references, JSON/Mesh payloads and embedded originals")
