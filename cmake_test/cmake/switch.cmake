# Find DEVKITPRO
if(NOT DEFINED ENV{DEVKITPRO})
	message(FATAL_ERROR "You must have defined DEVKITPRO before calling cmake.")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})

set(ARCH "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec")
set(CMAKE_C_FLAGS          "${ARCH} -O2 -ffunction-sections -fdata-sections -Wall")
set(CMAKE_CXX_FLAGS        "${CMAKE_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "-specs=${DEVKITPRO}/libnx/switch.specs")
set(CMAKE_ASM_FLAGS        "${CMAKE_C_FLAGS}" )


find_program(ELF2NRO elf2nro ${DEVKITPRO}/tools/bin)
if (ELF2NRO)
	message(STATUS "elf2nro: ${ELF2NRO} - found")
else ()
    message(WARNING "elf2nro - not found")
endif ()

find_program(NACPTOOL nacptool ${DEVKITPRO}/tools/bin)
if (NACPTOOL)
    message(STATUS "nacptool: ${NACPTOOL} - found")
else ()
    message(WARNING "nacptool - not found")
endif ()

function(__add_nacp target APP_TITLE APP_AUTHOR APP_VERSION)
    set(__NACP_COMMAND ${NACPTOOL} --create ${APP_TITLE} ${APP_AUTHOR} ${APP_VERSION} ${CMAKE_CURRENT_BINARY_DIR}/${target})

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}
            COMMAND ${__NACP_COMMAND}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            VERBATIM
            )
endfunction()

function(add_nro_target target title author version icon romfs)
    get_filename_component(target_we ${target} NAME_WE)
        if (NOT ${target_we}.nacp)
            __add_nacp(${target_we}.nacp ${title} ${author} ${version})
        endif ()
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro
            COMMAND ${ELF2NRO} $<TARGET_FILE:${target}>
				${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro
				--icon=${icon}
				--nacp=${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nacp
				--romfsdir=${romfs}
            DEPENDS ${target} ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nacp
            VERBATIM
        )
        add_custom_target(${target_we}_nro ALL SOURCES ${CMAKE_CURRENT_BINARY_DIR}/${target_we}.nro)
endfunction()

message(STATUS "CMAKE_C_FLAGS=${CMAKE_C_FLAGS}")
message(STATUS "CMAKE_CPP_FLAGS=${CMAKE_CPP_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")

