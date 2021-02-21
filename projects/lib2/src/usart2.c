////////////////////////////////////////////////////////

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>

#include <stddef.h> // size_t

#include "buffer.h"
#include "usart2.h"




static CBuf *output_buf = NULL;
static CBuf *input_buf = NULL;


void usart_setup( CBuf *output, CBuf *input)
{
  output_buf = output;
  input_buf = input;

  //////

  nvic_enable_irq(NVIC_USART1_IRQ);
  nvic_set_priority(NVIC_USART1_IRQ,  5 );    // value???

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO10);


  gpio_set_af(GPIOA, GPIO_AF7, GPIO9 | GPIO10);

  gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO10);

  usart_set_baudrate(USART1, 115200);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  // usart_set_mode(USART1, USART_MODE_TX_RX);

  usart_set_mode(USART1, USART_MODE_TX_RX);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);


  /* Enable USART1 Receive interrupt. */
  usart_enable_rx_interrupt(USART1);

  usart_enable(USART1);
}


void usart1_isr(void)
{

  /* Check if we were called because of RXNE. */
  if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) &&
      ((USART_SR(USART1) & USART_SR_RXNE) != 0)) {

    /* Retrieve the data from the peripheral.
      and clear flags.
     */
    char ch = usart_recv(USART1);

    // write the input buffer
    cBufWrite(input_buf, ch);
  }

  return ;
}






/////////////////////////////////////////////

// maybe change name to flush out()?

void usart_output_update()
{
  // eg. superloop.

  while(true) {
    // if tx queue empty - do nothing, just return
    if(!usart_get_flag(USART1,USART_SR_TXE))
      return;

    // check for output to flush...
    int32_t ch = cBufRead(output_buf);
    if(ch == -1)
      return;

    usart_send(USART1,ch);
  }
}


/*
  OK. EXTREME.
    when logging from a long func,
    we do not need to wait for control to return to superloop, in order to flush the console output
    we can sync/flush anytime
    or even automatically on '\n' line buffered...

    change name sync_flush();
*/

void usart_sync_flush()
{

  while(true) {

    // check for output to flush...
    int32_t ch = cBufRead(output_buf);
    if(ch == -1)
      return;

    // block, until tx empty
    while(!usart_get_flag(USART1,USART_SR_TXE));

    usart_send(USART1,ch);
  }
}




void usart_input_update()
{
  // just transfer any input chars to output so that they appear on output
  // note that this consumes input.
  // more an example, can do line buffering etc, fsm on input also.
  while(true) {

    // read input buf
    int32_t ch = cBufRead(input_buf);
    if(ch == -1)
      return;
    // transfer to output buf
    cBufWrite(output_buf, ch);
  }
}




