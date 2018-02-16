import unittest
import numpy as np
from lazperf import VLRDecompressor, VLRCompressor, RecordSchema



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

class TestVLRCompress(unittest.TestCase):

    def test_compression(self):
        with open('test/simple_points_uncompressed.bin', mode='rb') as fin:
            points_to_compress = np.frombuffer(fin.read(), dtype=np.uint8)

        with open('test/simple.laz', mode="rb") as fin:
            ground_truth = fin.read()

        laszip_vlr_data = ground_truth[
            offset_to_laszip_vlr_data: offset_to_laszip_vlr_data + laszip_vlr_data_size ]

        gt_vlr_data = np.frombuffer(laszip_vlr_data, dtype=np.uint8)
        gt_point_compressed = np.frombuffer(ground_truth[offset_to_point_data:], dtype=np.uint8)


        rs = RecordSchema()
        rs.add_gps_time()
        rs.add_rgb()

        compressor = VLRCompressor(rs, offset_to_laszip_vlr_data + laszip_vlr_data_size)
        compressed = compressor.compress(points_to_compress, point_count)
        vlr_data = compressor.get_vlr_data()

        self.assertTrue(np.all(vlr_data == gt_vlr_data))

        chunk_table_offset = compressed[:8].tobytes()
        chunk_table = compressed[:-8].tobytes()
        points_compressed = compressed[8:].tobytes()

        gt_chunk_table_offset = ground_truth[offset_to_point_data - 8:offset_to_point_data]
        gt_chunk_table = ground_truth[offset_to_point_data:-8]
        gt_points_compressed = ground_truth[offset_to_point_data:]

        self.assertEqual(chunk_table_offset, gt_chunk_table_offset)
        self.assertEqual(points_compressed, gt_points_compressed)


if __name__ == '__main__':
    unittest.main()
        

        