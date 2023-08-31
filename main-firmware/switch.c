#include <stdio.h>
#include <string.h>
#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "gpio_neopixel.h"
#include "switch.h"
#include "i2c_master.h"

#define I2C_ADDRESS_REMOTE_SWITCH 0x74
#define LOOP_MS 10
// #define LOOP_MS 500
#define LED_PIN GPIOv_from_PORT_PIN(GPIO_port_D, 4)
#define SELECT_U1_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 1)
#define SELECT_U2_PIN GPIOv_from_PORT_PIN(GPIO_port_A, 2)
#define SELECT_U3_PIN GPIOv_from_PORT_PIN(GPIO_port_D, 0)
#define SELECT_U4_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 0)
#define BTN1_PIN GPIOv_from_PORT_PIN(GPIO_port_D, 2)
#define BTN2_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 7)
#define BTN3_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 6)
#define BTN4_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 5)

void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt));

void init_rcc(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;
}

void init_gpio()
{
    GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);
    GPIO_pinMode(LED_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(SELECT_U1_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(SELECT_U2_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(SELECT_U3_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(SELECT_U4_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN1_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN2_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN3_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN4_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);

    GPIO_digitalWrite_0(LED_PIN);
}

uint8_t usb1_color[3] = {0x00, 0x00, 0xF0};
uint8_t usb2_color[3] = {0x00, 0xF0, 0x00};
uint8_t usb3_color[3] = {0xF0, 0x00, 0x00};
uint8_t usb4_color[3] = {0x00, 0xF0, 0xF0};

void select_usb(int no)
{
    switch (no)
    {
    case 1:
        GPIO_digitalWrite(SELECT_U1_PIN, high);
        GPIO_digitalWrite(SELECT_U2_PIN, low);
        GPIO_digitalWrite(SELECT_U3_PIN, low);
        GPIO_digitalWrite(SELECT_U4_PIN, low);
        neopixel_write(GPIOD, GPIO_Pin_4, &usb1_color, 3);
        break;
    case 2:
        GPIO_digitalWrite(SELECT_U1_PIN, low);
        GPIO_digitalWrite(SELECT_U2_PIN, high);
        GPIO_digitalWrite(SELECT_U3_PIN, low);
        GPIO_digitalWrite(SELECT_U4_PIN, low);
        neopixel_write(GPIOD, GPIO_Pin_4, &usb2_color, 3);
        break;
    case 3:
        GPIO_digitalWrite(SELECT_U1_PIN, low);
        GPIO_digitalWrite(SELECT_U2_PIN, low);
        GPIO_digitalWrite(SELECT_U3_PIN, high);
        GPIO_digitalWrite(SELECT_U4_PIN, low);
        neopixel_write(GPIOD, GPIO_Pin_4, &usb3_color, 3);
        break;
    case 4:
        GPIO_digitalWrite(SELECT_U1_PIN, low);
        GPIO_digitalWrite(SELECT_U2_PIN, low);
        GPIO_digitalWrite(SELECT_U3_PIN, low);
        GPIO_digitalWrite(SELECT_U4_PIN, high);
        neopixel_write(GPIOD, GPIO_Pin_4, &usb4_color, 3);
        break;
    }
}

int read_pushed()
{
    int pushed = 0;
    if (GPIO_digitalRead(BTN1_PIN) == high)
    {
        pushed = 1;
    }
    if (GPIO_digitalRead(BTN2_PIN) == high)
    {
        pushed = 2;
    }
    if (GPIO_digitalRead(BTN3_PIN) == high)
    {
        pushed = 3;
    }
    if (GPIO_digitalRead(BTN4_PIN) == high)
    {
        pushed = 4;
    }

    return pushed;
}

int read_remote_pushed()
{
    uint8_t data[1];
    if (i2c_receive(I2C_ADDRESS_REMOTE_SWITCH, data, 1) != 0)
    {
        return 0;
    }
    uint8_t switch_state = data[0];
    for (int i = 0; i < 4; i++)
    {
        if (switch_state & (1 << i))
        {
            return i + 1;
        }
    }
    return 0;
}

void flash_remote_usb(int no)
{
    uint8_t data[4 * 3];
    for (int i = 0; i < 4 * 3; i++)
    {
        data[i] = 0x00;
    }
    switch (no)
    {
    case 1:
        memcpy(&data[3 * 0], usb1_color, 3);
        break;
    case 2:
        memcpy(&data[3 * 1], usb2_color, 3);
        break;
    case 3:
        memcpy(&data[3 * 2], usb3_color, 3);
        break;
    case 4:
        memcpy(&data[3 * 3], usb4_color, 3);
        break;
    }
    i2c_send(I2C_ADDRESS_REMOTE_SWITCH, &data, 4 * 3);
}

int main()
{
    SystemInit();
    init_rcc();

    printf("initialize\r\n");

    init_gpio();
    init_i2c_master();

    int current_usb_no = 1;
    select_usb(1);

    printf("initialize done\r\n");

    Delay_Ms(100);

    printf("neopixel test start\r\n");

    select_usb(1);
    if (i2c_slave_available(I2C_ADDRESS_REMOTE_SWITCH))
    {
        flash_remote_usb(1);
    }

    printf("neopixel test done\r\n");

    while (1)
    {
        int remote_available = i2c_slave_available(I2C_ADDRESS_REMOTE_SWITCH);

        int pushed_no = read_pushed();
        if (remote_available)
        {
            int remote_pushed_no = read_remote_pushed();
            if (remote_pushed_no > 0)
            {
                pushed_no = remote_pushed_no;
            }
        }

        if (pushed_no > 0)
        {
            if (current_usb_no != pushed_no)
            {
                printf("pushed: %d\r\n", pushed_no);

                select_usb(pushed_no);
                if (remote_available)
                {
                    flash_remote_usb(pushed_no);
                }

                current_usb_no = pushed_no;
            }
        }

        Delay_Ms(LOOP_MS);
    }
}
