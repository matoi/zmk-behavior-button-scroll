#include "../../src/behavior_button_scroll.c"
#include <assert.h>
#include <stdio.h>

int main(void) {
  struct button_scroll_data data = {0};
  const struct button_scroll_config cfg = {
      .button_code = INPUT_BTN_0 + 3, .timeout_ms = 150, .wheel_step = 2};
  struct device dev = {.data = &data, .config = &cfg};
  test_dev = &dev;
  struct zmk_behavior_binding binding = {.param1 = 10};
  struct zmk_behavior_binding_event event = {0};
  assert(button_scroll_init(&dev) == 0);

  assert(button_scroll_api.binding_pressed(&binding, event) == 0);
  assert(button_downs == 1 && wheel_sum == 2 && data.held);
  assert(last_button == INPUT_BTN_0 + 3 && last_axis == INPUT_REL_WHEEL);
  now = 100;
  assert(button_scroll_pressed(&binding, event) == 0);
  assert(button_downs == 1 && wheel_sum == 4 && data.release_at == 250);

  /* A callback already dispatched at the old deadline must respect extension.
   */
  now = 150;
  button_scroll_timeout(&data.release_work.work);
  assert(data.held && button_ups == 0 && data.release_work.due == 250);

  binding.param1 = (uint16_t)-10;
  now = 180;
  button_scroll_pressed(&binding, event);
  assert(wheel_sum == 2 && button_downs == 1 && data.release_at == 330);

  /* No encoder release event is delivered. Cleanup must still occur. */
  now = 330;
  button_scroll_timeout(&data.release_work.work);
  assert(!data.held && button_ups == 1 && wheel_reports == 3);
  button_scroll_timeout(&data.release_work.work);
  assert(button_ups == 1);

  now = 400;
  button_scroll_pressed(&binding, event);
  now = 550;
  fail_release = 1;
  button_scroll_timeout(&data.release_work.work);
  assert(data.held && data.release_work.due == 560);
  now = 560;
  button_scroll_timeout(&data.release_work.work);
  assert(!data.held && button_ups == 2);
  button_scroll_released(&binding, event);
  assert(!data.held);

  /* Failed presses must not produce wheel events or claim ownership. */
  int reports = wheel_reports;
  fail_press = 1;
  assert(button_scroll_pressed(&binding, event) == -EIO);
  assert(!data.held && wheel_reports == reports);

  /* Failed wheel delivery must still leave a working cleanup deadline. */
  now = 600;
  fail_wheel = 1;
  assert(button_scroll_pressed(&binding, event) == -EIO);
  assert(data.held && data.release_work.due == 750);
  now = 750;
  button_scroll_timeout(&data.release_work.work);
  assert(!data.held);

  /* Sustained rotation only presses the button once and leaves one deadline. */
  int downs = button_downs;
  for (int i = 0; i < 1000; ++i) {
    now = 1000 + i;
    assert(button_scroll_pressed(&binding, event) == 0);
  }
  assert(button_downs == downs + 1 && data.release_at == 2149);
  now = 2149;
  button_scroll_timeout(&data.release_work.work);
  assert(!data.held);

  binding.param1 = 0;
  assert(button_scroll_pressed(&binding, event) == -EINVAL);
  binding.param1 = 0x00010001;
  assert(button_scroll_pressed(&binding, event) == -EINVAL);
  assert(!data.held);

  /* A second instance can select a different button and horizontal direction.
   */
  struct button_scroll_data other_data = {0};
  const struct button_scroll_config other_cfg = {
      .button_code = INPUT_BTN_0 + 1, .timeout_ms = 50, .wheel_step = 3};
  struct device other = {.data = &other_data, .config = &other_cfg};
  test_dev = &other;
  button_scroll_init(&other);
  binding.param1 = (uint32_t)(uint16_t)-10 << 16;
  reports = wheel_sum;
  assert(button_scroll_pressed(&binding, event) == 0);
  assert(last_axis == INPUT_REL_HWHEEL && wheel_sum == reports - 3);
  assert(last_button == INPUT_BTN_0 + 1 && other_data.held && !data.held);
  now += 50;
  button_scroll_timeout(&other_data.release_work.work);
  assert(!other_data.held);
  puts("PASS: timeout, retrigger, stale callback, reversal, missing release, "
       "failure recovery, sustained rotation, configuration and axes");
  return 0;
}
