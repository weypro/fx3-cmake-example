# =============================================================================
# fx3-toolchain.cmake
# =============================================================================

cmake_minimum_required(VERSION 3.16)

# Prevent duplicate inclusion
if(DEFINED _FX3_TOOLCHAIN_LOADED)
    return()
endif()
set(_FX3_TOOLCHAIN_LOADED TRUE)

# -----------------------------------------------------------------------------
# Basic cross-compilation configuration
# -----------------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
# Bare metal scenario, try_compile only generates static libraries to avoid executable detection failure
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# -----------------------------------------------------------------------------
# Make tool configuration options
# -----------------------------------------------------------------------------
# Whether to use SDK's built-in cs-make on Windows
if(WIN32)
    option(FX3_USE_SDK_MAKE "Force set MAKE command to the cs-make on Windows" OFF)
endif()

# SDK library configuration options
option(FX3_SDK_ENABLE_STDC "Enable standard C library support in fx3_sdk" ON)
option(FX3_SDK_ENABLE_STDCXX "Enable standard C++ library support in fx3_sdk_cpp" ON)

# -----------------------------------------------------------------------------
# Paths and toolchain prefix
# -----------------------------------------------------------------------------
# ARM GCC
if(NOT DEFINED ARMGCC_INSTALL_PATH)
    set(ARMGCC_INSTALL_PATH $ENV{ARMGCC_INSTALL_PATH})
endif()
if(NOT ARMGCC_INSTALL_PATH)
    message(FATAL_ERROR "ARMGCC_INSTALL_PATH is not set. Please specify the ARM GCC toolchain root.")
endif()

# FX3/CX3 SDK installation root
if(NOT DEFINED FX3_INSTALL_PATH)
    set(FX3_INSTALL_PATH $ENV{FX3_INSTALL_PATH})
endif()
if(NOT FX3_INSTALL_PATH)
    message(FATAL_ERROR "FX3_INSTALL_PATH is not set. Please specify the Cypress/Infineon FX3 SDK root.")
endif()
file(TO_CMAKE_PATH "${FX3_INSTALL_PATH}" FX3_INSTALL_PATH)
set(FX3_FIRMWARE_COMMON_ROOT "${FX3_INSTALL_PATH}/fw_build/fx3_fw")
set(FX3_FIRMWARE_ROOT "${FX3_INSTALL_PATH}/firmware")
set(FX3_PFWROOT   "${FX3_FIRMWARE_ROOT}/u3p_firmware")
set(FX3_INCLUDE_DIR "${FX3_PFWROOT}/inc")
set(FX3_LIB_DIR     "${FX3_PFWROOT}/lib")

# Toolchain prefix and suffix
set(TOOLCHAIN_PREFIX arm-none-eabi-)
if(WIN32)
    set(TOOL_SUFFIX ".exe")
else()
    set(TOOL_SUFFIX "")
endif()

# Directly set absolute paths of compiler and common tools (cached for user override)
set(CMAKE_C_COMPILER   "${ARMGCC_INSTALL_PATH}/bin/${TOOLCHAIN_PREFIX}gcc${TOOL_SUFFIX}"   CACHE FILEPATH "ARM GCC C compiler")
set(CMAKE_CXX_COMPILER "${ARMGCC_INSTALL_PATH}/bin/${TOOLCHAIN_PREFIX}g++${TOOL_SUFFIX}"   CACHE FILEPATH "ARM GCC CXX compiler")
set(CMAKE_ASM_COMPILER "${ARMGCC_INSTALL_PATH}/bin/${TOOLCHAIN_PREFIX}gcc${TOOL_SUFFIX}"   CACHE FILEPATH "ARM GCC ASM compiler")
set(CMAKE_OBJCOPY      "${ARMGCC_INSTALL_PATH}/bin/${TOOLCHAIN_PREFIX}objcopy${TOOL_SUFFIX}" CACHE FILEPATH "objcopy")
set(CMAKE_SIZE_UTIL    "${ARMGCC_INSTALL_PATH}/bin/${TOOLCHAIN_PREFIX}size${TOOL_SUFFIX}"    CACHE FILEPATH "size")

# Set Make program on Windows
if(WIN32 AND FX3_USE_SDK_MAKE)
    set(CMAKE_MAKE_PROGRAM "${ARMGCC_INSTALL_PATH}/bin/cs-make${TOOL_SUFFIX}"
            CACHE FILEPATH "The toolchain make command" FORCE)
