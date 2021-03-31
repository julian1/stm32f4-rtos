
// cjmcu  stm32f407.
// issue. is that board/stlink doesn't appear to reset cleanly. needs sleep.


#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

#include <libopencm3/stm32/exti.h>

#include <libopencm3/stm32/timer.h>

#include <libopencm3/stm32/spi.h>

// #include <libopencm3/cm3/scb.h>

#include <stddef.h> // size_t
//#include <math.h> // nanf
//#include <stdio.h>


#include "buffer.h"
#include "miniprintf2.h"
#include "usart2.h"
#include "util.h"

#include "winbond.h"




#define LED_PORT      GPIOE
#define LED_OUT       GPIO0



static void led_setup(void)
{
  gpio_mode_setup(LED_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_OUT);
}



////////////////////////////////////////////////////////

/* Common function descriptions */
// #include "clock.h"

/* milliseconds since boot */
static volatile uint32_t system_millis;



// static void systick_setup(void)
static void systick_setup(uint32_t tick_divider)
{
  // TODO change name systick_setup().
  // TODO pass clock reload divider as argument, to localize.

  /* clock rate / 168000 to get 1mS interrupt rate */
  // systick_set_reload(168000);
  systick_set_reload(tick_divider);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();

  /* this done last */
  systick_interrupt_enable();
}



void sys_tick_handler(void)
{
  /*
    this is an interupt context. shouldn't do any work in here
  */

  system_millis++;
}



////////////////////////////////////////////////////////

// implement critical_error_blink() msleep() and usart_printf()
// eg. rather than ioc/ dependency injection, just implement

void critical_error_blink(void)
{
  // avoid passing arguments, consuming stack.
	for (;;) {
		gpio_toggle(LED_PORT,LED_OUT);
		for(uint32_t i = 0; i < 500000; ++i)
       __asm__("nop");
	}
}

// defined in util.h
void msleep(uint32_t delay)
{
  // TODO. review. don't think this works on wrap around
  uint32_t wake = system_millis + delay;
  if(wake < delay) {

    // wait for overflow
    while (system_millis > (UINT32_MAX / 2));
  }
  while (system_millis < wake);
}




static char buf1[1000];
static char buf2[1000];

static CBuf console_in;
static CBuf console_out;




void usart_printf( const char *format, ... )
{
  // TODO rename to just printf... it's not the responsibiilty of user to know context
  // can override the write, to do flush etc.
	va_list args;

	va_start(args,format);
	internal_vprintf((void *)cBufPut, &console_out, format,args);
	va_end(args);
}

void flush( void)
{
  usart_sync_flush();
}



//////////////////////////

#define SPI_ICE40       SPI1

#define SPI_ICE40_PORT  GPIOA
#define SPI_ICE40_CLK   GPIO5
#define SPI_ICE40_CS    GPIO4
#define SPI_ICE40_MOSI  GPIO7
#define SPI_ICE40_MISO  GPIO6

// output reg.
#define SPI_ICE40_SPECIAL GPIO3




static void spi1_special_setup(void)
{
  // special
  gpio_mode_setup(SPI_ICE40_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SPI_ICE40_SPECIAL);
  gpio_set_output_options(SPI_ICE40_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, SPI_ICE40_SPECIAL);

  gpio_set(SPI_ICE40_PORT, SPI_ICE40_SPECIAL ); // hi == off, active low...
}


static void spi1_flash_setup(void)
{
  // same...
  uint16_t out = SPI_ICE40_CLK | SPI_ICE40_CS | SPI_ICE40_MOSI ; // not MISO
  uint16_t all = out | SPI_ICE40_MISO;

  // rcc_periph_clock_enable(RCC_SPI1);

  gpio_mode_setup(SPI_ICE40_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, all);
  gpio_set_af(SPI_ICE40_PORT, GPIO_AF5, all); // af 5
  gpio_set_output_options(SPI_ICE40_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, out);


  spi_init_master(
    SPI_ICE40,
    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    SPI_CR1_CPHA_CLK_TRANSITION_1,    // 1 == rising edge. difference
    SPI_CR1_DFF_8BIT,
    SPI_CR1_MSBFIRST
  );

  spi_disable_software_slave_management( SPI_ICE40);
  spi_enable_ss_output(SPI_ICE40);
}

