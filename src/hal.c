#include <string.h>
#include <stdlib.h>

/* hardware */
#include <nrf.h>
#include <nrf_nvic.h>
#include "../lib/picoruby/mrbgems/picoruby-io-console/src/hal/hal.h"

/* mruby/c */
#include <rrt0.h>

#ifndef MRBC_TICK_UNIT
#define MRBC_TICK_UNIT 1
#endif

#ifndef MRBC_NO_TIMER
#include <nrfx_timer.h>

const nrfx_timer_t TIMER_MRB = NRFX_TIMER_INSTANCE(3);

void
alarm_irq(nrf_timer_event_t event_type, void* p_context)
{
    switch(event_type) {
        case NRF_TIMER_EVENT_COMPARE0:
            mrbc_tick();
            break;
    }
}

void
hal_init(void)
{
  nrfx_timer_config_t timer_config = NRFX_TIMER_DEFAULT_CONFIG;
  nrfx_timer_init(&TIMER_MRB, &timer_config, alarm_irq);
  uint32_t time_ticks = nrfx_timer_ms_to_ticks(&TIMER_MRB, MRBC_TICK_UNIT);
  nrfx_timer_extended_compare(
    &TIMER_MRB, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
  nrfx_timer_enable(&TIMER_MRB);
}

void
hal_enable_irq()
{
  sd_nvic_EnableIRQ(TIMER3_IRQn);
}

void
hal_disable_irq()
{
  sd_nvic_DisableIRQ(TIMER3_IRQn);
}

void
hal_idle_cpu()
{
  __WFI();
}

#else // MRBC_NO_TIMER
#warning "If you use MRBC_NO_TIMER, sleep_ms is not working correctly."
void
hal_idle_cpu()
{
  nrf_delay_ms(MRBC_TICK_UNIT);
  mrbc_tick();
}
#endif

int hal_write(int fd, const void *buf, int nbytes)
{
  // todo
  return 0;
}

int hal_flush(int fd) {
  // todo
  return 0;
}

int
hal_read_available(void)
{
  // todo
  return 0;
}

int
hal_getchar(void)
{
  // todo
  return '1';
}

//================================================================
/*!@brief
  abort program

  @param s	additional message.
*/
void hal_abort(const char *s)
{
  // todo
}


