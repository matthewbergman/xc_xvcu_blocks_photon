add_library(xvcu_blocks_photon STATIC
	${SOURCE_DIR}/blocks/kreisel_bms/src/kreisel_bms.c
	${SOURCE_DIR}/blocks/kreisel_bms/src/can/ke20_bms.c

	${SOURCE_DIR}/blocks/abs_bms/src/abs_bms.c
	${SOURCE_DIR}/blocks/abs_bms/src/can/abs_bms_nf.c

	${SOURCE_DIR}/blocks/dometic_helm/src/dometic_helm.c
	${SOURCE_DIR}/blocks/dometic_helm/src/can/dom.c

	${SOURCE_DIR}/utils/counter.c
)