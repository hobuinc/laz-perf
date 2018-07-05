import struct
import unittest

import numpy as np
from lazperf import VLRDecompressor, RecordSchema, LazVLR, VLRCompressor

LAS_HEADER_SIZE = 227
LAZ_OFFSET_TO_POINTS = 1353
LAS_VLR_HEADER_SIZE = 54
LAS_POINT_SIZE = 61  # Extra-bytes included
LAS_POINT_COUNT = 1065

LASZIP_VLR_DATA_SIZE = 58
LASZIP_CHUNK_TABLE_SIZE = 8
OFFSET_TO_LASZIP_VLR_DATA = LAZ_OFFSET_TO_POINTS - LASZIP_VLR_DATA_SIZE

POINTS_DTYPE = np.dtype(
    [
        ("X", "<i4"),
        ("Y", "<i4"),
        ("Z", "<i4"),
        ("intensity", "<u2"),
        ("bit_fields", "u1"),
        ("raw_classification", "u1"),
        ("scan_angle_rank", "i1"),
        ("user_data", "u1"),
        ("point_source_id", "<u2"),
        ("gps_time", "<f8"),
        ("red", "<u2"),
        ("green", "<u2"),
        ("blue", "<u2"),
        ("Colors", "<u2", (3,)),
        ("Reserved", "u1", (7,)),
        ("Flags", "i1", (2,)),
        ("Intensity", "<u4"),
        ("Time", "<u8"),
    ]
)


class TestExtraBytesDecompression(unittest.TestCase):
    def test_decompression(self):
        with open("../test/raw-sets/extrabytes.laz", mode="rb") as f:
            raw_laz = f.read()

        # The file contains 2 vlrs in this order:
        # ExtraBytesVLR, LaszipVLR
        laszip_vlr_data = raw_laz[
            OFFSET_TO_LASZIP_VLR_DATA : OFFSET_TO_LASZIP_VLR_DATA + LASZIP_VLR_DATA_SIZE
        ]
        self.assertEqual(len(laszip_vlr_data), LASZIP_VLR_DATA_SIZE)

        raw_compressed_points = np.frombuffer(
            raw_laz[LAZ_OFFSET_TO_POINTS + LASZIP_CHUNK_TABLE_SIZE :], np.uint8
        )
        laszip_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)

        decompressor = VLRDecompressor(
            raw_compressed_points, POINTS_DTYPE.itemsize, laszip_vlr_data
        )
        decompressed_points = decompressor.decompress_points(LAS_POINT_COUNT)
        decompressed_points = np.frombuffer(decompressed_points, dtype=POINTS_DTYPE)

        with open("../test/raw-sets/extrabytes.las", mode="rb") as f:
            raw_las = f.read()

        offset_to_points = (
            LAZ_OFFSET_TO_POINTS - LAS_VLR_HEADER_SIZE - LASZIP_VLR_DATA_SIZE
        )
        expected_decompressed_points = np.frombuffer(
            raw_las[offset_to_points:], dtype=POINTS_DTYPE
        )

        self.assertTrue(
            np.all(
                np.allclose(
                    expected_decompressed_points[dim_name],
                    decompressed_points[dim_name],
                )
                for dim_name in POINTS_DTYPE.names
            )
        )


class TestExtraBytesCompression(unittest.TestCase):
    def test_compression(self):
        with open("../test/raw-sets/extrabytes.laz", mode="rb") as f:
            raw_laz = f.read()

        with open("../test/raw-sets/extrabytes.las", mode="rb") as f:
            raw_las = f.read()

        record_schema = RecordSchema()
        record_schema.add_point()
        record_schema.add_gps_time()
        record_schema.add_rgb()
        record_schema.add_extra_bytes(27)

        vlr = LazVLR(record_schema)
        vlr_data = vlr.data()
        expected_vlr_data = np.frombuffer(
            raw_laz[
                OFFSET_TO_LASZIP_VLR_DATA : OFFSET_TO_LASZIP_VLR_DATA
                + LASZIP_VLR_DATA_SIZE
            ],
            np.uint8,
        )

        self.assertEqual(LASZIP_VLR_DATA_SIZE, vlr.data_size())
        self.assertTrue(np.all(expected_vlr_data == vlr_data))

        compressor = VLRCompressor(record_schema, LAZ_OFFSET_TO_POINTS)
        points_to_compress = np.frombuffer(
            raw_las[
                LAZ_OFFSET_TO_POINTS - LASZIP_VLR_DATA_SIZE - LAS_VLR_HEADER_SIZE :
            ],
            np.uint8,
        )
        points_compressed = compressor.compress(points_to_compress)

        chunk_table_offset = struct.unpack(
            "<Q", points_compressed[:LASZIP_CHUNK_TABLE_SIZE].tobytes()
        )[0]
        expected_chunk_table_offset = struct.unpack(
            "<Q",
            raw_laz[
                LAZ_OFFSET_TO_POINTS : LAZ_OFFSET_TO_POINTS + LASZIP_CHUNK_TABLE_SIZE
            ],
        )[0]
        self.assertEqual(chunk_table_offset, expected_chunk_table_offset)

        relative_chunk_table_offset = chunk_table_offset - LAZ_OFFSET_TO_POINTS
        chunk_table = points_compressed[relative_chunk_table_offset:].tobytes()
        expected_chunk_table = raw_laz[expected_chunk_table_offset:]
        self.assertEqual(chunk_table, expected_chunk_table)

        expected_points_compressed = raw_laz[
            LAZ_OFFSET_TO_POINTS + LASZIP_CHUNK_TABLE_SIZE :
        ]
        points_compressed = points_compressed[LASZIP_CHUNK_TABLE_SIZE:].tobytes()
        self.assertEqual(expected_points_compressed, points_compressed)
