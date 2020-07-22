set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "Enable building cJSON test" FORCE)
add_subdirectory(cJSON)