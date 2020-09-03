

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>


#include "utility.h"
#include "usart.h"
#include "rails.h"
#include "dac8734.h"


/*
  fucking hell
  spi 1 AF5
    MOSI == PA7 == GPIO7    DAC SDI pin 4
    MISO == PA6 == GPIO6    DAC SDO pin 5
*/
#define DAC_SPI       SPI1
#define DAC_PORT_SPI  GPIOA
#define DAC_CS        GPIO4
// use spi1/ port A alternate function
#define DAC_CLK       GPIO5
#define DAC_MOSI      GPIO7
#define DAC_MISO      GPIO3 // not connected right now.




//  GPIOE
#define DAC_PORT      GPIOE
// GPIO1 is led.
#define DAC_LDAC      GPIO2
#define DAC_RST       GPIO3
#define DAC_GPIO0     GPIO4  // gpio pe4   dac pin 8
#define DAC_GPIO1     GPIO5
#define DAC_UNIBIPA   GPIO6
#define DAC_UNIBIPB   GPIO7



static void dac_write_register_spi(uint32_t r)
{

  spi_send( DAC_SPI, (r >> 16) & 0xff );
  spi_send( DAC_SPI, (r >> 8) & 0xff  );
  spi_send( DAC_SPI, r & 0xff  );


/*
  // OK. this doesn't work...
  // think tries to do write followed by read, rather than simultaneously
  uint8_t a = spi_xfer( DAC_SPI, (r >> 16) & 0xff );
  uint8_t b = spi_xfer( DAC_SPI, (r >> 8) & 0xff  );
  uint8_t c = spi_xfer( DAC_SPI, r & 0xff  );
*/


/*
  // don't think we can read 24 bytes... when hardware limited to 16 bytes
  // perhaps this is just reading the same register value over and over...
  uint8_t a = spi_read( DAC_SPI );
  uint8_t b = spi_read( DAC_SPI );
  uint8_t c = spi_read( DAC_SPI );
*/

}


static void dac_write_register_bitbash(uint32_t v)
{
  for(int i = 23; i >= 0; --i) {

    gpio_set(DAC_PORT_SPI, DAC_CLK);  // clock high
    // msleep(1);      // OK... interesting it doesn't like this at all...
                        // but it should be fine

    // assert value
    if( v & (1 << i ))
      gpio_set(DAC_PORT_SPI, DAC_MOSI );
    else
      gpio_clear (DAC_PORT_SPI, DAC_MOSI );

    // read register something like this,
    // x |=  (gpio_get(DAC_PORT_SPI, DAC_MISO ) ? 1 : 0) << i ;

    msleep(1);
    gpio_clear(DAC_PORT_SPI, DAC_CLK);  // slave gets value on down transition
    msleep(1);
  }
}


static void dac_write_register1(uint32_t r)
{
  gpio_clear(DAC_PORT_SPI, DAC_CS);     // CS active low
  msleep(1);
  // dac_write_register_bitbash( r );     // write
  dac_write_register_spi( r );     // write
  msleep(1); // required
  gpio_set(DAC_PORT_SPI, DAC_CS);      // ldac is transparent if low, so will latch value on cs deselect (pull high).
  msleep(1);
}


void dac_write_register(uint8_t r, uint16_t v)
{

  dac_write_register1( r << 16 | v );

}


#if 0
static uint32_t dac_read(void)
{
  // dac write register exchange...

  msleep(1); // required
  gpio_clear(DAC_PORT_SPI, DAC_CS);  // CS active low

  // think problem is that it doesn't fiddle the clock.
  uint8_t a = spi_read(DAC_PORT);
  uint8_t b = spi_read(DAC_PORT);
  uint8_t c = spi_read(DAC_PORT);

  /*
  uint8_t a = spi_xfer(DAC_PORT, 0);
  uint8_t b = spi_xfer(DAC_PORT, 0 );
  uint8_t c = spi_xfer(DAC_PORT, 0);
  */
  msleep(1); // required
  gpio_set(DAC_PORT_SPI, DAC_CS);

  return (a << 16) | (b << 8) | c;
}
#endif


