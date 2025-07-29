cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/fx3-toolchain.cmake" CACHE FILEPATH "Toolchain file")

project(Fx3Demos
        VERSION 1.0.0
        DESCRIPTION "FX3 Firmware Demos (C & C++)"
        LANGUAGES C CXX ASM)


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FX3Macros)

# -----------------------------------------------------------------------------
# 选项
# -----------------------------------------------------------------------------
option(ENABLE_PEDANTIC "Enable -Wpedantic compiler option" OFF)
option(ENABLE_WERROR   "Enable -Werror compiler option"   OFF)
option(ENABLE_STDC     "Enable standard C library"        ON)
option(ENABLE_STDCXX   "Enable standard C++ library"      ON)

# 选择要构建的 demo
option(BUILD_DEMO_C   "Build pure-C demo target"   ON)
option(BUILD_DEMO_C_2   "Build pure-C demo 2 target"   ON)
option(BUILD_DEMO_CPP "Build C++ demo target"      ON)

# -----------------------------------------------------------------------------
# 公共编译标志
# -----------------------------------------------------------------------------
set(COMMON_COMPILE_FLAGS "")
if(ENABLE_PEDANTIC)
  list(APPEND COMMON_COMPILE_FLAGS -Wpedantic)
endif()
if(ENABLE_WERROR)
  list(APPEND COMMON_COMPILE_FLAGS -Werror)
endif()

# 设置全局编译标志，供子目录使用
set(COMMON_COMPILE_FLAGS ${COMMON_COMPILE_FLAGS})

# 设置全局编译定义，供子目录使用
set(COMMON_COMPILE_DEFINITIONS
        PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# -----------------------------------------------------------------------------
# 添加子目录
# -----------------------------------------------------------------------------
if(BUILD_DEMO_C)
  add_subdirectory(demo_c)
  add_subdirectory(demo_c_1)
endif()

if(BUILD_DEMO_C_2)
  add_subdirectory(demo_c_2)
endif()

if(BUILD_DEMO_CPP)
  add_subdirectory(demo_cpp)
endif()

# -----------------------------------------------------------------------------
# 构建信息汇总
# -----------------------------------------------------------------------------

fx3_show_config()