endif()

# elf2img (SDK built-in)
if(WIN32)
    find_program(ELF2IMG_TOOL elf2img.exe PATHS "${FX3_INSTALL_PATH}/util/elf2img" REQUIRED)
else()
    find_program(ELF2IMG_TOOL elf2img PATHS "${FX3_INSTALL_PATH}/util/elf2img" "/usr/local/bin")
    if(NOT ELF2IMG_TOOL)
        message(WARNING "elf2img not found in SDK util path. Post-build image conversion will be skipped.")
    endif()
endif()

# -----------------------------------------------------------------------------
# Cross-compilation search paths to avoid using host libraries/headers
# -----------------------------------------------------------------------------
set(CMAKE_FIND_ROOT_PATH
        "${FX3_INSTALL_PATH}"
        "${FX3_FIRMWARE_ROOT}"
        "${ARMGCC_INSTALL_PATH}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# -----------------------------------------------------------------------------
# Default build type
# -----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build: Debug Release RelWithDebInfo MinSizeRel" )
endif()

# Choose SDK library subdirectory based on configuration (FX3: fx3_debug / fx3_release)
set(FX3_CONFIG_DIR $<IF:$<CONFIG:Debug>,fx3_debug,fx3_release>)

# -----------------------------------------------------------------------------
# Common compilation options (only for *_INIT initialization; target level will apply again to ensure constraints)
# -----------------------------------------------------------------------------
set(FX3_COMMON_FLAGS
        -mcpu=arm926ej-s
        -mthumb-interwork
        -ffunction-sections -fdata-sections
        -fmessage-length=0 -fshort-wchar
        -Wall -Wno-write-strings
        -Wno-unused-parameter
)
string(JOIN " " FX3_COMMON_FLAGS_STR ${FX3_COMMON_FLAGS})

# Use *_INIT variables for default initialization (won't forcibly override user cache)
set(CMAKE_C_FLAGS_INIT     "${FX3_COMMON_FLAGS_STR} -std=c11")
set(CMAKE_CXX_FLAGS_INIT   "${FX3_COMMON_FLAGS_STR} -std=c++11")
set(CMAKE_ASM_FLAGS_INIT   "${FX3_COMMON_FLAGS_STR} -x assembler-with-cpp")

set(CMAKE_C_FLAGS_DEBUG_INIT           "-O0 -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE_INIT         "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT  "-O2 -g -DNDEBUG")
set(CMAKE_C_FLAGS_MINSIZEREL_INIT      "-Os -DNDEBUG")

set(CMAKE_CXX_FLAGS_DEBUG_INIT           "${CMAKE_C_FLAGS_DEBUG_INIT}")
set(CMAKE_CXX_FLAGS_RELEASE_INIT         "${CMAKE_C_FLAGS_RELEASE_INIT}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT  "${CMAKE_C_FLAGS_RELWITHDEBINFO_INIT}")
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT      "${CMAKE_C_FLAGS_MINSIZEREL_INIT}")

# Disable default C/CXX link flags for shared library linking (not needed for bare metal)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS   "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# -----------------------------------------------------------------------------
# Library configuration
# -----------------------------------------------------------------------------
# FX3 core libraries (ThreadX, LPP, SPORT, API)
set(FX3_CORE_LIBS
        ${FX3_LIB_DIR}/${FX3_CONFIG_DIR}/cyu3sport.a
        ${FX3_LIB_DIR}/${FX3_CONFIG_DIR}/cyu3lpp.a
        ${FX3_LIB_DIR}/${FX3_CONFIG_DIR}/cyfxapi.a
        ${FX3_LIB_DIR}/${FX3_CONFIG_DIR}/cyu3threadx.a)

# Optional standard libraries
set(FX3_STDC_LIBS
        m
        c
        gcc)

set(FX3_STDCXX_LIBS
        stdc++)

# -----------------------------------------------------------------------------
# Default source collection
# -----------------------------------------------------------------------------
function(fx3_get_default_sources out_var enable_cxx)
    set(_startup "${FX3_FIRMWARE_COMMON_ROOT}/cyfx_gcc_startup.S")
    if(enable_cxx)
        set(_core
                ${FX3_FIRMWARE_COMMON_ROOT}/cyfxtx.cpp
                ${FX3_FIRMWARE_COMMON_ROOT}/cyfxcppsyscall.cpp)
    else()
        set(_core ${FX3_FIRMWARE_COMMON_ROOT}/cyfxtx.c)
    endif()
    set(${out_var} ${_startup} ${_core} PARENT_SCOPE)
endfunction()
function(fx3_add_firmware target_name)
    # Parameter definition
    set(_opts ENABLE_CXX ENABLE_STDC NO_STDCXX MAP_FILE LTO KEEP_VECTORLOAD)
    set(_singles LINKER_SCRIPT OUTPUT_DIRECTORY OUTPUT_IMG I2C_CONF)
    set(_multis SOURCES INCLUDE_DIRS DEFINES LIB_DIRS LIBS COMPILE_OPTIONS LINK_OPTIONS)

    # Parameter parsing and validation
    cmake_parse_arguments(FX3 "${_opts}" "${_singles}" "${_multis}" ${ARGN})
    if(FX3_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "fx3_add_firmware(): Unknown args: ${FX3_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT FX3_SOURCES)
        message(FATAL_ERROR "fx3_add_firmware(): please provide at least one source via SOURCES")
    endif()

    # Unified selection and validation of linker script
    if(NOT FX3_LINKER_SCRIPT)
        if(FX3_ENABLE_CXX)
            set(FX3_LINKER_SCRIPT "${FX3_FIRMWARE_COMMON_ROOT}/fx3cpp.ld")
        else()
            set(FX3_LINKER_SCRIPT "${FX3_FIRMWARE_COMMON_ROOT}/fx3.ld")
        endif()
    endif()
    if(NOT EXISTS "${FX3_LINKER_SCRIPT}")
        message(FATAL_ERROR "Linker script not found: ${FX3_LINKER_SCRIPT}")
    endif()

    # Get default sources
    fx3_get_default_sources(_core_src ${FX3_ENABLE_CXX})

    # Create target
    add_executable(${target_name} ${FX3_SOURCES} ${_core_src})
    set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "${target_name}.elf")
    if(FX3_OUTPUT_DIRECTORY)
        set_target_properties(${target_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${FX3_OUTPUT_DIRECTORY}")
    endif()

    # Compilation and link options collection
    set(_compile_opts)
    set(_link_opts)

    # LTO support
    if(FX3_LTO)
        list(APPEND _compile_opts -flto)
        list(APPEND _link_opts -flto)
    endif()

    # User-defined options
    if(FX3_COMPILE_OPTIONS)
        list(APPEND _compile_opts ${FX3_COMPILE_OPTIONS})
    endif()
    if(FX3_LINK_OPTIONS)
        list(APPEND _link_opts ${FX3_LINK_OPTIONS})
    endif()

    # MAP file generation
    if(FX3_MAP_FILE)
        list(APPEND _link_opts "LINKER:-Map=${target_name}.map")
    endif()

    # Apply compilation and link options
    if(_compile_opts)
        target_compile_options(${target_name} PRIVATE ${_compile_opts})
    endif()

    target_link_options(${target_name} PRIVATE
            "LINKER:--script=${FX3_LINKER_SCRIPT}"
            "LINKER:--gc-sections"
            "LINKER:--no-wchar-size-warning"
            "LINKER:--entry=CyU3PFirmwareEntry"
            -nostdlib -nostartfiles
            ${_link_opts}
    )

    # Libraries, headers and macro definitions
    # Choose SDK library version
    if(FX3_ENABLE_CXX)
        target_link_libraries(${target_name} PRIVATE fx3_sdk_cpp)
        set(_sdk_name "fx3_sdk_cpp")
    else()
        target_link_libraries(${target_name} PRIVATE fx3_sdk)
        set(_sdk_name "fx3_sdk")
    endif()

    # User additional libraries
    if(FX3_LIBS)
        target_link_libraries(${target_name} PRIVATE ${FX3_LIBS})
    endif()

    # User additional configuration
    if(FX3_INCLUDE_DIRS)
        target_include_directories(${target_name} PRIVATE ${FX3_INCLUDE_DIRS})
    endif()
    if(FX3_DEFINES)
        target_compile_definitions(${target_name} PRIVATE ${FX3_DEFINES})
    endif()
    if(FX3_LIB_DIRS)
        target_link_directories(${target_name} PRIVATE ${FX3_LIB_DIRS})
    endif()

    # Size statistics
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_SIZE_UTIL} --format=berkeley $<TARGET_FILE:${target_name}>
            COMMENT "Firmware size for ${target_name}:")

    # Convert to .img file
    if(ELF2IMG_TOOL)
        # Determine output path
        if(FX3_OUTPUT_IMG)
            set(_img_output "${FX3_OUTPUT_IMG}")
        else()
            set(_img_output "${target_name}.img")
        endif()

        # Build elf2img command arguments
        set(_elf2img_args -i $<TARGET_FILE:${target_name}> -o ${_img_output})
        if(FX3_KEEP_VECTORLOAD)
            list(APPEND _elf2img_args -vectorload yes)
        endif()
        if(FX3_I2C_CONF)
            list(APPEND _elf2img_args -i2cconf "${FX3_I2C_CONF}")
        endif()

        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${ELF2IMG_TOOL} ${_elf2img_args}
                BYPRODUCTS ${_img_output}
                COMMENT "Converting ${target_name}.elf to ${_img_output}")
    endif()

    # Unified status printing
    message(STATUS "[FX3] Target: ${target_name}")
    message(STATUS "[FX3] Linker: ${FX3_LINKER_SCRIPT}")
    message(STATUS "[FX3] C++: ${FX3_ENABLE_CXX}")
    message(STATUS "[FX3] STD C: ${FX3_ENABLE_STDC}")
    message(STATUS "[FX3] LTO: ${FX3_LTO}")
    message(STATUS "[FX3] Map: ${FX3_MAP_FILE}")
    message(STATUS "[FX3] SDK: ${_sdk_name}")