// on flash chip.
// OK. there is nothing on miso.  it is lo. perhaps high impe.
// there is stuff on outputs cs, clk, mosi.

// may need to look at the iceprog - code. to see what it does.
// if not generic enough.

/*
	spi_enable(spi);
	spi_xfer(spi,W25_CMD_READ_SR1);
	sr1 = spi_xfer(spi,DUMMY);
	spi_disable(spi);
*/	


/* Flash command definitions */
/* This command list is based on the Winbond W25Q128JV Datasheet */
enum flash_cmd {
 FC_WE = 0x06, /* Write Enable */
 FC_SRWE = 0x50, /* Volatile SR Write Enable */
 FC_WD = 0x04, /* Write Disable */
 FC_RPD = 0xAB, /* Release Power-Down, returns Device ID */
 FC_MFGID = 0x90, /*  Read Manufacturer/Device ID */
 FC_JEDECID = 0x9F, /* Read JEDEC ID */
 FC_UID = 0x4B, /* Read Unique ID */
 FC_RD = 0x03, /* Read Data */
 FC_FR = 0x0B, /* Fast Read */
 FC_PP = 0x02, /* Page Program */
 FC_SE = 0x20, /* Sector Erase 4kb */
 FC_BE32 = 0x52, /* Block Erase 32kb */
 FC_BE64 = 0xD8, /* Block Erase 64kb */
 FC_CE = 0xC7, /* Chip Erase */
 FC_RSR1 = 0x05, /* Read Status Register 1 */
 FC_WSR1 = 0x01, /* Write Status Register 1 */
 FC_RSR2 = 0x35, /* Read Status Register 2 */
 FC_WSR2 = 0x31, /* Write Status Register 2 */
 FC_RSR3 = 0x15, /* Read Status Register 3 */
 FC_WSR3 = 0x11, /* Write Status Register 3 */
 FC_RSFDP = 0x5A, /* Read SFDP Register */
 FC_ESR = 0x44, /* Erase Security Register */
 FC_PSR = 0x42, /* Program Security Register */
 FC_RSR = 0x48, /* Read Security Register */
 FC_GBL = 0x7E, /* Global Block Lock */
 FC_GBU = 0x98, /* Global Block Unlock */
 FC_RBL = 0x3D, /* Read Block Lock */
 FC_RPR = 0x3C, /* Read Sector Protection Registers (adesto) */
 FC_IBL = 0x36, /* Individual Block Lock */
 FC_IBU = 0x39, /* Individual Block Unlock */
 FC_EPS = 0x75, /* Erase / Program Suspend */
 FC_EPR = 0x7A, /* Erase / Program Resume */
 FC_PD = 0xB9, /* Power-down */
 FC_QPI = 0x38, /* Enter QPI mode */
 FC_ERESET = 0x66, /* Enable Reset */
 FC_RESET = 0x99, /* Reset Device */
};

static void mpsse_xfer_spi(uint32_t spi, uint8_t *data, size_t n)
{
  // CAREFUL. this modifies the data...
  // don't use static arrays .
  
  for(size_t i = 0; i < n; ++i) {

    // spi_xfer(spi, data[i]);
    uint8_t ret = spi_xfer(spi, data[i]);
    data[i] = ret;
  }
}

// check that it really is active lo. yes. because of pullup.

static void flash_reset( uint32_t spi)
{
 uint8_t data[8] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

 // flash_chip_select();

 spi_enable(spi);
 mpsse_xfer_spi(spi, data, 8);
 spi_disable(spi);
 // flash_chip_deselect();
}


static void flash_power_up(uint32_t spi)
{
 uint8_t data_rpd[1] = { FC_RPD };

 spi_enable(spi);
 // flash_chip_select();
 mpsse_xfer_spi(spi, data_rpd, 1);

 spi_disable(spi);
 // flash_chip_deselect();
}

// OK. we have stuff on the scope. for status.
// But we are going through the fpga. so we need to make sure MISO propagates.
// OK. got status register being read.

static uint8_t flash_read_status( uint32_t spi)
{
 uint8_t data[2] = { FC_RSR1, 0 };

 spi_enable(spi);
 // flash_chip_select();
 mpsse_xfer_spi(spi, data, 2);
 spi_disable(spi);
 // flash_chip_deselect();

  
 return data[1];

}



