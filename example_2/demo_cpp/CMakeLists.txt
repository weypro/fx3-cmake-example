# 源码收集
file(GLOB_RECURSE DEMO_CPP_SOURCES
        "*.cpp" "*.c")
file(GLOB_RECURSE DEMO_CPP_HEADERS
        "*.hpp" "*.h")

if(NOT DEMO_CPP_SOURCES)
    message(WARNING "[demo_cpp] No source files found in demo_cpp/")
endif()

# 设置FX3选项
set(_fx3_opts_cpp ENABLE_CXX)
if(ENABLE_STDC)
    list(APPEND _fx3_opts_cpp ENABLE_STDC)
endif()
if(NOT ENABLE_STDCXX)
    list(APPEND _fx3_opts_cpp NO_STDCXX)
endif()

# 创建固件目标
fx3_add_firmware(demo_cpp
        SOURCES ${DEMO_CPP_SOURCES}
        ${_fx3_opts_cpp}
        MAP_FILE
        LTO
)

# 包含目录
target_include_directories(demo_cpp PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 编译选项
target_compile_options(demo_cpp PRIVATE
        ${COMMON_COMPILE_FLAGS}
        $<$<COMPILE_LANGUAGE:C>:${C_SPECIFIC_FLAGS}>
        $<$<COMPILE_LANGUAGE:CXX>:${CXX_SPECIFIC_FLAGS}>
)

# 编译定义
target_compile_definitions(demo_cpp PRIVATE
        ${COMMON_COMPILE_DEFINITIONS}
)

# 构建信息
#message(STATUS "demo_cpp sources: ${DEMO_CPP_SOURCES}")
