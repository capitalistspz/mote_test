project(wmote)

option(ENABLE_LOGGING "Enables logging, uses fmt" ON)

find_package(hidapi REQUIRED)


find_package(fmt REQUIRED)
add_library(wmote wiimote.cpp wiimote.hpp wiimote_handle.cpp
        vec.hpp
        extensions.hpp
        logging.hpp
        reports.hpp
        logging.cpp
        internal_logging.cpp
        calibration.hpp
        wiimote_get.cpp
        writes.hpp
        byteswap.hpp
)

target_link_libraries(wmote hidapi::hidapi)
if (ENABLE_LOGGING)
    target_link_libraries(wmote fmt::fmt)
endif()