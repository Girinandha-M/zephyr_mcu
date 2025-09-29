/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <ti/driverlib/dl_crc.h>
#include <ti/driverlib/dl_flashctl.h>
#include <zephyr/storage/flash_map.h>
#include "l_bcrbsl.h"

#include <ti/driverlib/dl_crcp.h>

#define CRCP0	(CRCP_Regs *)CRCP0_BASE
#define FLASHCTL	(FLASHCTL_Regs *)FLASHCTL_BASE

#define BOOT_START	0x00000000

/* Base address of nonmain memory */
#define NONMAIN_BASE_ADDRESS                                      (0x41C00000U)

/* Base address of the BCR configuration structure in nonmain memory */
#define BCR_USER_CFG_BASE_ADDRESS                                 (0x41C00000U)

/* Base address of the BSL configuration structure in nonmain memory */
#define BSL_USER_CFG_BASE_ADDRESS                                 (0x41C00100U)

/* Size in bytes of the BSL and BCR configuration structures */
#define BCR_CONFIG_SIZE_BYTES                                            (184U)
#define BSL_CONFIG_SIZE_BYTES                                             (88U)

/* The calculated CRC based on the default configuration values */
#define BCR_CFG_DEFAULT_CRC                                        (0x3f6920b2)
#define BSL_CFG_DEFAULT_CRC                                        (0x6f014bd1)

#define MCUBOOT_START	FIXED_PARTITION_OFFSET(boot_partition)
#define MCUBOOT_LEN	FIXED_PARTITION_SIZE(boot_partition)

/* clang-format on */

/* The default configuration of the BCR config struct */
BCR_Config BCRConf = {
	.bcrConfigID                   = 0x1000002,
	.debugAccess                   = BCR_CFG_DEBUG_ACCESS_EN,
	.swdpMode                      = BCR_CFG_SWDP_EN,
	.tifaMode                      = BCR_CFG_TIFA_EN,
	.bslPinInvokeEnable            = BCR_CFG_BSL_PIN_INVOKE_EN,
	.staticWriteProtectionMainLow  = CFG_DEFAULT_VALUE,
	.staticWriteProtectionMainHigh = CFG_DEFAULT_VALUE,
	.staticWriteProtectionNonMain  = BCR_CFG_NON_MAIN_STATIC_PROT_DIS,
	.debugHold                     = BCR_CFG_DEBUG_HOLD_DIS,
	.CSCexist                      = BCR_CFG_CSC_NOT_EXIST,
	.flashBankSwapPolicy           = BCR_CFG_FLASH_BS_DIS,
	.fastBootMode                  = BCR_CFG_FAST_BOOT_DIS,
	.bootloaderMode                = BCR_CFG_BOOTLOADER_MODE_EN,
	.massEraseMode                 = BCR_CFG_MASS_ERASE_EN,
	.factoryResetMode              = BCR_CFG_FACTORY_RESET_EN,
	.passwordMassErase             = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.passwordFactoryReset          = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.passwordDebugLock             = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.bc_reserved_0                 = 0xFFFF,
	.secureBootMode                = BCR_CFG_SECURE_BOOT_DIS,
	.userSecureAppStartAddr        = CFG_DEFAULT_VALUE,
	.userSecureAppLength           = CFG_DEFAULT_VALUE,
	.userSecureAppHash             = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.bc_reserved_1                 = 0xFFFFFFFFU,
	.userCfgCRC                    = BCR_CFG_DEFAULT_CRC,
};

uint32_t calcUserConfigCRC(uint8_t *dataPointer, uint32_t size)
{
	static uint32_t i;

	uint32_t calculatedCRC;

	DL_CRCP_init(CRCP0, DL_CRCP_POLYNOMIAL_SIZE_32, DL_CRCP_BIT_REVERSED,
		     DL_CRCP_INPUT_ENDIANESS_LITTLE_ENDIAN,
		     DL_CRCP_OUTPUT_BYTESWAP_DISABLED);

	/* Set the Seed value to reset the calculation */
	DL_CRCP_setSeed32(CRCP0, CRCP_SEED);

	for (i = (uint32_t) 0U; i < size; i++) {
		DL_CRCP_feedData8(CRCP0, dataPointer[i]);
	}
	calculatedCRC = DL_CRCP_getResult32(CRCP0);

	return calculatedCRC;
}

int main(void)
{
	DL_CRCP_enablePower(CRCP0);

	/* mark as csc exist to stop initdone issue after BCR */
	BCRConf.CSCexist = BCR_CFG_CSC_EXIST;

	/* unprotect NONMAIN, and then erase NONMAIN memory */
	DL_FlashCTL_executeClearStatus(FLASHCTL);
	DL_FlashCTL_unprotectSector(FLASHCTL, NONMAIN_BASE_ADDRESS, DL_FLASHCTL_REGION_SELECT_NONMAIN);
	DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, NONMAIN_BASE_ADDRESS, DL_FLASHCTL_COMMAND_SIZE_SECTOR);

	DL_FlashCTL_programMemoryBlockingFromRAM64WithECCGenerated(FLASHCTL,
								   BCR_USER_CFG_BASE_ADDRESS, (uint32_t *) &BCRConf,
								   (BCR_CONFIG_SIZE_BYTES / 4), DL_FLASHCTL_REGION_SELECT_NONMAIN);
	/* Protect flash nonmain region */
	DL_FlashCTL_protectNonMainMemory(FLASHCTL);

	printf("\nBcr config completed");

	return 0;
}
