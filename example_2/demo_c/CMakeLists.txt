# 源码收集
file(GLOB_RECURSE DEMO_C_SOURCES
        "*.c")
file(GLOB_RECURSE DEMO_C_HEADERS
        "*.h")

if(NOT DEMO_C_SOURCES)
    message(WARNING "[demo_c] No source files found in demo_c/")
endif()

# 设置FX3选项
set(_fx3_opts_c)
if(ENABLE_STDC)
    list(APPEND _fx3_opts_c ENABLE_STDC)
endif()
# 纯 C 目标不链接/启用 C++ 运行库
list(APPEND _fx3_opts_c NO_STDCXX)

# 创建固件目标
fx3_add_firmware(demo_c
        SOURCES ${DEMO_C_SOURCES}
        ${_fx3_opts_c}
        MAP_FILE
        LTO
)

# 包含目录
target_include_directories(demo_c PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

# 编译选项
target_compile_options(demo_c PRIVATE
        ${COMMON_COMPILE_FLAGS}
        $<$<COMPILE_LANGUAGE:C>:${C_SPECIFIC_FLAGS}>
        $<$<COMPILE_LANGUAGE:CXX>:${CXX_SPECIFIC_FLAGS}>
)

# 编译定义
target_compile_definitions(demo_c PRIVATE
        ${COMMON_COMPILE_DEFINITIONS}
)

# 构建信息
#message(STATUS "demo_c sources:   ${DEMO_C_SOURCES}")