endfunction()

# -----------------------------------------------------------------------------
# SDK INTERFACE library definition
# -----------------------------------------------------------------------------

# Basic C version SDK library
add_library(fx3_sdk INTERFACE)

# Configure header file directories
target_include_directories(fx3_sdk INTERFACE
        ${FX3_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}  # Usually project root directory also needs to be included
)

# Configure compilation definitions
target_compile_definitions(fx3_sdk INTERFACE
        CYU3P_FX3=1
        __CYU3P_TX__=1
)

# Configure core library linking
target_link_libraries(fx3_sdk INTERFACE ${FX3_CORE_LIBS})

# Configure standard C library support based on options
if(FX3_SDK_ENABLE_STDC)
    target_link_libraries(fx3_sdk INTERFACE ${FX3_STDC_LIBS})
endif()

# C++ version SDK library
add_library(fx3_sdk_cpp INTERFACE)

# C++ library inherits all configurations from base library
target_link_libraries(fx3_sdk_cpp INTERFACE fx3_sdk)

# Configure standard C++ library support based on options
if(FX3_SDK_ENABLE_STDCXX)
    target_link_libraries(fx3_sdk_cpp INTERFACE ${FX3_STDCXX_LIBS} ${FX3_STDC_LIBS})
endif()

# Create namespaced aliases for easier debugging
add_library(FX3::SDK ALIAS fx3_sdk)
add_library(FX3::SDK_CPP ALIAS fx3_sdk_cpp)

# -----------------------------------------------------------------------------
# Final status printing
# -----------------------------------------------------------------------------
message(STATUS "FX3 Toolchain Configuration:")
message(STATUS "  ARM GCC:         ${ARMGCC_INSTALL_PATH}")
message(STATUS "  FX3 SDK:         ${FX3_INSTALL_PATH}")
message(STATUS "  C Compiler:      ${CMAKE_C_COMPILER}")
message(STATUS "  CXX Compiler:    ${CMAKE_CXX_COMPILER}")
message(STATUS "  Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "  ELF2IMG:         ${ELF2IMG_TOOL}")
message(STATUS "  Make Program:    ${CMAKE_MAKE_PROGRAM}")
message(STATUS "  SDK Libraries:")
message(STATUS "    fx3_sdk:       Standard C support=${FX3_SDK_ENABLE_STDC}")
message(STATUS "    fx3_sdk_cpp:   Standard C++ support=${FX3_SDK_ENABLE_STDCXX}")
if(WIN32 AND FX3_USE_SDK_MAKE)
    message(STATUS "  Note: Using SDK cs-make, recommend 'MinGW Makefiles' generator")
endif()