# CMake script to generate kernel_sectors.inc
file(READ "${CMAKE_BINARY_DIR}/bin/kernel.bin" KERNEL_BIN HEX)
string(LENGTH "${KERNEL_BIN}" KERNEL_HEX_LEN)
math(EXPR KERNEL_SIZE "${KERNEL_HEX_LEN} / 2")
math(EXPR KERNEL_SECTORS "(${KERNEL_SIZE} + 511) / 512")
file(WRITE "${CMAKE_BINARY_DIR}/bin/kernel_sectors.inc" "KERNEL_SECTORS equ ${KERNEL_SECTORS}\n")
message(STATUS "Kernel size: ${KERNEL_SIZE} bytes (${KERNEL_SECTORS} sectors)")
