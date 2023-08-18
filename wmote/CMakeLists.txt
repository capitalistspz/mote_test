project(wmote)

include(FetchContent)
option(ENABLE_LOGGING "Enables logging, uses fmt" ON)
FetchContent_Declare(hidapi
        GIT_REPOSITORY https://github.com/libusb/hidapi.git
        GIT_TAG hidapi-0.14.0
)

FetchContent_MakeAvailable(hidapi)

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