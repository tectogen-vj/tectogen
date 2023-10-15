cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 17)

project(tectogen C CXX)

# Work around dmon incompaitibility with clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Wno-c++11-narrowing)
endif()

add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

set(PROJ_DIR "${CMAKE_SOURCE_DIR}/src"
    CACHE PATH "Root of the project sources")

set(EXTPATH_LIBSOUNDIO "${CMAKE_SOURCE_DIR}/ext/libsoundio"
    CACHE PATH "Where to find libsoundio")

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/ext/imgui"
    CACHE PATH "Where to find imgui, this directory contains imgui.cpp etc.")

  set(IMNODES_DIR "${CMAKE_SOURCE_DIR}/ext/imnodes"
      CACHE PATH "Where to find imnodes")

option(USE_JET_LIVE "Use jet-live library for hot code reloading" OFF)
if(USE_JET_LIVE)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

set(EXTPATH_JET_LIVE ext/jet-live
    CACHE PATH "Where to find jet-live, this directory contains its CMakeLists.txt")

set(EXTPATH_DMON "${CMAKE_SOURCE_DIR}/ext/dmon"
    CACHE PATH "Where dmon.h resides")

set(EXTPATH_VISIT_STRUCT "${CMAKE_SOURCE_DIR}/ext/visit_struct/include"
    CACHE PATH "visit_Struct")

set(CLASSES
  app
  logs
  shader
  shadermanager
  console
  audioconfigwindow
  instreammanager
  widgets
  config
  uicomponent
  context
  nodewindow

  nodeprogrammanager
  nodeprogramlibrary
  nodegraph
  nodeprograminstancewrapper
  nodeprogrammetadata

  asynctex
  multibuffer

  shaderprogram
  displaypayload
  shaderinvocation
  video
  profiler

  shadernodeprogram
  monitoringwindow
)

set(HEADER
  jettools
  ringbuffer
  imconfig

  nodeprogramapi

  monitoring
)

foreach(class ${CLASSES})
  list(APPEND SOURCES ${PROJ_DIR}/${class}.cpp)
  list(APPEND HEADERS ${PROJ_DIR}/${class}.h)
endforeach()

foreach(header ${HEADER})
  list(APPEND HEADERS ${PROJ_DIR}/${header}.h)
endforeach()

list(APPEND SOURCES
    ${PROJ_DIR}/main.cpp
)

list(APPEND HEADERS
    ${EXTPATH_DMON}/dmon.h
)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMNODES_DIR}/imnodes.cpp
    )

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/backends/imgui_impl_glfw.h
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
    ${IMGUI_DIR}/backends/imgui_impl_opengl2.h

    ${IMNODES_DIR}/imnodes.h
    )

set(EXT_SOURCES
    ${EXT_SOURCES}
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    )

set(BINARY_TARGET "${PROJECT_NAME}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(${BINARY_TARGET} "")
target_sources(${BINARY_TARGET}
    PRIVATE
    ${SOURCES}
    ${EXT_SOURCES}
    ${IMGUI_SOURCES}
    ${HEADERS}
    ${IMGUI_HEADERS}
)

set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

IF(WIN32)
  set(OpenGL_GL_PREFERENCE LEGACY) # not needed?

  set(GLFW_LIBRARIES "libglfw3.a") # not needed?
  set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static") # not needed?
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0")
  target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE SOUNDIO_STATIC_LIBRARY)
ENDIF()

## STANDARD ##
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

## MODULES ##
find_package(FFTW REQUIRED COMPONENTS SIMPLE) # signle precision fftw3f
find_package(glfw3 REQUIRED)
# TODO: libsoundio module?

if(USE_JET_LIVE)
    include(${EXTPATH_JET_LIVE}/cmake/jet_live_setup.cmake) # setup needed compiler and linker flags, include this file in your root CMakeLists.txt
    set(JET_LIVE_BUILD_EXAMPLE OFF)
    set(JET_LIVE_SHARED ON) # if you want to
    add_subdirectory(${EXTPATH_JET_LIVE})
    target_link_libraries(${PROJECT_NAME} jet-live)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_JET_LIVE)
    message("Using JetLive")
endif()

set(BUILD_STATIC_LIBS ON CACHE BOOL "Build static libraries" FORCE)
set(BUILD_DYNAMIC_LIBS OFF CACHE BOOL "Build dynamic libraries")
set(BUILD_EXAMPLE_PROGRAMS OFF CACHE BOOL "Build example programs" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)

add_subdirectory(${EXTPATH_LIBSOUNDIO})

# https://github.com/andrewrk/libsoundio/issues/276
set_property(DIRECTORY "${EXTPATH_LIBSOUNDIO}" APPEND PROPERTY COMPILE_DEFINITIONS "NDEBUG")

get_directory_property(LIBSOUNDIO_LIBS DIRECTORY ${EXTPATH_LIBSOUNDIO} DEFINITION LIBSOUNDIO_LIBS)
get_directory_property(SOUNDIO_STATIC_LIBNAME DIRECTORY ${EXTPATH_LIBSOUNDIO} DEFINITION SOUNDIO_STATIC_LIBNAME)


IF (WIN32)
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  set_target_properties(${BINARY_TARGET} PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(${BINARY_TARGET} PROPERTIES LINK_SEARCH_END_STATIC 1)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  target_link_libraries(${BINARY_TARGET} -static-libgcc -static-libstdc++)
  find_package(Threads REQUIRED)

  #TODO: remove explicit references to static library files (above should suffice to link statically)
  target_link_libraries(${BINARY_TARGET} PRIVATE
    libsoundio_static ${LIBSOUNDIO_LIBS}
    libglew32.a
    ${OPENGL_LIBRARIES}
    glfw
    ${FFTW_LIBRARIES}
    ${LIBSOUNDIO_LIBS}
  )
ELSE ()
  target_link_libraries(${BINARY_TARGET} PRIVATE
    libsoundio_static
    ${GLEW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    glfw
    ${FFTW_LIBRARIES}
    ${LIBSOUNDIO_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
  )
ENDIF()


target_include_directories(${BINARY_TARGET} PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW3_INCLUDE_DIRS}
    ${FFTW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
     PRIVATE ${IMGUI_DIR}
     PRIVATE ${IMGUI_DIR}/backends
     PRIVATE ${IMNODES_DIR}
     PRIVATE ${PROJ_DIR}
     ${EXTPATH_LIBSOUNDIO}
     PRIVATE ${EXTPATH_DMON}
     PRIVATE ${EXTPATH_VISIT_STRUCT}
)

target_compile_definitions(${BINARY_TARGET} PRIVATE IMGUI_IMPL_LOADER_GLEW)

file(GLOB SHADER_FILES
  "res/shaders/*"
)
file(COPY ${SHADER_FILES} DESTINATION ${CMAKE_BINARY_DIR}/share/tectogen/shaders)
