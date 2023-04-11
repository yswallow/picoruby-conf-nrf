cmake_minimum_required(VERSION 3.24)

#####################################################
# project specific configuration from here
set(NRFSDK17_ROOT $ENV{NRFSDK17_ROOT})

set(CMAKE_VERSION_NUMBER 0.0.1)
set(CMAKE_REVISION asdfg)
execute_process (COMMAND date +%Y%m%d OUTPUT_VARIABLE CMAKE_BUILDDATE OUTPUT_STRIP_TRAILING_WHITESPACE)
#execute_process (COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE CMAKE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
#execute_process (COMMAND git describe --tags --abbrev=0 OUTPUT_VARIABLE CMAKE_VERSION_NUMBER OUTPUT_STRIP_TRAILING_WHITESPACE)
set (PRK_VERSION   ${CMAKE_VERSION_NUMBER})
set (PRK_BUILDDATE ${CMAKE_BUILDDATE})
set (PRK_REVISION  ${CMAKE_REVISION})
configure_file ("${CMAKE_SOURCE_DIR}/include/version.h.in" "${CMAKE_SOURCE_DIR}/include/version.h")

if(PRK_NO_MSC)
  project("nrf-${PRK_VERSION}-${PRK_BUILDDATE}-no_msc" LANGUAGES C ASM)
else()
  project("nrf-${PRK_VERSION}-${PRK_BUILDDATE}-${PRK_REVISION}" LANGUAGES C ASM)
endif()

set(CMAKE_CROSS_COMPILING 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_LINKER arm-none-eabi-ld)

add_definitions(
  -DNDEBUG
#  -DMRBC_USE_HAL_NRF52840
  -DMRBC_REQUIRE_32BIT_ALIGNMENT
  -DMAX_REGS_SIZE=256
  -DMAX_VM_COUNT=255
  -DMAX_SYMBOLS_COUNT=1000
  -DMRBC_CONVERT_CRLF
#  -DMRBC_USE_MATH
  -DMRBC_NO_TIMER
  -DPICORBC_PTR_SIZE=4
# for nRF
  -DCONFIG_GPIO_AS_PINRESET
  -DFLOAT_ABI_HARD
  -D__HEAP_SIZE=8192
  -D__STACK_SIZE=8192
  -DNRF52840_XXAA
  -mcpu=cortex-m4
  -mthumb
  -mabi=aapcs
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
  -ffunction-sections
  -fdata-sections
  -fno-strict-aliasing
  -fno-builtin
  -fshort-enums
  -Wall
  -std=c99
  -O3
  -g3
)
if(PRK_NO_MSC)
  add_definitions(-DPRK_NO_MSC)
endif()


add_link_options(
  -mthumb
  -mabi=aapcs
  -mcpu=cortex-m4
  -mfloat-abi=hard
  -mfpu=fpv4-sp-d16
  -Wl,--gc-sections
  -L ${NRFSDK17_ROOT}/modules/nrfx/mdk
  --specs=nano.specs
  -O3
  -g3
  -T ${CMAKE_SOURCE_DIR}/nrf/s140_v6.ld
)

file(GLOB SRCS   
   src/*.c
#  lib/picoruby/build/repos/nrf-cortex-m4/mruby-pico-compiler/src/*.c
)

add_executable(${PROJECT_NAME}
  ${SRCS}
  ${NRFSDK17_ROOT}/components/drivers_nrf/nrf_soc_nosd/nrf_nvic.c
  ${NRFSDK17_ROOT}/modules/nrfx/drivers/src/nrfx_timer.c
  ${NRFSDK17_ROOT}/modules/nrfx/mdk/gcc_startup_nrf52840.S
  ${NRFSDK17_ROOT}/modules/nrfx/mdk/system_nrf52840.c
)

set(PICORBC ${CMAKE_SOURCE_DIR}/lib/picoruby/bin/picorbc)
set(RBC ${CMAKE_CURRENT_SOURCE_DIR}/lib/picoruby/bin/picorbc)
#set(RBC RBENV_VERSION=mruby-3.1.0 mrbc) # In case of PicoRuby compiler enbugged 😵

# Ruby

if(PRK_NO_MSC)
  add_custom_target(keymap
    COMMAND ${RBC} -Bkeymap -o ./keymap.c ../keymap.rb
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
  )
  target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_BINARY_DIR}
  )
endif()

add_custom_target(mrbdir ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/build/mrb
)

add_dependencies(${PROJECT_NAME}
  mrbdir
)

if(PRK_NO_MSC)
  add_dependencies(${PROJECT_NAME}
    keymap
  )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/picoruby/include
  ${CMAKE_SOURCE_DIR}/lib/picoruby/include/hal_no_impl
  ${CMAKE_SOURCE_DIR}/lib/picoruby/mrbgems/picoruby-mrubyc/repos/mrubyc/src
  ${CMAKE_SOURCE_DIR}/lib/picoruby/build/nrf-cortex-m4/mrbgems
  ${CMAKE_SOURCE_DIR}/lib/picoruby/build/repos/nrf-cortex-m4/mruby-pico-compiler/include
  ${CMAKE_SOURCE_DIR}/lib/picoruby/mrbgems
  ${NRFSDK17_ROOT}/modules/nrfx
  ${NRFSDK17_ROOT}/modules/nrfx/hal
  ${NRFSDK17_ROOT}/modules/nrfx/mdk
  ${NRFSDK17_ROOT}/modules/nrfx/drivers/include
  ${NRFSDK17_ROOT}/integration/nrfx
  ${NRFSDK17_ROOT}/components/libraries/delay
  ${NRFSDK17_ROOT}/components/libraries/util
  ${NRFSDK17_ROOT}/components/libraries/log
  ${NRFSDK17_ROOT}/components/libraries/log/src
  ${NRFSDK17_ROOT}/components/libraries/experimental_section_vars
  ${NRFSDK17_ROOT}/components/toolchain/cmsis/include
  ${NRFSDK17_ROOT}/components/toolchain/cmsis/dsp/Include
  ${NRFSDK17_ROOT}/components/drivers_nrf/nrf_soc_nosd
  ${CMAKE_SOURCE_DIR}/nrf
)

target_link_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/lib/picoruby/build/nrf-cortex-m4/lib
  #${NRFSDK17_ROOT}/components/toolchain/cmsis/dsp/GCC
)

target_link_libraries(${PROJECT_NAME}
    #libarm_cortexM4lf_math.a
    mruby
)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
    COMMAND arm-none-eabi-objcopy -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
    COMMAND python ~/prog/uf2/utils/uf2conv.py -f 0xADA52840 -c ${PROJECT_NAME}.hex -o ${PROJECT_NAME}.uf2
)
