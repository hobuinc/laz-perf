import unittest
import numpy as np
import struct
from lazperf import VLRDecompressor, VLRCompressor, RecordSchema, LazVLR


LAS_HEADER_SIZE = 227
VLR_HEADER_SIZE = 54
OFFSET_TO_LASZIP_VLR_DATA = LAS_HEADER_SIZE + VLR_HEADER_SIZE
LASZIP_VLR_DATA_SIZE = 52
OFFSET_TO_POINT_DATA = OFFSET_TO_LASZIP_VLR_DATA + LASZIP_VLR_DATA_SIZE
SIZEOF_CHUNK_TABLE_OFFSET = 8
POINT_COUNT = 1065

POINT_DTYPE = np.dtype(
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
    ]
)


class TestVLRDecompress(unittest.TestCase):
    def test_decompression(self):

        with open("python/test/simple_points_uncompressed.bin", mode="rb") as fin:
            points_ground_truth = np.frombuffer(fin.read(), dtype=POINT_DTYPE)

        with open("python/test/simple.laz", mode="rb") as fin:
            raw_data = fin.read()

        laszip_vlr_data = raw_data[
            OFFSET_TO_LASZIP_VLR_DATA : OFFSET_TO_LASZIP_VLR_DATA + LASZIP_VLR_DATA_SIZE
        ]

        raw_points = raw_data[OFFSET_TO_POINT_DATA + SIZEOF_CHUNK_TABLE_OFFSET :]

        laszip_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)
        compressed_points = np.frombuffer(raw_points, dtype=np.uint8)

        decompressor = VLRDecompressor(
            compressed_points, POINT_DTYPE.itemsize, laszip_vlr_data
        )
        points_decompressed = decompressor.decompress_points(POINT_COUNT)

        points_decompressed = np.frombuffer(points_decompressed, dtype=POINT_DTYPE)

        self.assertTrue(
            np.all(
                np.allclose(
                    points_ground_truth[dim_name], points_decompressed[dim_name]
                )
                for dim_name in POINT_DTYPE.names
            )
        )


class TestVLRCompress(unittest.TestCase):
    def test_compression(self):
        with open("python/test/simple_points_uncompressed.bin", mode="rb") as fin:
            points_to_compress = np.frombuffer(fin.read(), dtype=np.uint8)

        with open("python/test/simple.laz", mode="rb") as fin:
            ground_truth = fin.read()

        laszip_vlr_data = ground_truth[
            OFFSET_TO_LASZIP_VLR_DATA : OFFSET_TO_LASZIP_VLR_DATA + LASZIP_VLR_DATA_SIZE
        ]

        gt_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)

        rs = RecordSchema()
        rs.add_point()
        rs.add_gps_time()
        rs.add_rgb()

        vlr = LazVLR(rs)
        vlr_data = vlr.data()

        self.assertEqual(vlr.data_size(), LASZIP_VLR_DATA_SIZE)
        self.assertTrue(np.all(vlr_data == gt_vlr_data))

        offset_to_data = OFFSET_TO_LASZIP_VLR_DATA + vlr.data_size()
        self.assertEqual(offset_to_data, OFFSET_TO_POINT_DATA)

        compressor = VLRCompressor(rs, offset_to_data)
        points_compressed = compressor.compress(points_to_compress)

        chunk_table_offset = struct.unpack(
            "<Q", points_compressed[:SIZEOF_CHUNK_TABLE_OFFSET].tobytes()
        )[0]
        gt_chunk_table_offset = struct.unpack(
            "<Q",
            ground_truth[
                OFFSET_TO_POINT_DATA : OFFSET_TO_POINT_DATA + SIZEOF_CHUNK_TABLE_OFFSET
            ],
        )[0]
        self.assertEqual(chunk_table_offset, gt_chunk_table_offset)

        # This is the chunk table offset relative to the point data
        # without las header and vlrs
        relative_chunk_table_offset = chunk_table_offset - offset_to_data
        chunk_table = points_compressed[relative_chunk_table_offset:].tobytes()
        gt_chunk_table = ground_truth[gt_chunk_table_offset:]
        self.assertEqual(chunk_table, gt_chunk_table)

        points_compressed = points_compressed[SIZEOF_CHUNK_TABLE_OFFSET:].tobytes()
        gt_points_compressed = ground_truth[
            OFFSET_TO_POINT_DATA + SIZEOF_CHUNK_TABLE_OFFSET :
        ]
        self.assertEqual(points_compressed, gt_points_compressed)

    def test_raises(self):
        with open("python/test/simple_points_uncompressed.bin", mode="rb") as fin:
            point_buffer = fin.read()

        rs = RecordSchema()
        rs.add_gps_time()
        rs.add_rgb()

        vlr = LazVLR(rs)

        # Cut off point data to check if exception is properly raised
        points_to_compress = np.frombuffer(point_buffer[:-12], dtype=np.uint8)
        offset_to_data = OFFSET_TO_LASZIP_VLR_DATA + vlr.data_size()
        compressor = VLRCompressor(rs, offset_to_data)

        with self.assertRaises(ValueError):
            _ = compressor.compress(points_to_compress)


if __name__ == "__main__":
    unittest.main()
