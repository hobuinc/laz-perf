# distutils: language=c++
from libcpp.vector cimport vector
from libc.stdint cimport uint8_t, int32_t, uint64_t

cdef extern from "PyLazperfTypes.hpp" namespace "pylazperf":
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
        Decompressor(const uint8_t *data, size_t dataLen) except +
        size_t decompress(char* buffer, size_t length)  except +
        size_t getPointSize()
        void add_dimension(LazPerfType t)



cdef extern from "PyLazperf.hpp" namespace "pylazperf":
    cdef cppclass Compressor:
        Compressor(vector[uint8_t]& arr) except +
        size_t compress(const char* buffer, size_t length)  except +
        size_t getPointSize()
        void add_dimension(LazPerfType t)
        void done()
        const vector[uint8_t]* data()
        void copy_data_to(uint8_t *dest) const



cdef extern from "laz-perf/factory.hpp" namespace "laszip::factory":
    cdef cppclass record_item:
        @staticmethod
        const record_item& point();
        @staticmethod
        const record_item& gpstime();
        @staticmethod
        const record_item& rgb();
        @staticmethod
        const record_item& eb(size_t count);

cdef extern from "laz-perf/factory.hpp" namespace "laszip::factory":
    cdef cppclass record_schema:
        record_schema()
        void push(const record_item&)


cdef extern from 'laz-perf/io.hpp' namespace "laszip::io":
    cdef cppclass laz_vlr:
        laz_vlr()
        void extract(char *data)
        size_t size() const
        @staticmethod
        laz_vlr from_schema(const record_schema)

cdef extern from "PyLazperf.hpp" namespace "pylazperf":
    cdef cppclass VlrDecompressor:
        VlrDecompressor(const unsigned char* compressed_data, size_t dataLength, size_t pointSize, const char *vlr) except +
        size_t decompress(char* buffer)  except +
        size_t getPointSize()


cdef extern from "PyLazperf.hpp" namespace "pylazperf":
    cdef cppclass VlrCompressor:
        VlrCompressor(record_schema, uint64_t offset) except +
        size_t compress(const char *inbuf) except +
        void done()
        const vector[uint8_t]* data()
        size_t getPointSize()
        size_t vlrDataSize() const
        void extractVlrData(char* out)
        void copy_data_to(uint8_t *dst) const