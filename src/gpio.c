#include "nrf_gpio.h"
#include <mrubyc.h>

//      GPIO_IN          0b000 (defined in the SDK)
#define GPIO_IN 0b000
#define GPIO_IN_PULLUP   0b010
#define GPIO_IN_PULLDOWN 0b110
//      GPIO_OUT         0b001 (defined in the SDK)
#define GPIO_OUT 0b001
#define GPIO_OUT_LO      0b011
#define GPIO_OUT_HI      0b101

static void
c_gpio_get(mrb_vm *vm, mrb_value *v, int argc)
{
  bool val = nrf_gpio_pin_read(GET_INT_ARG(1));
  if( val ) {
    SET_TRUE_RETURN();
  } else {
    SET_FALSE_RETURN();
  }
}

static void
c_gpio_init(mrb_vm *vm, mrb_value *v, int argc)
{
  // do nothing
}

static void
c_gpio_set_dir(mrb_vm *vm, mrb_value *v, int argc)
{
  if (GET_INT_ARG(2)&1 == GPIO_OUT) {
    nrf_gpio_cfg_output(GET_INT_ARG(1));
    if (GET_INT_ARG(2) == GPIO_OUT_LO) {
      nrf_gpio_pin_clear(GET_INT_ARG(1));
    } else if (GET_INT_ARG(2) == GPIO_OUT_HI) {
      nrf_gpio_pin_set(GET_INT_ARG(1));
    }
  } else {
    if (GET_INT_ARG(2) == GPIO_IN_PULLUP) {
      nrf_gpio_cfg_input(GET_INT_ARG(1), NRF_GPIO_PIN_PULLUP);
    } else if (GET_INT_ARG(2) == GPIO_IN_PULLDOWN) {
      nrf_gpio_cfg_input(GET_INT_ARG(1), NRF_GPIO_PIN_PULLDOWN);
    } else {
      nrf_gpio_cfg_input(GET_INT_ARG(1), NRF_GPIO_PIN_NOPULL);
    }
  }
}

static void
c_gpio_pull_up(mrb_vm *vm, mrb_value *v, int argc)
{
  nrf_gpio_cfg_input(GET_INT_ARG(1), NRF_GPIO_PIN_PULLDOWN);
}

static void
c_gpio_put(mrb_vm *vm, mrb_value *v, int argc)
{
  int gpio = GET_INT_ARG(1);
  int value = GET_INT_ARG(2);
  nrf_gpio_pin_write(gpio, value);
}

void
prk_init_gpio(void)
{
  mrbc_define_method(0, mrbc_class_object, "gpio_init",    c_gpio_init);
  mrbc_define_method(0, mrbc_class_object, "gpio_set_dir", c_gpio_set_dir);
  mrbc_define_method(0, mrbc_class_object, "gpio_pull_up", c_gpio_pull_up);
  mrbc_define_method(0, mrbc_class_object, "gpio_put",     c_gpio_put);
  mrbc_define_method(0, mrbc_class_object, "gpio_get",     c_gpio_get);
}

