include_directories(../..)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_definitions(-DWIN_EXPORT)

set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
set(BUILD_DYNAMIC_LIBS OFF CACHE BOOL "Build dynamic libraries" FORCE)
set(BUILD_EXAMPLE_PROGRAMS OFF CACHE BOOL "Build example programs" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(ENABLE_JACK ON CACHE BOOL "Enable JACK backend" FORCE)
set(ENABLE_PULSEAUDIO ON CACHE BOOL "Enable PulseAudio backend" FORCE)
set(ENABLE_ALSA ON CACHE BOOL "Enable ALSA backend" FORCE)
set(ENABLE_COREAUDIO ON CACHE BOOL "Enable CoreAudio backend" FORCE)
set(ENABLE_WASAPI ON CACHE BOOL "Enable WASAPI backend" FORCE)


add_subdirectory(libsoundio)
if(TARGET libsoundio_static)
        set_property(TARGET libsoundio_static PROPERTY POSITION_INDEPENDENT_CODE ON)

        get_directory_property(LIBSOUNDIO_LIBS DIRECTORY libsoundio DEFINITION LIBSOUNDIO_LIBS)

        include_directories(libsoundio)

        set(SRC audio.c)
        
        add_library(audio SHARED ${SRC})
        target_link_libraries(audio libsoundio_static ${LIBSOUNDIO_LIBS})
        set_target_properties(audio PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)
        set_target_properties(audio PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/stdlib/libs/)

endif()