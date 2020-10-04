/*
 *
  see, for
  ~/devel/stm32/FreeRTOSv10.3.1/FreeRTOS/Demo/CORTEX_M4F_STM32F407ZG-SK/FreeRTOSConfig.h

  doc,
  https://www.freertos.org/FreeRTOS-for-STM32F4xx-Cortex-M4F-IAR.html

  so, i think 'proper' usart will use dma.

  // OK. so we want to move the dac code.

 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
  - ok. move back to actual spi port - see if can still bit bash.
  - see if peripheral spi works.
  - mon - to ADC - resistor divider? won't work - for negative signals.

  - we want
*/

// #include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
// #include <libopencm3/stm32/timer.h>
// #include <libopencm3/stm32/spi.h>
// #include <libopencm3/stm32/gpio.h>
// #include <libopencm3/stm32/usart.h>
// #include <libopencm3/cm3/nvic.h>


#include "usart.h"
#include "led.h"
#include "rails.h"
#include "ref.h"
#include "dac8734.h"
#include "utility.h" // msleep


/*
  OK. our sequencing *is* no good.
    - because we are pumping in the reference before the rails are up.
    - we need to fix this.

*/

// static int last = 0;

static void led_blink_task2(void *args __attribute((unused))) {

	for (;;) {

    // led_toggle
		led_toggle();

    // ping
    // uart_printf("ping %d\n\r", last++);

/*
    uart_printf("hi %d %d %d\n\r",
      last++,
      gpio_get(DAC_PORT, DAC_GPIO0),
      gpio_get(DAC_PORT, DAC_GPIO1 )

    );
*/
    msleep(500);
  }
}






static void dac_test(void)
{

  dac_reset();

  /*
  34,the digital supplies (DVDD and IOVDD) and logic inputs (UNI/BIP-x) must be
  applied before AVSS and AVDD. Additionally, AVSS must be applied before AVDD
  unless both can ramp up at the same time. REF-x should be applied after AVDD
  comes up in order to make sure the ESD protection circuitry does not turn on.
  */
  msleep(50);
  rails_negative_on();
  msleep(50);
  rails_positive_on();
  msleep(50);
  ref_on();
  msleep(50);


  // WRITING THIS - does not affect mon value...
  uart_printf("dac writing dac register 1\n\r");
  // dac_write_register1( 0b00000100 << 16 | 0x7f7f ); // write dac 0
  //dac_write_register1( 0b00000101 << 16 | 0x3fff ); // write dac 1 1.5V out.
  // dac_write_register1( 0b00000101 << 16 | 0x2fff ); // write dac 1 1.129 out.
  // dac_write_register1( 0b00000101 << 16 | -10000 ); // didn't work
  // dac_write_register1( 0b00000101 << 16 | 10000 ); // works 0.919V
  // dac_write_register1( 0b00000101 << 16 | 0xffff - 10000 ); //  works. output -0.919V
  // dac_write_register1( 0b00000101 << 16 | 0x5fff );

  // dac_write_register(0x05, 0x5fff ); // dac1 0b0101
  // dac_write_register(0x05, 0 ); // dac1 0b0101
  // dac_write_register(0x05, 0x7fff );
  // dac_write_register(0x05, 0 );      // Vout = 0V
  // dac_write_register(0x05, 65535 );     // Vout == 13.122


  dac_write_register(0x04, 25000 );  // Vout == 5V

  dac_write_register(0x05, 50000 );  // Iout == 10V


  // ok for v reference of 6.5536V
  // then rails need to be 6.5536 * 2 + 1 == 14.1V.
  //


#if 0
  dac_write_register1( 0b00000110 << 16 | 0x7f7f ); // write dac 2
  dac_write_register1( 0b00000111 << 16 | 0x7f7f ); // write dac 3
  msleep(1);  // must wait for update - before we read
#endif


  /*
    OKK - power consumption - seems *exactly* right.  around 10mA.
    AIDD (normaloperation) ±10V output range, no loading current, VOUT=0V 2.7-3.4mA/Channel
    AAISS(normaloperation)±10V outputrange, no loadingcurrent, VOUT=0V 3.3-4.0mA/Channel
    Input current  1μA - this is for the digital section only.
    // guy says device is drawing 10mA.
    // https://e2e.ti.com/support/data-converters/f/73/t/648061?DAC8734-Is-my-dac-damaged-

  */

  // 11 is ain. 13 is dac1.

  uart_printf("dac write mon register for ain\n\r");
  // dac_write_register1( 0b00000001 << 16 | (1 << 11) ); // select AIN.
  // dac_write_register1( 0b00000001 << 16 | (1 << 13) ); // select dac 1.

  dac_write_register(0x01, (1 << 13) ); // select monitor dac1

  uart_printf("dac finished\n\r");

#if 0
#endif

  // sleep forever
  // exiting a task thread isn't very good...
  for(;;) {
    msleep(1000);
  }

}




static void dac_test1(void *args __attribute((unused)))
{
  dac_test();
}






int main(void) {

  ////
  // Disable HSE
  // clocks
  // rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  // led
  rcc_periph_clock_enable(RCC_GPIOE); // JA
  // Dac
  rcc_periph_clock_enable(RCC_GPIOB); // JA

  // usart
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_USART1);

  // spi1
  rcc_periph_clock_enable(RCC_SPI1);



  ///////////////
  // setup
  // TODO maybe change names setup_led() setup_uart() ?
  led_setup();
  usart_setup();


  uart_printf("------------------\n\r");

/*
  This organisation is really bad...
  There should be a single linear task that does all the configuration.
  Rather than doing half in the main thread, and half in the task thread.
*/


  rails_setup();
  ref_setup();
  // dac_setup_bitbash();
  dac_setup_spi();


  ///////////////
  // tasks
  // value is the stackdepth.
	xTaskCreate(led_blink_task2, "LED",100,NULL,configMAX_PRIORITIES-1,NULL);
  xTaskCreate(uart_task,      "UART",200,NULL,configMAX_PRIORITIES-1,NULL); /* Highest priority */

  // IMPORTANT setting from 100 to 200, stops deadlock
  xTaskCreate(usart_prompt_task,    "PROMPT",200,NULL,configMAX_PRIORITIES-2,NULL); /* Lower priority */


  // ok....
  xTaskCreate(dac_test1,    "DAC_TEST",200,NULL,configMAX_PRIORITIES-2,NULL); // Lower priority

	vTaskStartScheduler();

	for (;;);
	return 0;
}


