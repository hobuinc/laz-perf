# distutils: language = c++

from libcpp.vector cimport vector
from libcpp.string cimport string
from libc.stdint cimport uint8_t, int32_t
from cpython.version cimport PY_MAJOR_VERSION
cimport numpy as np
import numpy as np
np.import_array()
from cpython.array cimport array, clone
from cpython cimport PyObject, Py_INCREF
from cython.operator cimport dereference as deref, preincrement as inc
import json as jsonlib

def buildNumpyDescription(schema):
    """Given a Greyhound schema, convert it into a numpy dtype description http://docs.scipy.org/doc/numpy/reference/generated/numpy.dtype.html"""
    formats = []
    names = []
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
        Decompressor(vector[uint8_t]& arr, const char*) except +
        size_t decompress(char* buffer, size_t length)
        size_t getPointSize()
        const char* getJSON()
        void add_dimension(LazPerfType t)

cdef class PyDecompressor:
    cdef Decompressor *thisptr      # hold a c++ instance which we're wrapping

    def get_type(self, size, t):
    #    print 'getting type for  %s size %d' % (t, size)
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

    def add_dimensions(self, jsondata):

        data = jsonlib.loads(jsondata)
        for dim in data:
            t = self.get_type(dim['size'], dim['type'])
            self.thisptr.add_dimension(t)


    def decompress(self, length):
        cdef np.ndarray[uint8_t, ndim=1, mode="c"] arr
        arr = np.zeros(length, dtype=np.uint8)
        point_count = self.thisptr.decompress(arr.data, arr.shape[0])
        output = np.resize(arr, self.thisptr.getPointSize() * point_count)
        schema = jsonlib.loads(self.json)
        view = output.view(dtype=buildNumpyDescription(schema))
        return view

    def _init(self):
        self.add_dimensions(self.json)

    def __cinit__(self, np.ndarray[uint8_t, ndim=1, mode="c"]  arr not None, unicode jsondata):
        cdef char* x
        cdef uint8_t* buf
        cdef vector[uint8_t]* v

        buf = <uint8_t*> arr.data;
        v = new vector[uint8_t]()
        v.assign(buf, buf + len(arr))

        if PY_MAJOR_VERSION >= 3:
            py_byte_string = jsondata.encode('UTF-8')
            x = py_byte_string
            self.thisptr = new Decompressor(v[0], x)
        else:
            self.thisptr = new Decompressor(v[0], jsondata)

        self._init()

    def __dealloc__(self):
        del self.thisptr


    property json:
        def __get__(self):
            return self.thisptr.getJSON().decode('UTF-8')