void dac_setup_spi( void )
{
  uart_printf("dac setup spi\n\r");

  // spi alternate function 5
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,  DAC_CLK | DAC_MOSI | DAC_MISO );

  gpio_set_af(GPIOA, GPIO_AF5,  DAC_CLK | DAC_MOSI | DAC_MISO );

  // rcc_periph_clock_enable(RCC_SPI1);
  spi_init_master(DAC_SPI,
    SPI_CR1_BAUDRATE_FPCLK_DIV_4,     // when we change this - we get different values?
    // SPI_CR1_BAUDRATE_FPCLK_DIV_256,     // the monitor pin values change - but still nothing correct
    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
    SPI_CR1_CPHA_CLK_TRANSITION_2,    // 1 == rising edge, 2 == falling edge.
    SPI_CR1_DFF_8BIT,
    SPI_CR1_MSBFIRST
    // SPI_CR1_LSBFIRST
  );
  spi_enable_ss_output(DAC_SPI);
  spi_enable(DAC_SPI);


  gpio_mode_setup(DAC_PORT_SPI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DAC_CS );

  /////
  // internal pu, doesn't change anything - because its powered off, and starts up high-Z.
  gpio_mode_setup(DAC_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DAC_RST | DAC_LDAC | DAC_UNIBIPA | DAC_UNIBIPB);
  gpio_mode_setup(DAC_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, DAC_GPIO0 | DAC_GPIO1 ); // these are open-drain as inputs
}



void dac_setup_bitbash( void )
{
  uart_printf("dac setup bitbash\n\r");

  gpio_mode_setup(DAC_PORT_SPI, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DAC_CS  | DAC_CLK | DAC_MOSI);
  gpio_mode_setup(DAC_PORT_SPI, GPIO_MODE_INPUT, GPIO_PUPD_NONE, DAC_MISO );

  gpio_mode_setup(DAC_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DAC_RST | DAC_LDAC | DAC_UNIBIPA | DAC_UNIBIPB);
  gpio_mode_setup(DAC_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, DAC_GPIO0 | DAC_GPIO1 ); // these are open-drain as inputs
}









void dac_test(void *args __attribute((unused)))
{
  /*
  Reset input (active low). Logic low on this pin resets the input registers
  and DACs to the values RST67I defined by the UNI/BIP pins, and sets the Gain
  Register and Zero Register to default values.
  */

  /* Load DAC latch control input(activelow). When LDAC is low, the DAC latch
  is transparent and the LDAC 56I contents of the Input Data Register are
  transferred to it.The DAC output changes to the corresponding level
  simultaneously when the DAClat */

  // do latch first... not sure order makes a difference
  gpio_clear(DAC_PORT, DAC_LDAC);   // keep latch low, and unused, unless chaining

  gpio_clear(DAC_PORT_SPI, DAC_CLK); // raise clock


  /*
  Output mode selection of groupB (DAC-2 and DAC-3). When UNI/BIP-A is tied to
  IOVDD, group B is in unipolar output mode; when tied to DGND, group B is in
  bipolar output mode
  */
  gpio_clear(DAC_PORT, DAC_UNIBIPA);
  gpio_clear(DAC_PORT, DAC_UNIBIPB);  // bipolar, lower supply voltage range required.

  //msleep(1000);   // 500ms not long enough. on cold power-up.
                  // 1s ok.
                  // actually sometimes 1s. fails.
                  // maybe issue with latch...
                  // Ok. 500ms was ok. when clear latch first.
                  // and 250ms was ok.
                  // actually nope. it must have been running from cap charge.  10secs usb unplug no good.
                  // ----------
                  // Maybe need to pull reset low - as first gpio configuration action, before configure spi etc.
                  // Also could be rails, need to be pulled to ground - first, and there is stray capacitance on them.
                  // Also could be,


  // These values need to be increased for cold start...
  uart_printf("dac reset\n\r");
  gpio_clear(DAC_PORT, DAC_RST);
  msleep(20);
  gpio_set(DAC_PORT, DAC_RST);
  msleep(20);
  uart_printf("reset done\n\r");

  // god damn it.

  uart_printf("mcu gpio read %d %d\n\r", gpio_get(DAC_PORT, DAC_GPIO0), gpio_get(DAC_PORT, DAC_GPIO1));
  // TODO - IMPORTANT - remove this.  just clear gpio pins separately if need to.
  uart_printf("dac clear\n\r");
  dac_write_register1( 0);
  uart_printf("mcu gpio read %d %d\n\r", gpio_get(DAC_PORT, DAC_GPIO0), gpio_get(DAC_PORT, DAC_GPIO1));
  uart_printf("dac set\n\r");
  // OK. with bitbashing reg 8 and 9 look correct...
  // dac_write_register1( 0 << 16 | 1 << 9 | 1 << 8); // ok so this really looks like it works ok...

  dac_write_register(0, 1 << 9 | 1 << 8);

  uart_printf("mcu gpio read %d %d\n\r", gpio_get(DAC_PORT, DAC_GPIO0), gpio_get(DAC_PORT, DAC_GPIO1));


  // dac_write_register1( 0 << 16 | 1 << 14); // LD bit    // now sometimes comes up 0, 0.629, or 0.755 ...
                                                        // indicating that gain regsisters are etting garbage?
                                                        // LD default value is 0...
                                                        // freaking weird.


  rails_turn_on();


  /*
  34,the digital supplies (DVDD and IOVDD) and logic inputs (UNI/BIP-x) must be
  applied before AVSS and AVDD. Additionally, AVSS must be applied before AVDD
  unless both can ramp up at the same time. REF-x should be applied after AVDD
  comes up in order to make sure the ESD protection circuitry does not turn on.
  */

  /*
    I think we need to understand this better...
    ...
    p21.
    The DAC8734 updates the DAC latch only if it has been accessed since the last
    time the LDAC pin was brought low or the LD bit in the CommandRegister was set
    to'1', there by eliminating any unnecessary glitch.
  */


  // WRITING THIS - does not affect mon value...
  uart_printf("writing dac register 1\n\r");
  // dac_write_register1( 0b00000100 << 16 | 0x7f7f ); // write dac 0
  //dac_write_register1( 0b00000101 << 16 | 0x3fff ); // write dac 1 1.5V out.
  // dac_write_register1( 0b00000101 << 16 | 0x2fff ); // write dac 1 1.129 out.
  // dac_write_register1( 0b00000101 << 16 | -10000 ); // didn't work
  // dac_write_register1( 0b00000101 << 16 | 10000 ); // works 0.919V
  // dac_write_register1( 0b00000101 << 16 | 0xffff - 10000 ); //  works. output -0.919V
  // dac_write_register1( 0b00000101 << 16 | 0x5fff );

  dac_write_register(0x05, 0x5fff ); // dac1 0b0101


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

  uart_printf("write mon register for ain\n\r");
  // dac_write_register1( 0b00000001 << 16 | (1 << 11) ); // select AIN.
  // dac_write_register1( 0b00000001 << 16 | (1 << 13) ); // select dac 1.

  dac_write_register(0x01, (1 << 13) ); // select monitor dac1



  uart_printf("finished\n\r");

  // sleep forever
  // exiting a task thread isn't very good...
  for(;;) {
    msleep(1000);
  }
}







