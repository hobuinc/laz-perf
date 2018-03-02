import unittest
import numpy as np
import struct
from lazperf import VLRDecompressor, VLRCompressor, RecordSchema, LazVLR



las_header_size = 227
vlr_header_size = 54
offset_to_laszip_vlr_data = las_header_size + vlr_header_size
laszip_vlr_data_size = 52
gt_offset_to_point_data = offset_to_laszip_vlr_data + laszip_vlr_data_size
sizeof_chunk_table_offset = 8
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

        raw_points = raw_data[gt_offset_to_point_data + sizeof_chunk_table_offset:]

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
        gt_points_compressed = np.frombuffer(ground_truth[gt_offset_to_point_data + sizeof_chunk_table_offset:], dtype=np.uint8)

        rs = RecordSchema()
        rs.add_gps_time()
        rs.add_rgb()

        vlr = LazVLR(rs)
        vlr_data = vlr.data()

        self.assertEqual(vlr.data_size(), laszip_vlr_data_size)
        self.assertTrue(np.all(vlr_data == gt_vlr_data))

        offset_to_data = offset_to_laszip_vlr_data + vlr.data_size()
        self.assertEqual(offset_to_data, gt_offset_to_point_data)

        compressor = VLRCompressor(rs, offset_to_data)
        points_compressed = compressor.compress(points_to_compress)

        chunk_table_offset = struct.unpack('<Q', points_compressed[:sizeof_chunk_table_offset].tobytes())[0]
        gt_chunk_table_offset = struct.unpack('<Q', ground_truth[gt_offset_to_point_data:gt_offset_to_point_data + sizeof_chunk_table_offset])[0]
        self.assertEqual(chunk_table_offset, gt_chunk_table_offset)

        # This is the chunkt table offset relative to the point data
        # without las header and vlrs
        relative_chunk_table_offset = chunk_table_offset - offset_to_data
        chunk_table = points_compressed[relative_chunk_table_offset:].tobytes()
        gt_chunk_table = ground_truth[gt_chunk_table_offset:]
        self.assertEqual(chunk_table, gt_chunk_table)

        points_compressed = points_compressed[sizeof_chunk_table_offset:].tobytes()
        gt_points_compressed = ground_truth[gt_offset_to_point_data + sizeof_chunk_table_offset:]
        self.assertEqual(points_compressed, gt_points_compressed)

    def test_raises(self):
        with open('test/simple_points_uncompressed.bin', mode='rb') as fin:
            point_buffer = fin.read()

        rs = RecordSchema()
        rs.add_gps_time()
        rs.add_rgb()

        vlr = LazVLR(rs)
        vlr_data = vlr.data()

        # Cut off point data to check if exception is properly raised
        points_to_compress = np.frombuffer(point_buffer[:-12], dtype=np.uint8)
        offset_to_data = offset_to_laszip_vlr_data + vlr.data_size()
        compressor = VLRCompressor(rs, offset_to_data)

        with self.assertRaises(ValueError):
            points_compressed = compressor.compress(points_to_compress)

if __name__ == '__main__':
    unittest.main()
