
if (EMSCRIPTEN)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(${LAZPERF_LIB}
    ${LIB_TYPE}
        charbuf.cpp
        lazperf.cpp
        filestream.cpp
        io.cpp
        vlr.cpp
        detail/field_point10.cpp
        detail/field_gpstime10.cpp
        detail/field_rgb10.cpp
        detail/field_byte10.cpp
        detail/field_rgb14.cpp
        detail/field_nir14.cpp
        detail/field_point14.cpp
        detail/field_byte14.cpp
)
lazperf_target_compile_settings(${LAZPERF_LIB})
