/*
 * (C) 2005-2019 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT3620 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "printf.h"
#include "mt3620.h"
#include "os_hal_uart.h"
#include "os_hal_gpt.h"
#include "os_hal_gpio.h"

#include "mt3620-intercore.h"
#include "ctype.h"

/******************************************************************************/
/* Configurations */
/******************************************************************************/
/* UART */
static const uint8_t uart_port_num = OS_HAL_UART_ISU0;

/* GPT */
/* GPT0 for LED blinking */
static const uint8_t gpt_timer_led = OS_HAL_GPT0;
/* GPE3 for Button status scan */
static const uint8_t gpt_timer_button = OS_HAL_GPT3;
/* 500ms, GPT0 clock speed: 1KHz or 32KHz */
static const uint32_t gpt_timer_led_blinking_perios_ms = 500;
/* 100ms, GPT3 clock speed: 1MHz */
static const uint32_t gpt_timer_button_scan_perios_ms = 100000;

/* GPIO */
/* GPIO_8 for LED_Red Control */
static const uint8_t gpio_led_red = OS_HAL_GPIO_8;
/* GPIO_9 for LED_Green Control */
static const uint8_t gpio_led_green = OS_HAL_GPIO_9;
/* GPIO_10 for LED_Blue Control */
static const uint8_t gpio_led_blue = OS_HAL_GPIO_10;
/* GPIO_12 for Button_A Status Sensing */
static const uint8_t gpio_button_a = OS_HAL_GPIO_12;
/* GPIO_13 for Button_B Status Sensing */
static const uint8_t gpio_button_b = OS_HAL_GPIO_13;

static int colorcode = 0;
typedef unsigned char BYTE;

/******************************************************************************/
/* Applicaiton Hooks */
/******************************************************************************/
/* Hook for "printf". */
void _putchar(char character)
{
	mtk_os_hal_uart_put_char(uart_port_num, character);
	if (character == '\n')
		mtk_os_hal_uart_put_char(uart_port_num, '\r');
}

/******************************************************************************/
/* Functions */
/******************************************************************************/
static int gpio_output(u8 gpio_no, u8 level)
{
	int ret;

	ret = mtk_os_hal_gpio_request(gpio_no);
	if (ret != 0) {
		printf("request gpio[%d] fail\n", gpio_no);
		return ret;
	}

	mtk_os_hal_gpio_set_direction(gpio_no, OS_HAL_GPIO_DIR_OUTPUT);
	mtk_os_hal_gpio_set_output(gpio_no, level);
	ret = mtk_os_hal_gpio_free(gpio_no);
	if (ret != 0) {
		printf("free gpio[%d] fail\n", gpio_no);
		return 0;
	}
	return 0;
}


static void TimerHandlerGpt0(void *cb_data)
{
	static bool ledOn = true;
	uint8_t i;

	/* Toggle LED Status */
	ledOn = !ledOn;

	gpio_output(gpio_led_green, true);
	gpio_output(gpio_led_red, true);
	gpio_output(gpio_led_blue, true);

	if (colorcode == 0)
		gpio_output(gpio_led_red, ledOn);
	else if (colorcode == 1)
		gpio_output(gpio_led_blue, ledOn);
	else if (colorcode == 2)
		gpio_output(gpio_led_green, ledOn);
	else
		printf("wrong color code\0\r\n");

	/* Toggle ISU1~ISU2 GPIO(31~40) */
	/*     ISU1: GPIO31~35 */
	/*     ISU2: GPIO36~40 */
	for (i = OS_HAL_GPIO_31 ; i <= OS_HAL_GPIO_40 ; i++)
		gpio_output(i, ledOn);
}



void string2ByteArray(char* input, BYTE* output)
{
	int loop;
	int i;

	loop = 0;
	i = 0;

	while (input[loop] != '\0')
	{
		output[i++] = input[loop++];
	}
}

_Noreturn void RTCoreMain(void)
{
	struct os_gpt_int gpt0_int;
	struct os_gpt_int gpt3_int;

	char* color[3] = { "Red", "Blue", "Green" };
	char* pch;

	/* Init Vector Table */
	NVIC_SetupVectorTable();

	/* Init UART */
	mtk_os_hal_uart_ctlr_init(uart_port_num);
	printf("\nUART Inited (port_num=%d)\n", uart_port_num);

	/* Init GPT */
	mtk_os_hal_gpt_init();

	printf("--------------------------------\r\n");
	printf("IntercoreComms_RTApp_MT3620_BareMetal\r\n");
	printf("App built on: " __DATE__ ", " __TIME__ "\r\n");

	BufferHeader* outbound, * inbound;
	uint32_t sharedBufSize = 0;
	if (GetIntercoreBuffers(&outbound, &inbound, &sharedBufSize) == -1) {
		for (;;) {
			// empty.
		}
	}

	static const size_t payloadStart = 20;

	for (;;) {
		uint8_t buf[256];
		uint32_t dataSize = sizeof(buf);

		// On success, dataSize is set to the actual number of bytes which were read.
		int r = DequeueData(outbound, inbound, sharedBufSize, buf, &dataSize);
		int msgsize = dataSize - payloadStart;
		char msg[msgsize];

		if (r == -1 || dataSize < payloadStart) {
			continue;
		}

		printf("Received message of ");
		printf("%ld", dataSize);
		printf(" bytes:\r\n");

		// Print message as text.
		printf("  Payload (");
		printf(" bytes as text): ");
		for (size_t i = payloadStart; i < dataSize; ++i) {
			char c[2];
			c[0] = isprint(buf[i]) ? buf[i] : '.';
			c[1] = '\0';
			msg[i - payloadStart] = c[0];
			printf(c);
		}
		printf("\r\n");

		for (int i = 0; i < sizeof(color) / sizeof(int); i++)
		{
			if (strncmp(msg, color[i], strlen(color[i])) == 0)
			{

				colorcode = i;
				///* Init GPT0 for LED blinking, repeat mode. */
				gpt0_int.gpt_cb_hdl = TimerHandlerGpt0;
				gpt0_int.gpt_cb_data = NULL;
				mtk_os_hal_gpt_config(gpt_timer_led, false, &gpt0_int);
				mtk_os_hal_gpt_reset_timer(gpt_timer_led,
					gpt_timer_led_blinking_perios_ms, true);
				mtk_os_hal_gpt_start(gpt_timer_led);
			}
		}

		// Transform the payload by converting upper-case text to lower-case and vice versa,
		// and send the payload back to the sender.
		for (size_t i = payloadStart; i < dataSize; ++i) {
			// This must be an unsigned char, rather than a char, else a compile-time warning
			// is triggered by __ctype_lookup in ctype.h.
			unsigned char c = buf[i];
			if (isupper(c)) {
				c = tolower(c);
			}
			else if (islower(c)) {
				c = toupper(c);
			}

			buf[i] = c;
		}
		
		char cust_msg[] = " : MSFT"; //customized your RT message here 
		
		BYTE arr[strlen(cust_msg)];
		string2ByteArray(cust_msg, arr);
		
		for (int i = 0; i < strlen(cust_msg); i++)
		{
			buf[dataSize + i] = arr[i];
		}

		dataSize = dataSize + strlen(cust_msg);
	

		EnqueueData(inbound, outbound, sharedBufSize, buf, dataSize);
	}


	for (;;)
		__asm__("wfi");
}

