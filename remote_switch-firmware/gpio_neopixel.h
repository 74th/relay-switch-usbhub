#include <stdint.h>
#include "ch32v003fun.h"
#include <stdio.h>

static void neopixel_write(GPIO_TypeDef *gpio, uint16_t gpio_pin, uint8_t *data, uint32_t len)
{

	uint32_t h = gpio_pin;
	uint32_t l = gpio_pin << 16;

	for (int i = 0; i < len; i++)
	{
		uint8_t c = data[i];
		for (int j = 0; j < 8; j++)
		{
			if (c & 0x1)
			{
				gpio->BSHR = h;
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				gpio->BSHR = l;
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
			}
			else
			{
				gpio->BSHR = h;
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				gpio->BSHR = l;
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
				asm("c.nop");
			}
			c = c >> 1;
		}
	}
}
