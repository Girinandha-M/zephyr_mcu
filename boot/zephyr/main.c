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
#include "bootutil/bootutil.h"

static void start_app(uint32_t *vec_tab)
{
	__set_MSP((uint32_t)vec_tab);

	SCB->VTOR = (uint32_t)vec_tab;

	((void (*)(void))(*(vec_tab+1)))();
}

static void do_boot(struct boot_rsp *rsp)
{
	uint32_t *vec_tab = 0x0 + rsp->br_image_off + rsp->br_hdr->ih_hdr_size;

	printk("\nStart main application !!");
	printk("\nImage start Offset : 0x%x\n", rsp->br_image_off);

	start_app(vec_tab);
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

	boot_status = boot_go(&bootrsp);

	if((boot_status == 0) && (IMAGE_MAGIC == bootrsp.br_hdr->ih_magic)) {
		do_boot(&bootrsp);
	}
	mcuboot_fail();
}
