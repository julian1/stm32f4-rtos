/*
  change name to supplies?
*/

#include <libopencm3/stm32/gpio.h>

#include "usart.h"
#include "ref.h"


#define REF_PORT    GPIOE
// #define REF_VREF    GPIO11
#define REF_PIN     GPIO13  // dg444 pins swapped.. change name REF_PORT??? or REF_IN? or REF_SWITCH?


// TODO pass an argument. eg. 1 for on. 0 for off.  ref_ctrl(bool)

void ref_on( void )
{
  gpio_clear(REF_PORT, REF_PIN);
  uart_printf("ref on\n\r");
}

/*
  remember dg444 active low.
  So *MUST* set state/config... before we poweron rails...
*/

void ref_setup( void )
{
  // WE SHOULD DO ALL PINS here???

  uart_printf("ref setup\n\r");

  gpio_set(REF_PORT, REF_PIN);

  gpio_mode_setup(REF_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, REF_PIN);

  uart_printf("ref setup done\n\r");
}