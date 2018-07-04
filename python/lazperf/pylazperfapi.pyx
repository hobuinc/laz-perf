# distutils: language = c++

from libcpp.vector cimport vector
from libcpp.string cimport string
from libc.stdint cimport uint8_t, int32_t, uint64_t
from cpython.version cimport PY_MAJOR_VERSION
from cpython.array cimport array, clone

import json as jsonlib
import numpy as np

cimport lazperf
cimport numpy as np
np.import_array()

def get_lazperf_type(size, t):
    if t == 'floating':
        if size == 8:
            return lazperf.Double
        else:
            return lazperf.Float
    if t == 'unsigned':
        if size == 8:
            return lazperf.Unsigned64
        elif size == 4:
            return lazperf.Unsigned32
        elif size == 2:
            return lazperf.Unsigned16
        elif size == 1:
            return lazperf.Unsigned8
        else:
            raise Exception("Unexpected type size '%s' for unsigned type" % size)
    if t == 'signed':
        if size == 8:
            return lazperf.Signed64
        elif size == 4:
            return lazperf.Signed32
        elif size == 2:
            return lazperf.Signed16
        elif size == 1:
            return lazperf.Signed8
        else:
            raise Exception("Unexpected type size '%s' for signed type" % size)

def buildNumpyDescription(schema):
    """Given a Greyhound schema, convert it into a numpy dtype description
    http://docs.scipy.org/doc/numpy/reference/generated/numpy.dtype.html
    """
    formats = []
    names = []

    try:
        schema[0]['type']
    except:
        schema = jsonlib.loads(schema)

    for s in schema:
        t = s['type']
        if t == 'floating':
            t = 'f'
        elif t == 'unsigned':
            t = 'u'
        else:
            t = 'i'

        f = '%s%d' % (t, int(s['size']))
        names.append(s['name'])
        formats.append(f)
    return np.dtype({'names': names, 'formats': formats})

def buildGreyhoundDescription(dtype):
    """Given a numpy dtype, return a Greyhound schema"""
    output = []
    for t in dtype.descr:
        name = t[0]
        dt = dtype.fields[name]
        size = dt[0].itemsize
        tname = dt[0].name

        entry = {}
        if 'float' in tname:
            entry['type'] = 'floating'
        elif 'uint' in tname:
            entry['type'] = 'unsigned'
        else:
            entry['type'] = 'signed'

        entry['size'] = size
        entry['name'] = name
        output.append(entry)
    return output


cdef class PyCompressor:
    """ Class to compress points in the laz format using a json schema or numpy dtype
    to describe the point format
    """
    cdef lazperf.Compressor *thisptr      # hold a c++ instance which we're wrapping
    cdef public str jsondata
    cdef vector[uint8_t] *v

    def __init__(self, object schema):
        """
        schema: numpy dtype or json string of the point schema
        """
        self.v = new vector[uint8_t]()
        self.thisptr = new lazperf.Compressor(self.v[0])

        try:
            self.jsondata = jsonlib.dumps(buildGreyhoundDescription(schema))
        except AttributeError:
            self.jsondata = schema
        self.add_dimensions(self.jsondata)

    def compress(self, np.ndarray arr not None):
        """ Compresses points and return the result as a numpy array of bytes
        """

        cdef np.ndarray[uint8_t, ndim=1, mode="c"] view
        view = arr.view(dtype=np.uint8)

        point_count = self.thisptr.compress(view.data, view.shape[0])
        self.done()
        return self.get_data()

    cdef get_data(self):
        cdef const vector[uint8_t]* v = self.thisptr.data()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] arr = np.zeros(v.size(), dtype=np.uint8)

        self.thisptr.copy_data_to(<uint8_t*> arr.data)
        return arr

    def done(self):
        self.thisptr.done()

    def add_dimensions(self, jsondata):

        data = None
        try:
            jsondata[0]['type']
            data = jsondata
        except:
            data = jsonlib.loads(jsondata)

        for dim in data:
            t = get_lazperf_type(dim['size'], dim['type'])
            self.thisptr.add_dimension(t)

    def __dealloc__(self):
        del self.v
        del self.thisptr


cdef class PyDecompressor:
    """ Class to decompress laz points using a json schema/numpy dtype to
    describe the point format
    """
    cdef lazperf.Decompressor *thisptr      # hold a c++ instance which we're wrapping
    cdef public str jsondata

    def __init__(self, np.ndarray[uint8_t, ndim=1, mode="c"] compressed_points not None, object schema):
        """
        compressed_points: buffer of compressed_points
        schema: numpy dtype or json string of the point schema
        """
        try:
            self.jsondata = jsonlib.dumps(buildGreyhoundDescription(schema))
        except AttributeError:
            self.jsondata = schema

        self.thisptr = new lazperf.Decompressor(
            <const uint8_t*>compressed_points.data, compressed_points.shape[0])
        self.add_dimensions(self.jsondata)

    def decompress(self, size_t num_points):
        """ decompress points

        returns the numpy structured array of the decompressed points
        """
        cdef size_t point_size = self.thisptr.getPointSize()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] out = np.zeros(num_points * point_size, np.uint8)

        point_count = self.thisptr.decompress(out.data, out.shape[0])
        return out.view(dtype=buildNumpyDescription(self.jsondata))

    def add_dimensions(self, jsondata):
        data = None
        try:
            jsondata[0]['type']
            data = jsondata
        except:
            data = jsonlib.loads(jsondata)

        for dim in data:
            t = get_lazperf_type(dim['size'], dim['type'])
            self.thisptr.add_dimension(t)


    def __dealloc__(self):
        del self.thisptr

