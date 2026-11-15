"""Fetch just the UE 5.6 Mark provider from NVIDIA's multi-version 8 GB ZIP.

Uses HTTP byte ranges and validates the extracted member's ZIP CRC and length.
No credentials, cloud inference, or account tokens are needed by this CDN URL.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import io
import json
from pathlib import Path
import struct
import shutil
import urllib.request
import zipfile
import zlib

URL = "https://developer.download.nvidia.com/assets/ace/ACE_2.5_v2.3_A2F_Models.zip"
MEMBER = "ACE_2.3_A2F_Models/NvAudio2FaceMarkRegressive-UE5.6-v2.4.0.zip"


def request_range(start, end):
    request = urllib.request.Request(URL, headers={"Range": f"bytes={start}-{end}", "Accept-Encoding": "identity"})
    response = urllib.request.urlopen(request, timeout=60)
    if response.status != 206 or not response.headers.get("Content-Range", "").startswith(f"bytes {start}-"):
        response.close()
        raise RuntimeError("CDN did not honor the requested range")
    return response


class RemoteIndex(io.RawIOBase):
    def __init__(self, size):
        self.size, self.pos = size, 0

    def seekable(self):
        return True

    def seek(self, offset, whence=0):
        self.pos = offset if whence == 0 else self.pos + offset if whence == 1 else self.size + offset
        return self.pos

    def tell(self):
        return self.pos

    def read(self, length=-1):
        length = self.size - self.pos if length < 0 else min(length, self.size - self.pos)
        if length <= 0:
            return b""
        if length > 5_000_000:
            raise RuntimeError("Unexpectedly large ZIP index read")
        with request_range(self.pos, self.pos + length - 1) as response:
            data = response.read(length)
        self.pos += len(data)
        return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(urllib.request.Request(URL, method="HEAD"), timeout=30) as response:
        total = int(response.headers["Content-Length"])
    with zipfile.ZipFile(RemoteIndex(total)) as archive:
        info = archive.getinfo(MEMBER)
    with request_range(info.header_offset, info.header_offset + 29) as response:
        header = response.read()
    if header[:4] != b"PK\x03\x04" or info.compress_type != zipfile.ZIP_DEFLATED:
        raise RuntimeError("Unexpected ZIP member format")
    name_len, extra_len = struct.unpack_from("<HH", header, 26)
    start = info.header_offset + 30 + name_len + extra_len
    target = args.output / Path(MEMBER).name
    compressed = target.with_suffix(".deflate.part")
    completed = compressed.stat().st_size if compressed.exists() else 0
    if completed > info.compress_size:
        raise RuntimeError("Existing partial file is too large")
    # The CDN throttles long transfers; independent, resumable 64 MiB ranges avoid
    # restarting the whole archive when a connection stalls.
    chunk_dir = args.output / "mark-range-cache"
    chunk_dir.mkdir(exist_ok=True)
    ranges = [(offset, min(64 * 1024 * 1024, info.compress_size - offset))
              for offset in range(completed, info.compress_size, 64 * 1024 * 1024)]

    def fetch_chunk(item):
        offset, length = item
        path = chunk_dir / f"{offset}_{length}.part"
        have = path.stat().st_size if path.exists() else 0
        if have > length:
            raise RuntimeError(f"Oversized range cache: {path}")
        for attempt in range(3):
            if have == length:
                return path
            try:
                with request_range(start + offset + have, start + offset + length - 1) as response, path.open("ab") as stream:
                    while have < length:
                        block = response.read(min(4 * 1024 * 1024, length - have))
                        if not block:
                            raise RuntimeError("Download ended early")
                        stream.write(block)
                        have += len(block)
            except Exception as exc:
                print(f"Range {offset}: attempt {attempt + 1}: {exc}", flush=True)
        if have != length:
            raise RuntimeError("Model transfer incomplete; rerun to resume saved ranges")
        return path

    print(f"Retained {completed / 1e6:.0f} MB; downloading {len(ranges)} ranges", flush=True)
    with ThreadPoolExecutor(max_workers=6) as pool:
        paths = list(pool.map(fetch_chunk, ranges))
    with compressed.open("ab") as stream:
        for path in paths:
            with path.open("rb") as source:
                shutil.copyfileobj(source, stream, 4 * 1024 * 1024)
    if compressed.stat().st_size != info.compress_size:
        raise RuntimeError("Combined range length mismatch")
    decoder, crc, size = zlib.decompressobj(-15), 0, 0
    unpacked = target.with_suffix(".zip.part")
    with compressed.open("rb") as source, unpacked.open("wb") as output:
        while block := source.read(4 * 1024 * 1024):
            data = decoder.decompress(block)
            output.write(data)
            crc = zlib.crc32(data, crc)
            size += len(data)
        data = decoder.flush()
        output.write(data)
        crc = zlib.crc32(data, crc)
        size += len(data)
    if not decoder.eof or size != info.file_size or crc != info.CRC:
        raise RuntimeError("Extracted model ZIP failed size/CRC validation")
    unpacked.replace(target)
    (args.output / "model-source.json").write_text(json.dumps({
        "url": URL, "member": MEMBER, "archive_bytes": total,
        "member_bytes": size, "member_crc32": f"{crc:08x}",
    }, indent=2) + "\n")
    print(f"Validated: {target}", flush=True)


if __name__ == "__main__":
    main()
