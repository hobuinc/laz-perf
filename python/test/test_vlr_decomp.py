import unittest
import numpy as np
from lazperf import VLRDecompressor



las_header_size = 227
vlr_header_size = 54
offset_to_laszip_vlr_data = las_header_size + vlr_header_size
laszip_vlr_data_size = 52
offset_to_point_data = offset_to_laszip_vlr_data + laszip_vlr_data_size + 8
point_count = 1065

point_dtype = np.dtype([('X', '<i4'), ('Y', '<i4'), ('Z', '<i4'), ('intensity', '<u2'), ('bit_fields', 'u1'), ('raw_classification', 'u1'), ('scan_angle_rank', 'i1'), ('user_data', 'u1'), ('point_source_id', '<u2'), ('gps_time', '<f8'), ('red', '<u2'), ('green', '<u2'), ('blue', '<u2')])



class TestVLRDecompress(unittest.TestCase):
    def test_decompression(self):

        with open('test/simple_points_uncompressed.bin', mode='rb') as fin:
            groud_truth  = fin.read()

        with open('test/simple.laz', mode="rb") as fin:
            raw_data = fin.read()

        laszip_vlr_data = raw_data[
            offset_to_laszip_vlr_data: offset_to_laszip_vlr_data + laszip_vlr_data_size ]

        raw_points = raw_data[offset_to_point_data:] 

        laszip_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)
        compressed_points = np.frombuffer(raw_points, dtype=np.uint8)

        decompressor = VLRDecompressor(compressed_points, laszip_vlr_data)
        points_decompressed = decompressor.decompress_points(point_count)
        
        points_decompressed = np.frombuffer(points_decompressed, dtype=point_dtype)
        points_groud_truth = np.frombuffer(groud_truth, dtype=point_dtype)

        self.assertTrue(np.all(points_groud_truth == points_decompressed))


if __name__ == '__main__':
    unittest.main()
        

        