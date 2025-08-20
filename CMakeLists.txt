cmake_minimum_required(VERSION 3.13)
project(inspectorTool)

include(./third_party/vendor_tools/vendor.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0063 NEW)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(TGFX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tgfx)
set(INSPECTOR_COMMON_DIR ${TGFX_DIR}/src/debug)

if (NOT CMAKE_PREFIX_PATH)
    if (NOT EXISTS ${PROJECT_SOURCE_DIR}/QTCMAKE.cfg)
        file(WRITE ${PROJECT_SOURCE_DIR}/QTCMAKE.cfg
                "set(CMAKE_PREFIX_PATH /Users/[username]/Qt/6.6.1/macos/lib/cmake)  #put your own QT path here")
    endif ()
    include("QTCMAKE.cfg")
endif ()

string(REGEX MATCH "([0-9]+)\\.[0-9]+\\.[0-9]+" QT_VERSION ${CMAKE_PREFIX_PATH})
if (QT_VERSION)
    string(REGEX MATCH "^[0-9]+" QT_VERSION_MAJOR ${QT_VERSION})
    if (QT_VERSION_MAJOR GREATER_EQUAL 6 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR "QT has dropped support for 32-bit builds since version 6.0.0")
    endif ()
endif ()

set(TGFX_USE_QT ON)
set(TGFX_BUILD_INSPECTOR OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory(${TGFX_DIR} tgfx EXCLUDE_FROM_ALL)
list(APPEND PAG_STATIC_LIBS $<TARGET_FILE:tgfx>)
list(APPEND PAG_DEPEND_TARGETS tgfx)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL qml Quick WebSockets Network QuickControls2 Core5Compat)
list(APPEND TGFX_INSPECTOR_LIBS Qt6::Core Qt6::Widgets Qt6::WebSockets Qt6::OpenGL Qt6::Qml Qt6::Quick Qt6::Network
        Qt6::QuickControls2 Qt6::Core5Compat)
qt6_add_resources(QT_RESOURCES res.qrc)

if (APPLE)
    find_library(APPLICATION_SERVICES_FRAMEWORK ApplicationServices REQUIRED)
    list(APPEND TGFX_INSPECTOR_LIBS ${APPLICATION_SERVICES_FRAMEWORK})
    find_library(QUARTZ_CORE QuartzCore REQUIRED)
    list(APPEND TGFX_INSPECTOR_LIBS ${QUARTZ_CORE})
    find_library(COCOA Cocoa REQUIRED)
    list(APPEND TGFX_INSPECTOR_LIBS ${COCOA})
    find_library(FOUNDATION Foundation REQUIRED)
    list(APPEND TGFX_INSPECTOR_LIBS ${FOUNDATION})
    find_library(ICONV_LIBRARIES NAMES iconv libiconv libiconv-2 c)
    list(APPEND TGFX_INSPECTOR_LIBS ${ICONV_LIBRARIES})
    find_library(VIDEOTOOLBOX VideoToolbox)
    list(APPEND TGFX_INSPECTOR_LIBS ${VIDEOTOOLBOX})
    find_library(CORE_MEDIA CoreMedia)
    list(APPEND TGFX_INSPECTOR_LIBS ${CORE_MEDIA})
    find_library(COMPRESSION_LIBRARIES NAMES compression)
    list(APPEND TGFX_INSPECTOR_LIBS ${COMPRESSION_LIBRARIES})
elseif (WIN32)
    set(BUILD_USE_64BITS ON)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
    find_library(Bcrypt_LIB Bcrypt)
    list(APPEND TGFX_INSPECTOR_LIBS ${Bcrypt_LIB})
    find_library(ws2_32_LIB ws2_32)
    list(APPEND TGFX_INSPECTOR_LIBS ${ws2_32_LIB})
endif ()

list(APPEND TGFX_INSPECTOR_SRC src src/tags src/socket/*.* src/layerInspector/*.*)
file(GLOB TGFX_INSPECTOR_FILES src/*.* src/tags/*.* src/socket/*.* src/layerInspector/*.*)
set(LZ4_DIR ${TGFX_DIR}/third_party/lz4/lib)

list(APPEND LZ4_FILE ${LZ4_DIR}/lz4.c)
list(APPEND TGFX_INSPECTOR_COMMON_INCLUDE ${LZ4_DIR} ${INSPECTOR_COMMON_DIR})

file(GLOB TGFX_INSPECTOR_COMMON_FILES
        ${LZ4_FILE}
        ${INSPECTOR_COMMON_DIR}/Socket.cpp)

list(APPEND INSPECTOR_STATIC_VENDORS KDDockWidgets)
add_vendor_target(inspector-vendor STATIC_VENDORS ${INSPECTOR_STATIC_VENDORS})
find_vendor_libraries(inspector-vendor STATIC INSPECTOR_VENDOR_STATIC_LIBRARIES)
list(APPEND TGFX_INSPECTOR_LIBS ${INSPECTOR_VENDOR_STATIC_LIBRARIES})
list(APPEND TGFX_INSPECTOR_DEFINE KDDW_FRONTEND_QTQUICK KDDW_FRONTEND_QT KDDW_QTGUI_TYPES)

list(APPEND TGFX_INSPECTOR_INCLUDE ./third_party/KDDockWidgets/src/fwd_headers)

add_executable(inspectorTool ${RC_FILES} ${TGFX_INSPECTOR_FILES} ${TGFX_INSPECTOR_COMMON_FILES} ${QT_RESOURCES})
target_include_directories(inspectorTool PUBLIC ${TGFX_INSPECTOR_SRC} ${TGFX_INSPECTOR_COMMON_INCLUDE})
target_include_directories(inspectorTool SYSTEM PRIVATE ${TGFX_INSPECTOR_INCLUDE})
target_compile_definitions(inspectorTool PUBLIC ${TGFX_INSPECTOR_DEFINE})
target_link_libraries(inspectorTool tgfx ${TGFX_INSPECTOR_LIBS})