// OK. on reset there is no glitch. for neg rail. or pos rail. but there is when 3.3V power first applied . 250nS.



// gpio_clear (RAILS_PORT, RAILS_NEG);    // turn on... eg. pull p-chan gate down from 3.3V to 0.

// OK - problem - our p-chan fet for neg rail is barely turning on.
// to pull up the n-chan gate.


// OK - a 1k on the fet gate. and defining the port before init... makes the top rail not glitch.

// bottom rail  - our p-chan fet is too weak.
// if use 1k / 10k. it doesn't turn on at all.
// if use without input resistor - it barely turns on.
///// /HMMMMM

// no it does glitch.
// but why? because of weako

// ok. and 1k seems to often work for neg rail.
// issue - but secondary issue is that the tx VGS is too bad for us to use it to turn the damn rail on.
// God damn it...
// sometimes it spikes and sometimes it doesn't - with or without resistor.
// when it first turns on - the gate is negative.
// freaking...
// ---------
// try to use dg444?
// and now we cannot trigger it...

// ok added a 22uF cap as well...   glitching is only occasional and slight.c
// powering on the other stmicro - 3.3V supply helps a bit also.





/*
  OK. its complicated.
    the fet comes on as 3V is applied...
    BUT what happens if
    if have rail voltage and no 3V. should be ok. because of biasing.

  When we first plug in- we get a high side pulse.
    about 250nS.

    Looks like the gate gets some spikes.
    OK. but what about a small cap...
  ----
  ok 1nF helps
  but there's still a 20nS pulse... that turns on the gateo
  --
  OK. both rails glitch at power-on.
  ...
  fuck.

  try a cap across the gate...
  ok 1nF seems to be ok.

  ok. 0.1uF seems to have fixed.
  NOPE.

  OK. lets try a 10k on the gate... to try and stop the ringing.
  mosfet gatep has cap anyway.

  OK. disconnecting the signal - and we don't trigger. very good.
  ------------
  sep 1.
  OK. started from cold ok. but now cannot get to start now.
    No seems to come on ok with 1V ref on. 0.730V dac1-out.

*/








  // msleep(1);
  // gpio_clear(DAC_PORT_SPI, DAC_CS);  // CS active low
  // msleep(1);

  /*
    we need to control the general 3.3V power rail, as well without unplugging the usb all the time
      and having to reset openocd.
    -------
    actually perhaps we need to control the 3.3V power for the dac.
    not. sure
    turn on only after have everything set up. no. better to give it power first?
    i think.
    --------
    would it make it easier to test things - if could power everything separately.
  */

  /*
  Writing a '1' to the GPIO-0 bit puts the GPIO-1 pin into a Hi-Z state(default).
  DB8 GPIO-01 Writing a '0' to the GPIO-0 bit forces the GPIO-1 pin low

  p22 After a power-on reset or any forced hardware or software reset, all GPIO-n
  bits are set to '1', and the GPIO-n pin goes to a high-impedancestate.
  */

  // spi_xfer is 'data write and read'.
  // think we should be using spi_send which does not return anything

  // dac_write_register(  1 << 8 | 1 << 7 );

  // dac_write_register( 1 << 22 | 1 << 8 | 1 << 6 ); // read and nop
  // dac_write_register( 1 << 8  | 1 << 6 );      // nop, does nothing
  // dac_write_register( 1 << 22 | 1 << 8 );      // writes, it shouldn't though...
  // dac_write_register( 0 );        // turns off ,
  // dac_write_register( 1 << 7  );

  // very strange - code does not initialize properly... when plugged...

  /*********
  // reset gives gpio values = 1, which is high-z, therefore pulled hi.
  // setting to 0 will clear.
  **********/

  // msleep(1); // required
  // gpio_set(DAC_PORT_SPI, DAC_CS);      // if ldac is low, then latch will latch on deselect cs.


  // gpio_clear(DAC_PORT, DAC_LDAC);



