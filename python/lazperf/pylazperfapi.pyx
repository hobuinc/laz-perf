# distutils: language = c++

from libcpp.vector cimport vector
from libcpp.string cimport string
from libc.stdint cimport uint8_t, int32_t
from cpython.version cimport PY_MAJOR_VERSION
cimport numpy as np
import numpy as np
np.import_array()
from cpython.array cimport array, clone
import json as jsonlib

def get_lazperf_type(size, t):
    if t == 'floating':
        if size == 8:
            return Double
        else:
            return Float
    if t == 'unsigned':
        if size == 8:
            return Unsigned64
        elif size == 4:
            return Unsigned32
        elif size == 2:
            return Unsigned16
        elif size == 1:
            return Unsigned8
        else:
            raise Exception("Unexpected type size '%s' for unsigned type" % size)
    if t == 'signed':
        if size == 8:
            return Signed64
        elif size == 4:
            return Signed32
        elif size == 2:
            return Signed16
        elif size == 1:
            return Signed8
        else:
            raise Exception("Unexpected type size '%s' for signed type" % size)

def buildNumpyDescription(schema):
    """Given a Greyhound schema, convert it into a numpy dtype description http://docs.scipy.org/doc/numpy/reference/generated/numpy.dtype.html"""
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

cdef extern from "PyLazperftypes.hpp" namespace "pylazperf":
    enum LazPerfType "pylazperf::Type":
        Double
        Float
        Signed8
        Signed16
        Signed32
        Signed64
        Unsigned8
        Unsigned16
        Unsigned32
        Unsigned64


cdef extern from "PyLazperf.hpp" namespace "pylazperf":
    cdef cppclass Decompressor:
        Decompressor(vector[uint8_t]& arr) except +
        size_t decompress(char* buffer, size_t length)
        size_t getPointSize()
        void add_dimension(LazPerfType t)

cdef extern from "PyLazperf.hpp" namespace "pylazperf":
    cdef cppclass Compressor:
        Compressor(vector[uint8_t]& arr) except +
        size_t compress(const char* buffer, size_t length)
        size_t getPointSize()
        void add_dimension(LazPerfType t)
        void done()
        const vector[uint8_t]* data()

cdef class PyCompressor:
    cdef Compressor *thisptr      # hold a c++ instance which we're wrapping
    cdef public str jsondata

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

    cdef get_data(self):
        cdef const vector[uint8_t]* v = self.thisptr.data()
        cdef size_t size = v.size()
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] arr = np.ndarray(size, dtype=np.uint8)

        arr.data = <char*>v[0].data()
        return arr

    def compress(self, np.ndarray arr not None):

        cdef np.ndarray[uint8_t, ndim=1, mode="c"] view
        view = arr.view(dtype=np.uint8)

        point_count = self.thisptr.compress(view.data, view.shape[0])
        self.done()
        return self.get_data()


    def done(self):
        self.thisptr.done()

    def _init(self):
        self.add_dimensions(self.jsondata)

    def __cinit__(self, object schema):
        cdef char* x
        cdef uint8_t* buf
        cdef vector[uint8_t]* v

        v = new vector[uint8_t]()

        self.thisptr = new Compressor(v[0])
        try:
            self.jsondata = jsonlib.dumps(buildGreyhoundDescription(schema))
        except AttributeError:
            self.jsondata = schema
        self._init()

    def __dealloc__(self):
        del self.thisptr


cdef class PyDecompressor:
    cdef Decompressor *thisptr      # hold a c++ instance which we're wrapping
    cdef public str jsondata

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


    def decompress(self, np.ndarray data):
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] view

        view = data.view(dtype=np.uint8)
        point_count = self.thisptr.decompress(view.data, view.shape[0])
        output = np.resize(view, self.thisptr.getPointSize() * point_count)
        view2 = output.view(dtype=buildNumpyDescription(self.jsondata))
        return view2

    def _init(self):
        self.add_dimensions(self.jsondata)

    def __cinit__(self, np.ndarray[uint8_t, ndim=1, mode="c"]  data not None, object schema):
        cdef char* x
        cdef uint8_t* buf
        cdef vector[uint8_t]* v

        buf = <uint8_t*> data.data;
        v = new vector[uint8_t]()
        v.assign(buf, buf + len(data))

        try:
            self.jsondata = jsonlib.dumps(buildGreyhoundDescription(schema))
        except AttributeError:
            self.jsondata = schema

        self.thisptr = new Decompressor(v[0])
        self._init()

    def __dealloc__(self):
        del self.thisptr

