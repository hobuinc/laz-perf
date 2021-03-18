
add_library(
    lazperf SHARED
    interface.cpp
    detail/field_point10.cpp
    detail/field_gpstime10.cpp
    detail/field_rgb10.cpp
    detail/field_byte10.cpp
    detail/field_rgb14.cpp
    detail/field_nir14.cpp
    detail/field_point14.cpp
    detail/field_byte14.cpp
)