static void flash_write_enable( uint32_t spi)
{
 uint8_t data[1] = { FC_WE };
 // flash_chip_select();

 spi_enable(spi);
 mpsse_xfer_spi(spi, data, 1);
 // flash_chip_deselect();
 spi_disable(spi);
}

//////////////////////////////////////////////

// REGISTER_DAC?

#define LED_REGISTER  0x07
#define LED1 (1<<0)    // D38
#define LED2 (1<<1)    // D37

#define DAC_REGISTER  0x09
#define DAC_LDAC      (1<<0)
#define DAC_RST       (1<<1)
#define DAC_UNI_BIP_A (1<<2)
#define DAC_UNI_BIP_B (1<<3)


#define SPI_MUX_REGISTER  0x08
#define SPI_MUX_ADC03     (1<<0)
#define SPI_MUX_DAC       (1<<1)
#define SPI_MUX_FLASH     (1<<2)



static void soft_500ms_update(void)
{
  // rename update() or timer()
  // think update for consistency.

  // blink led
  gpio_toggle(LED_PORT, LED_OUT);


  flash_reset( SPI_ICE40);

  flash_power_up(SPI_ICE40);

  // uint8_t ret = w25_read_sr1(SPI_ICE40);
  // usart_printf("w25 read %d\n", ret);

  // at the end of powerup. looks like miso goes lo.
  // OKK. this does prompt somethinig on scope.
  uint8_t status = flash_read_status( SPI_ICE40);
  usart_printf("status %d\n", status);



//  flash_write_enable( SPI_ICE40);

/*
  status = flash_read_status( SPI_ICE40);
  usart_printf("status after %d\n", status);
*/


/*
  // need to unlock.

  // well at least it doesn't block...
  uint16_t ret2 = w25_manuf_device(SPI_ICE40); 
  usart_printf("device %d\n", ret2);

  uint16_t ret3 = w25_manuf_device(SPI_ICE40); 
  usart_printf("device %d\n", ret3);



  uint8_t x = flash_read_status( SPI_ICE40);
  usart_printf("x %d\n", x);
*/

  // it's really not responding.

}

// OK. issue is that the clk and mosi are not separate. 
// actually not sure. rising edge looks  okk.


static void loop(void)
{

  static uint32_t soft_500ms = 0;

  while(true) {

    // EXTREME - can actually call update at any time, in a yield()...
    // so long as we wrap calls with a mechanism to avoid stack reentrancy
    // led_update(); in systick.


    // pump usart queues
    usart_input_update();
    usart_output_update();



    // 500ms soft timer
    if( system_millis > soft_500ms) {
      soft_500ms = system_millis + 500;

      soft_500ms_update();
    }

  }

}


// hmmm. problem.
// the register writing using one type of clock dir and flash communication a different type of clock.
// actually it's kind of ok. we will set everything up first.
// need to unlock?

// https://github.com/YosysHQ/icestorm/blob/master/iceprog/iceprog.c
// flash_read_status()
// looks like sends two bytes?
// flash_reset();
//  flash_power_up();

// need to solder pin for miso - and 
// use soic hat?


int main(void)
{
  // high speed internal!!!

  systick_setup(16000);


  // clocks
  rcc_periph_clock_enable(RCC_SYSCFG); // maybe required for external interupts?

  // LED
  rcc_periph_clock_enable(RCC_GPIOE);

  // USART
  rcc_periph_clock_enable(RCC_GPIOA);     // f407
  // rcc_periph_clock_enable(RCC_GPIOB); // F410
  rcc_periph_clock_enable(RCC_USART1);


  // spi / ice40
  rcc_periph_clock_enable(RCC_SPI1);

  //////////////////////
  // setup

  // led
  led_setup();


  // usart
  cBufInit(&console_in, buf1, sizeof(buf1));
  cBufInit(&console_out, buf2, sizeof(buf2));
  usart_setup_gpio_portA();
  usart_setup(&console_in, &console_out);     // gahhh... we have to change this each time...


  //
  // spi1_ice40_setup();
  spi1_special_setup();
  spi1_flash_setup();




//  ice40_write_register1(0xff );
  // ice40_write_register1(0 );
//  ice40_write_register1( 2 );

  ////////////////////


  usart_printf("\n--------\n");
  usart_printf("starting\n");
  // usart_printf("size %d\n", sizeof(fbuf) / sizeof(float));



  loop();

	for (;;);
	return 0;
}




