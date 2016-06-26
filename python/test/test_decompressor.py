import unittest
import json
from lazperf import Decompressor, buildNumpyDescription
import numpy as np
import struct

class TestDecompression(unittest.TestCase):

    def test_decompressor(self):
        schema = [{u'type': u'floating', u'name': u'X', u'size': 8}, {u'type': u'floating', u'name': u'Y', u'size': 8}, {u'type': u'floating', u'name': u'Z', u'size': 8}, {u'type': u'unsigned', u'name': u'Origin', u'size': 8}, {u'type': u'unsigned', u'name': u'Intensity', u'size': 2}, {u'type': u'unsigned', u'name': u'ReturnNumber', u'size': 1}, {u'type': u'unsigned', u'name': u'NumberOfReturns', u'size': 1}, {u'type': u'unsigned', u'name': u'ScanDirectionFlag', u'size': 1}, {u'type': u'unsigned', u'name': u'EdgeOfFlightLine', u'size': 1}, {u'type': u'unsigned', u'name': u'Classification', u'size': 1}, {u'type': u'floating', u'name': u'ScanAngleRank', u'size': 4}, {u'type': u'unsigned', u'name': u'UserData', u'size': 1}, {u'type': u'unsigned', u'name': u'PointSourceId', u'size': 2}, {u'type': u'floating', u'name': u'GpsTime', u'size': 8}]
        s = json.dumps(schema).decode('UTF-8')

        len_compressed = 25691
        len_uncompressed = 54112
        data = file('test/compressed.bin','rb').read()
        original = file('test/uncompressed.bin','rb').read()
        self.assertEqual(len(data), len_compressed, "compressed file length is correct")
        self.assertEqual(len(original), len_uncompressed, "uncompressed file length is correct")

        # last four bytes are the point count
        compressed_point_count = struct.unpack('<L',data[-4:])[0]
        uncompressed_point_count = struct.unpack('<L',original[-4:])[0]
        expected_point_count = 1002

        self.assertEqual(compressed_point_count, uncompressed_point_count, "compressed point count matches expected")
        self.assertEqual(uncompressed_point_count, expected_point_count,"uncompressed point count matches expected")

        arr = np.frombuffer(data, dtype=np.uint8)
        dtype=buildNumpyDescription(json.loads(s))
        self.assertEqual(dtype.itemsize, 54)

        d = Decompressor(arr, s)
        self.assertEqual((len(d.json)), len(s), "confirm Decompressor didn't muck with our JSON")

        decompressed = d.decompress(compressed_point_count * dtype.itemsize)
        uncompressed = np.frombuffer(original[0:-4], dtype=dtype)

        self.assertEqual(uncompressed.shape[0], expected_point_count)
        self.assertEqual(decompressed.shape[0], expected_point_count)
        for i in range(len(uncompressed)):
            self.assertEqual(uncompressed[i], decompressed[i])

def test_suite():
    return unittest.TestSuite(
        [TestXML])

if __name__ == '__main__':
    unittest.main()

