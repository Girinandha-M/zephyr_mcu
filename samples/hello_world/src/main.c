/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <ti/driverlib/dl_crc.h>
#include <ti/driverlib/dl_flashctl.h>
#include <zephyr/storage/flash_map.h>
#include <ti/devices/msp/m0p/mspm0g350x.h>
#include "g_bcrbsl.h"

#define CRC_SEED                                                    (0xFFFFFFFF)

#define CRC	(CRC_Regs *)CRC_BASE
#define FLASH	(FLASHCTL_Regs *)FLASHCTL_BASE

/* clang-format off */

/* Base address of nonmain memory */
#define NONMAIN_BASE_ADDRESS                                      (0x41C00000U)

/* Base address of the BCR configuration structure in nonmain memory */
#define BCR_USER_CFG_BASE_ADDRESS                                 (0x41C00000U)

/* Size in bytes of the BSL and BCR configuration structures */
#define BCR_CONFIG_SIZE_BYTES                                             (96U)

/* The calculated CRC based on the default configuration values */
#define BCR_CFG_DEFAULT_CRC                                        (0x1879dac3)

/* clang-format on */
#define MCUBOOT_START	FIXED_PARTITION_OFFSET(boot_partition)
#define MCUBOOT_LEN	FIXED_PARTITION_SIZE(boot_partition)

#define BCR_SIZE	sizeof(BCR_Config)

/* The default configuration of the BCR config struct */
BCR_Config BCRConf = {
	.bcrConfigID                   = 0x1,
	.debugAccess                   = BCR_CFG_DEBUG_ACCESS_EN,
	.swdpMode                      = BCR_CFG_SWDP_EN,
	.tifaMode                      = BCR_CFG_TIFA_EN,
	.bslPinInvokeEnable            = BCR_CFG_BSL_PIN_INVOKE_EN,
	.passwordDebugLock             = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.fastBootMode                  = BCR_CFG_FAST_BOOT_DIS,
	.bootloaderMode                = BCR_CFG_BOOTLOADER_MODE_EN,
	.massEraseMode                 = BCR_CFG_MASS_ERASE_EN,
	.factoryResetMode              = BCR_CFG_FACTORY_RESET_EN,
	.passwordMassErase             = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.passwordFactoryReset          = {CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE,
		CFG_DEFAULT_VALUE, CFG_DEFAULT_VALUE},
	.staticWriteProtectionMainLow  = CFG_DEFAULT_VALUE,
	.staticWriteProtectionMainHigh = CFG_DEFAULT_VALUE,
	.staticWriteProtectionNonMain  = BCR_CFG_NON_MAIN_STATIC_PROT_DIS,
	.secureBootMode                = BCR_CFG_SECURE_BOOT_DIS,
	.userSecureAppStartAddr        = CFG_DEFAULT_VALUE,
	.userSecureAppLength           = CFG_DEFAULT_VALUE,
	.userSecureAppCrc              = CFG_DEFAULT_VALUE,
	.userCfgCRC                    = BCR_CFG_DEFAULT_CRC,
};


uint32_t calcUserConfigCRC(uint8_t *dataPointer, uint32_t size)
{
	uint32_t i;
	uint32_t calculatedCRC;

	DL_CRC_init(CRC, DL_CRC_32_POLYNOMIAL, DL_CRC_BIT_REVERSED,
		    DL_CRC_INPUT_ENDIANESS_LITTLE_ENDIAN, DL_CRC_OUTPUT_BYTESWAP_DISABLED);

	/* Set the Seed value to reset the calculation */
	DL_CRC_setSeed32(CRC, CRC_SEED);

	for (i = (uint32_t) 0U; i < size; i++) {
		DL_CRC_feedData8(CRC, dataPointer[i]);
	}
	calculatedCRC = DL_CRC_getResult32(CRC);

	return calculatedCRC;
}

int main(void)
{
	DL_CRC_enablePower(CRC);

	/* MCUBOOT partition start address */
	BCRConf.userSecureAppStartAddr = MCUBOOT_START;
	/* MCUBOOT partition size */
	BCRConf.userSecureAppLength = MCUBOOT_LEN;
	/* Enable CRC check from BCR code */
	BCRConf.secureBootMode = BCR_CFG_SECURE_BOOT_EN;

	/* main flash is write protected for first 32kb with 0x00000000 */
	BCRConf.staticWriteProtectionMainLow = 0x00000000;

	BCRConf.userSecureAppCrc = calcUserConfigCRC((uint8_t *)MCUBOOT_START,
						     MCUBOOT_LEN);

	/* calculate the nonmain bcr config crc value */
	BCRConf.userCfgCRC = calcUserConfigCRC((uint8_t *) &BCRConf, sizeof(BCRConf)-
					       ((uint32_t) sizeof(BCRConf.userCfgCRC)));

	DL_FlashCTL_executeClearStatus(FLASH);

	/* Unprotect flash nonmain region */
	DL_FlashCTL_unprotectNonMainMemory(FLASH);

	DL_FlashCTL_eraseMemoryFromRAM(FLASH, NONMAIN_BASE_ADDRESS,
				       DL_FLASHCTL_COMMAND_SIZE_SECTOR);

	/* Write the bcr secure policies to the nonmain bcr region */
	DL_FlashCTL_programMemoryBlockingFromRAM64WithECCGenerated(FLASH, BCR_USER_CFG_BASE_ADDRESS,
								   (uint32_t *)&BCRConf, (BCR_CONFIG_SIZE_BYTES / 4),
								   DL_FLASHCTL_REGION_SELECT_NONMAIN);
	/* Protect flash nonmain region */
	DL_FlashCTL_protectNonMainMemory(FLASH);

	printf("\nBCR config completed");

	return 0;
}
