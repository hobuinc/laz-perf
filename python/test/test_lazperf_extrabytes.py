import unittest

import numpy as np
from lazperf import VLRDecompressor

LAS_HEADER_SIZE = 227
LAS_HEADER_SIZE = 375
LAS_OFFSET_TO_POINTS = 1353
LAS_OFFSET_TO_POINTS = 1501
LAS_VLR_HEADER_SIZE = 54
LAS_POINT_SIZE = 61  # Extra-bytes included
LAS_POINT_COUNT = 1065

LASZIP_VLR_DATA_SIZE = 58

POINTS_DTYPE = np.dtype(
    [('X', '<i4'), ('Y', '<i4'), ('Z', '<i4'), ('intensity', '<u2'), ('bit_fields', 'u1'), ('raw_classification', 'u1'),
     ('scan_angle_rank', 'i1'), ('user_data', 'u1'), ('point_source_id', '<u2'), ('gps_time', '<f8'), ('red', '<u2'),
     ('green', '<u2'), ('blue', '<u2'), ('Colors', '<u2', (3,)), ('Reserved', 'u1', (7,)), ('Flags', 'i1', (2,)),
     ('Intensity', '<u4'), ('Time', '<u8')])


class TestExtraBytesDecompression(unittest.TestCase):
    def test_decompression(self):
        # with open('../test/raw-sets/extrabytes.laz', mode='rb') as f:
        with open('C:/Users/t.montaigu/Projects/pylas/pylastests/extra.laz', mode='rb') as f:
            raw_laz = f.read()

        # The file contains 2 vlrs in this order:
        # ExtraBytesVLR, LaszipVLR
        offset_to_laszip_vlr = LAS_OFFSET_TO_POINTS - LASZIP_VLR_DATA_SIZE
        laszip_vlr_data = raw_laz[offset_to_laszip_vlr: offset_to_laszip_vlr + LASZIP_VLR_DATA_SIZE]
        assert len(laszip_vlr_data) == LASZIP_VLR_DATA_SIZE

        raw_compressed_points = np.frombuffer(raw_laz[LAS_OFFSET_TO_POINTS:], np.uint8)
        laszip_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)

        decompressor = VLRDecompressor(raw_compressed_points, POINTS_DTYPE.itemsize, laszip_vlr_data)
        decompressed_points = decompressor.decompress_points(LAS_POINT_COUNT)
