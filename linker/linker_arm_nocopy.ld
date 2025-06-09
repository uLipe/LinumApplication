/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#if defined(CONFIG_CODE_DATA_RELOCATION) && DT_NODE_EXISTS(DT_INST(0, st_stm32_qspi_nor))
/* On stm32 QSPI, external flash is mapped in XIP region at address given by the reg property. */

#define EXTMEM_NODE	DT_INST(0, st_stm32_qspi_nor)
#define EXTMEM_ADDR	DT_REG_ADDR(DT_INST(0, st_stm32_qspi_nor))
#define EXTMEM_SIZE	DT_REG_ADDR_BY_IDX(DT_INST(0, st_stm32_qspi_nor), 1)

MEMORY
{
	EXTMEM (rx) : ORIGIN = EXTMEM_ADDR, LENGTH = EXTMEM_SIZE
}

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
{
    /* Adiciona a seção .extmem_rodata_reloc ao layout de seções */
    .extmem_local :
    {
        *(.extmem_local) /* Coloca todos os objetos marcados com .extmem_rodata_reloc */
    } > EXTMEM
}

#endif