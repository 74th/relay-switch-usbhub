#include "ch32v003fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "gpio_neopixel.h"
#include "remote_switch.h"
#include <stdio.h>
#include <stdbool.h>

#define I2C_SLAVE_ADDRESS 0x74
#define LOOP_MS 10
#define LED_NUM 4
#define LED_BUF_LEN 3 * LED_NUM
#define LED_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 6)
#define BTN1_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 3)
#define BTN2_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 4)
#define BTN3_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 5)
#define BTN4_PIN GPIOv_from_PORT_PIN(GPIO_port_C, 7)

void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) __attribute__((interrupt));

void init_rcc(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    RCC->APB1PCENR |= RCC_APB1Periph_I2C1;
}

void init_i2c_slave(uint8_t address)
{
    // https://github.com/cnlohr/ch32v003fun/blob/master/examples/i2c_slave/i2c_slave.h

    // PC1 is SDA, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 1));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 1);

    // PC2 is SCL, 10MHz Output, alt func, open-drain
    GPIOC->CFGLR &= ~(0xf << (4 * 2));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF) << (4 * 2);

    // Reset I2C1 to init all regs
    RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

    I2C1->CTLR1 |= I2C_CTLR1_SWRST;
    I2C1->CTLR1 &= ~I2C_CTLR1_SWRST;

    // Set module clock frequency
    uint32_t prerate = 2000000; // I2C Logic clock rate, must be higher than the bus clock rate
    I2C1->CTLR2 |= (APB_CLOCK / prerate) & I2C_CTLR2_FREQ;

    // Enable interrupts
    I2C1->CTLR2 |= I2C_CTLR2_ITBUFEN;
    I2C1->CTLR2 |= I2C_CTLR2_ITEVTEN; // Event interrupt
    I2C1->CTLR2 |= I2C_CTLR2_ITERREN; // Error interrupt

    NVIC_EnableIRQ(I2C1_EV_IRQn); // Event interrupt
    NVIC_SetPriority(I2C1_EV_IRQn, 2 << 4);
    NVIC_EnableIRQ(I2C1_ER_IRQn); // Error interrupt
    // Set clock configuration
    uint32_t clockrate = 1000000;                                                    // I2C Bus clock rate, must be lower than the logic clock rate
    I2C1->CKCFGR = ((APB_CLOCK / (3 * clockrate)) & I2C_CKCFGR_CCR) | I2C_CKCFGR_FS; // Fast mode 33% duty cycle
    // I2C1->CKCFGR = ((APB_CLOCK/(25*clockrate))&I2C_CKCFGR_CCR) | I2C_CKCFGR_DUTY | I2C_CKCFGR_FS; // Fast mode 36% duty cycle
    // I2C1->CKCFGR = (APB_CLOCK/(2*clockrate))&I2C_CKCFGR_CCR; // Standard mode good to 100kHz

    // Set I2C address
    I2C1->OADDR1 = address << 1;

    // Enable I2C
    I2C1->CTLR1 |= I2C_CTLR1_PE;

    // Acknowledge the first address match event when it happens
    I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

int i2c_pos = 0;
uint8_t button_state_buf = 0x00;
uint8_t led_color_buf[LED_BUF_LEN];
bool need_write_led = false;

void I2C1_EV_IRQHandler(void)
{
    uint16_t STAR1, STAR2 __attribute__((unused));
    STAR1 = I2C1->STAR1;
    STAR2 = I2C1->STAR2;

    printf("EV STAR1: 0x%04x STAR2: 0x%04x\r\n", STAR1, STAR2);

    I2C1->CTLR1 |= I2C_CTLR1_ACK;

    if (STAR1 & I2C_STAR1_ADDR) // 0x0002
    {
        printf("ADDR\r\n");
        // 最初のイベント
        // read でも write でも必ず最初に呼ばれる
        i2c_pos = 0;
    }

    if (STAR1 & I2C_STAR1_RXNE) // 0x0040
    {
        printf("RXNE write event; pos:%d \r\n", i2c_pos);
        // 1byte 受信
        if (i2c_pos < LED_BUF_LEN)
        {
            led_color_buf[i2c_pos] = I2C1->DATAR;
        }
        else
        {
            I2C1->DATAR;
        }
        i2c_pos++;
        if (i2c_pos == LED_BUF_LEN)
        {
            need_write_led = true;
            printf("ready to write led\r\n");
        }
    }

    if (STAR1 & I2C_STAR1_TXE) // 0x0080
    {
        // 1byte の read イベント（slave -> master）
        printf("TXE write event:\r\n");
        // 1byte 送信
        I2C1->DATAR = button_state_buf;
    }
}

void I2C1_ER_IRQHandler(void)
{
    uint16_t STAR1 = I2C1->STAR1;

    printf("ER STAR1: 0x%04x\r\n", STAR1);

    if (STAR1 & I2C_STAR1_BERR)           // 0x0100
    {                                     // Bus error
        I2C1->STAR1 &= ~(I2C_STAR1_BERR); // Clear error
    }

    if (STAR1 & I2C_STAR1_ARLO)           // 0x0200
    {                                     // Arbitration lost error
        I2C1->STAR1 &= ~(I2C_STAR1_ARLO); // Clear error
    }

    if (STAR1 & I2C_STAR1_AF)           // 0x0400
    {                                   // Acknowledge failure
        I2C1->STAR1 &= ~(I2C_STAR1_AF); // Clear error
    }
}

void init_gpio()
{
    GPIO_port_enable(GPIO_port_C);
    GPIO_pinMode(LED_PIN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN1_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN2_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN3_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);
    GPIO_pinMode(BTN4_PIN, GPIO_pinMode_I_pullDown, GPIO_Speed_10MHz);

    GPIO_digitalWrite_0(LED_PIN);
}

void read_pushed()
{
    uint8_t new = 0x00;
    new = GPIO_digitalRead(BTN1_PIN);
    new |= GPIO_digitalRead(BTN2_PIN) << 1;
    new |= GPIO_digitalRead(BTN3_PIN) << 2;
    new |= GPIO_digitalRead(BTN4_PIN) << 3;
    button_state_buf = new;
}

void write_led()
{
    printf("write led\r\n");
    neopixel_write(GPIOC, GPIO_Pin_6, &led_color_buf, LED_BUF_LEN);
}

int main()
{
    SystemInit();
    init_rcc();

    printf("initialize\r\n");

    init_i2c_slave(I2C_SLAVE_ADDRESS);
    init_gpio();

    int current_usb_no = 1;

    for (int i = 0; i < LED_BUF_LEN; i++)
        led_color_buf[i] = 0xF0;

    write_led();

    printf("initialize done\r\n");

    Delay_Ms(100);

    while (1)
    {
        read_pushed();

        if (need_write_led)
        {
            write_led();
            need_write_led = false;
        }

        Delay_Ms(LOOP_MS);
    }
}