/*
  // first byte,
  spi_send(DAC_SPI, 0);
  //spi_send(DAC_SPI, 0);
  spi_send(DAC_SPI, 0 | 1 );           // dac gpio1 on

  // spi_send(DAC_SPI, 0);
  spi_send(DAC_SPI, 0 | 1 << 7 );  // turn on gpio0
*/

#if 0
static uint8_t dac_read(void)
{


  /*
    OK.
      some timing diagrams are weird. BUT

      case 5. p11.  for standalone mode. read timing is fine. see p11.
  */

/*
    OK. issue is that spi_xfer attempts to read after sending a full byte
    while we want simultaneous. read/write.
    Not sure if supported or can do it without bit-bashing supported.

    spi_xfer,
      "Data is written to the SPI interface, then a read is done after the incoming transfer has finished."

    issue is that we cannot just clock it out.
    instead we have to send a no-op, while clocing it out.

    BUT. if we used a separate spi channel for input.
    Then we could do the write.
    while simultaneously doing a blocking read in another thread.
    pretty damn ugly.
    better choice would be to bit-bash.
*/

  // dac_write_register1( 0 );



                                                // very strange
  return 123;   // c value is 128... eg. 10000000 this was kind of correct for the gpio1 in last byte. ....
              // so appear to be getting something out....
              // but really need to look at it on a scope

              // b is now returning 1....
              // c is returning 2.
}
#endif

// use spi_read


/*
  strange issue - when first plug into usb power - its not initialized properly...
  but reset run is ok.
  could be decoupling
  or because mcu starts with GPIO undefined?.. but when do 'reset run' the gpio is still defined because its
  a soft reset?
  - Or check the reset pin is genuinely working? or some other sync issue?
*/

/*
  OK. that is really very very good.
    we want to add the other uni/bip .

  and we want to try and do spi read.
  bit hard to know how it works - if get back 24 bytes.
    except we control the clock... well spi does.

  having gpio output is actually kind of useful - for different functions . albeit we would just use mcu.
  likewise the mixer.
  ---

  ok added external 10k pu. does not help. bloody weird.

*/
#if 0
  spi_send(DAC_SPI, 0);
  //spi_send(DAC_SPI, 0);
  spi_send(DAC_SPI, 0 | 1 );           // dac gpio1 on

  // spi_send(DAC_SPI, 0);
  spi_send(DAC_SPI, 0 | 1 << 7 );  // turn on gpio0
#endif


#if 0
  // pull latch up to write
  uart_printf("toggle ldac\n\r");
  gpio_set(DAC_PORT, DAC_LDAC);
  msleep(1);

  // gpio_clear(DAC_PORT, DAC_LDAC);
  // msleep(1);
#endif

