# Provide an idempotent c-ares finder for gRPC packages that require the
# c-ares::cares target. Trantor may already have created c-ares_lib while
# loading Drogon; reuse it instead of invoking Trantor's finder a second time.

if (TARGET c-ares_lib)
    set(CARES_INCLUDE_DIR "${C-ARES_INCLUDE_DIRS}")
    set(CARES_LIBRARY "${C-ARES_LIBRARIES}")
    if (NOT TARGET c-ares::cares)
        add_library(c-ares::cares ALIAS c-ares_lib)
    endif ()
else ()
    find_path(CARES_INCLUDE_DIR NAMES ares.h)
    find_library(CARES_LIBRARY NAMES cares)

    if (CARES_INCLUDE_DIR AND CARES_LIBRARY AND NOT TARGET c-ares::cares)
        add_library(c-ares::cares UNKNOWN IMPORTED)
        set_target_properties(c-ares::cares PROPERTIES
            IMPORTED_LOCATION "${CARES_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CARES_INCLUDE_DIR}")
    endif ()
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(c-ares
    REQUIRED_VARS CARES_INCLUDE_DIR CARES_LIBRARY)
