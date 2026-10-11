# CMake script to create disk image
file(READ "${CMAKE_BINARY_DIR}/bin/stage1.bin" STAGE1_BIN)
file(READ "${CMAKE_BINARY_DIR}/bin/stage2.bin" STAGE2_BIN)
file(READ "${CMAKE_BINARY_DIR}/bin/kernel.bin" KERNEL_BIN)

set(sectorSize 512)
set(imageSectors 2880)
set(stage2Lba 2)
set(stage2Capacity 8192)
set(kernelLba 18)

if(NOT "${STAGE1_BIN}" STRLESS 512)
    math(STRING LENGTH "${STAGE1_BIN}" stage1Len)
    if(NOT stage1Len EQUAL 512)
        message(FATAL_ERROR "Stage 1 must be exactly 512 bytes; got ${stage1Len}.")
    endif()
endif()

# Pad stage2 to 8192 bytes (16 sectors)
math(STRING LENGTH "${STAGE2_BIN}" stage2Len)
math(EXPR stage2Pad "${stage2Capacity} - ${stage2Len}")
if(stage2Pad GREATER 0)
    string(REPEAT "\0" stage2Pad STAGE2_PADDING)
    set(STAGE2_PADDED "${STAGE2_BIN}${STAGE2_PADDING}")
else()
    set(STAGE2_PADDED "${STAGE2_BIN}")
endif()

# Concatenate: stage1 + stage2_padded + kernel
set(DISK_IMAGE "${STAGE1_BIN}${STAGE2_PADDED}${KERNEL_BIN}")
file(WRITE "${CMAKE_BINARY_DIR}/bin/aetherion.img" "${DISK_IMAGE}")
message(STATUS "Created aetherion.img")