cdef class PyRecordSchema:
    """ This class is used to represent a LAS record schema
    This RecordSchema is nessecary for the LazVlr to be able to compress
    points meant to be written in a LAZ file.
    """
    cdef lazperf.record_schema schema

    def __init__(self):
        pass

    def add_point(self):
        self.schema.push(lazperf.record_item.point())

    def add_gps_time(self):
        self.schema.push(lazperf.record_item.gpstime())

    def add_rgb(self):
        self.schema.push(lazperf.record_item.rgb())

    def add_extra_bytes(self, size_t count):
        self.schema.push(lazperf.record_item.eb(count))

cdef class PyLazVlr:
    """ Wraps a Lazperf's LazVlr class.
    This class is meant to give access to the Laszip's vlr raw record_data
    to allow writers to write LAZ files with its corresponding laszip vlr.
    """
    cdef lazperf.laz_vlr vlr
    cdef public PyRecordSchema schema

    def __init__(self, PyRecordSchema schema):
        self.schema = schema
        self.vlr = lazperf.laz_vlr.from_schema(schema.schema)

    def data(self):
        """ returns the laszip vlr record_data as a numpy array of bytes
        to be written in the VLR section of a LAZ compressed file.
        """
        cdef size_t vlr_size = self.vlr.size()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] arr = np.ndarray(vlr_size, dtype=np.uint8)

        self.vlr.extract(arr.data)
        return arr

    def data_size(self):
        """ Returns the number of bytes in the lazvlr record_data
        """
        return self.vlr.size()



cdef class PyVLRDecompressor:
    """ Class to decompress laz points stored in a .laz file using the
    Laszip vlr's record_data
    """
    cdef lazperf.VlrDecompressor *thisptr      # hold a c++ instance which we're wrapping

    def __init__(
            self,
            np.ndarray[uint8_t, ndim=1, mode="c"] compressed_points not None,
            size_t point_size,
            np.ndarray[uint8_t, ndim=1, mode="c"] vlr not None
        ):
        """
        compressed_points: buffer of points to be decompressed
        vlr: laszip vlr's record_data as an array of bytes
        """
        cdef const uint8_t *p_compressed =  <const uint8_t*> compressed_points.data
        self.thisptr = new lazperf.VlrDecompressor(
            p_compressed, compressed_points.shape[0], point_size, vlr.data)

    def decompress_points(self, size_t point_count):
        """ decompress the points

        returns the decompressed data as an array of bytes
        """
        cdef size_t point_size = self.thisptr.getPointSize()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] point_out = np.zeros(point_size, dtype=np.uint8)
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] points_uncompressed = np.zeros(point_count * point_size, dtype=np.uint8)
        cdef size_t i = 0
        cdef size_t begin = 0
        cdef size_t end = 0

        # Cython's memory views are needed to get the true C speed when slicing
        cdef uint8_t [:] points_view  = points_uncompressed
        cdef uint8_t [:] out_view = point_out

        for _ in range(point_count):
            self.thisptr.decompress(point_out.data)
            end = begin + point_size
            points_view[begin:end] = out_view
            begin = end

        return points_uncompressed

    def __dealloc__(self):
        del self.thisptr

cdef class PyVLRCompressor:
    """ Class to compress las points into laz format with the record schema
    from a laszip vlr, this is meant to be used by LAZ file writers
    """
    cdef lazperf.VlrCompressor *thisptr;

    def __init__(self, PyRecordSchema py_record_schema, uint64_t offset):
        """
        py_record_schema: The schema of the point format
        offset: offset to the point data (same as the las header field).
        This is needed because the first 8 bytes of the compressed points is an offset to the
        chunk table relative to the start of Las file. (Or you could pass in offset=0 and modify the
        8 bytes yourself)
        """
        self.thisptr = new lazperf.VlrCompressor(py_record_schema.schema, offset)


    def compress(self, np.ndarray arr):
        """ Returns the compressed points as a numpy array of bytes
        """
        cdef np.ndarray[char, ndim=1, mode="c"] view
        view = arr.view(np.uint8)

        cdef char *ptr = arr.data
        cdef size_t point_size = self.thisptr.getPointSize()
        cdef size_t num_bytes = arr.shape[0]
        cdef size_t point_count = num_bytes / point_size
        cdef double float_count = <double> num_bytes / point_size

        if arr.shape[0] % point_size != 0:
            raise ValueError("The number of bytes ({}) is not divisible by the point size ({})"
            " it gives {} points".format(num_bytes, point_size, float_count))

        for i in range(point_count):
            self.thisptr.compress(ptr)
            ptr += point_size

        self.thisptr.done()
        return self.get_data()

    cdef get_data(self):
        cdef const vector[uint8_t]* v = self.thisptr.data()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] arr = np.ndarray(v.size(), dtype=np.uint8)
        self.thisptr.copy_data_to(<uint8_t*>arr.data)
        return arr

    def __dealloc__(self):
        del self.thisptr

