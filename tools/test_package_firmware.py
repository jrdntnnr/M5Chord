import hashlib
import struct
import unittest

from package_firmware import EXPECTED_PARTITIONS, validate_partitions, validate_segments, verify_merged


def table(partitions=None):
    entries = EXPECTED_PARTITIONS if partitions is None else partitions
    data = b"".join(struct.pack("<HBBII16sI", 0x50AA, *values, name.encode(), 0)
                    for name, values in entries.items())
    return data + b"\xeb\xeb" + b"\xff" * 14 + hashlib.md5(data).digest()


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.segments = {0: b"boot", 0x8000: table(), 0xE000: b"ota", 0x10000: b"app"}
        self.merged = bytearray(b"\xff" * 0x11000)
        for offset, data in self.segments.items():
            self.merged[offset:offset + len(data)] = data

    def test_valid(self):
        validate_segments(self.segments)
        verify_merged(self.merged, self.segments)

    def test_bad_checksum(self):
        data = bytearray(table())
        data[12] ^= 1
        with self.assertRaises(ValueError):
            validate_partitions(data)

    def test_wrong_layout(self):
        entries = dict(EXPECTED_PARTITIONS)
        entries["app0"] = (0, 0x10, 0x20000, 0x320000)
        with self.assertRaises(ValueError):
            validate_partitions(table(entries))

    def test_missing_checksum(self):
        with self.assertRaises(ValueError):
            validate_partitions(table()[:-32])

    def test_wrong_offsets(self):
        self.segments[0x1000] = self.segments.pop(0)
        with self.assertRaises(ValueError):
            validate_segments(self.segments)

    def test_oversized_app(self):
        self.segments[0x10000] = b"a" * 0x330001
        with self.assertRaises(ValueError):
            validate_segments(self.segments)

    def test_empty_app(self):
        self.segments[0x10000] = b""
        with self.assertRaises(ValueError):
            validate_segments(self.segments)

    def test_corrupt_merge(self):
        self.merged[0x10000] ^= 1
        with self.assertRaises(ValueError):
            verify_merged(self.merged, self.segments)

    def test_private_settings(self):
        self.merged[0x9000] = 0
        with self.assertRaises(ValueError):
            verify_merged(self.merged, self.segments)


if __name__ == "__main__":
    unittest.main()
