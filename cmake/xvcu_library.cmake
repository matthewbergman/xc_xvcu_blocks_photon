add_library(xvcu_blocks_photon STATIC
	${SOURCE_DIR}/blocks/kreisel_bms/src/kreisel_bms.c
	${SOURCE_DIR}/blocks/kreisel_bms/src/can/ke20_bms.c

	${SOURCE_DIR}/utils/counter.c
)