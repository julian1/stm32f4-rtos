


/*

  GPIOx_BSRR
    This is GPIO Bit Set/Reset Register. When you want to set or reset the
    particular bit or pin, you can use this register.

  GPIOx_ODR
    This is the Output Data Register. [...] this register is used to set the value
    to the GPIO pin.
  https://embetronicx.com/tutorials/microcontrollers/stm32/stm32-gpio-tutorial/#GPIOx_BSRR


void gpio_set(uint32_t gpioport, uint16_t gpios)
 {
         GPIO_BSRR(gpioport) = gpios;
 }

void  gpio_clear(uint32_t gpioport, uint16_t gpios)
 {
         GPIO_BSRR(gpioport) = (gpios << 16);
 }

  So BSRR. doesn't need bit masks, or to read values.  to set individual values.
    instead it uses low bytes to set.
    and high bytes to clear

  #define GPIO_BSRR  (     port )     MMIO32((port) + 0x18)
  #define MMIO32  (     addr )     (*(volatile uint32_t *)(addr))

toggle does a read eg. stores in port ..

void gpio_toggle(uint32_t gpioport, uint16_t gpios)
 {
         uint32_t port = GPIO_ODR(gpioport);
         GPIO_BSRR(gpioport) = ((port & gpios) << 16) | (~port & gpios);
 }


  so for setting it's just    current | val
     and clear                ~(~current | val)

   0b01111
  ~0b10010
val     1
  |  10010
  ~  01101

  but then how are they combined?
    yes. apply set(), take output, then apply clear()
  -----------------------

  we want to get analog power - to test dac. i think. in priority.


void gpio_port_write(uint32_t gpioport, uint16_t data)
 {
         GPIO_ODR(gpioport) = data;
 }
  write uses a different register.
*/