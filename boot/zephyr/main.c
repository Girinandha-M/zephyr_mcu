/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Arm Limited
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 * Copyright (c) 2025 Aerlync Labs Inc.
 * Copyright (c) 2025 Siemens Mobility GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bootutil/bootutil_log.h"
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/boot_hooks.h"
#include "bootutil/fault_injection_hardening.h"
#include "bootutil/mcuboot_status.h"
#include "flash_map_backend/flash_map_backend.h"

#include <ti/driverlib/m0p/sysctl/dl_sysctl_mspm0l122x_l222x.h>

#include <arm_cleanup.h>

#define LOCKABLE_FLASH	0xffffffff

#define SLOT0_OFFSET	FIXED_PARTITION_OFFSET(slot0_partition)
#define SLOT0_SIZE	FIXED_PARTITION_SIZE(slot0_partition)

#define MCUBOOT_HDR	0x200

static void do_boot(struct boot_rsp *rsp)
{
	printk("\nStart main application !!");
	printk("\nImage start Offset : 0x%x\n", rsp->br_image_off);

	uint32_t *vec_tab = 0x0 + rsp->br_image_off + rsp->br_hdr->ih_hdr_size;

	__set_MSP((uint32_t)vec_tab);

	SCB->VTOR = (uint32_t)vec_tab;

	((void (*)(void))(*(vec_tab+1)))();
}

static void mcuboot_fail()
{
	printk("\nMcu boot failed");
	while(1);
}

int main(void)
{
	int boot_status;
	struct boot_rsp bootrsp;

	/* check if initdone is issued by csc or not */
	if (DL_SYSCTL_isINITDONEIssued()){

		/* check if the firewall protection is enabled by csc or not */
		if (!((DL_SYSCTL_getWriteProtectFirewallAddrRange() & LOCKABLE_FLASH ) ==
		      LOCKABLE_FLASH)){
			printk("\ncsc failed to enable security before initdone ");
			mcuboot_fail();
		}
		/* do a sanity check and boot the validated image */
		boot_status = boot_go(&bootrsp);
		if((boot_status == 0) && (IMAGE_MAGIC == bootrsp.br_hdr->ih_magic)) {
			do_boot(&bootrsp);
		}
	} else {
		boot_status = boot_go(&bootrsp);
		if((boot_status == 0) && (IMAGE_MAGIC == bootrsp.br_hdr->ih_magic)) {
			/* write protect the boot partiton */
			DL_SYSCTL_setWriteProtectFirewallAddrRange((uint32_t)LOCKABLE_FLASH);
			/* csc security enforced , issue initdone */
			DL_SYSCTL_issueINITDONE();
		}
		mcuboot_fail();
	}
}
