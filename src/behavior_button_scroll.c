/* SPDX-License-Identifier: MIT */
#define DT_DRV_COMPAT zmk_behavior_button_scroll

#include <drivers/behavior.h>
#include <dt-bindings/zmk/pointing.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zmk/behavior.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct button_scroll_config {
  int button_code;
  int timeout_ms;
  int wheel_step;
};

struct button_scroll_data {
  const struct device *dev;
  struct k_mutex lock;
  struct k_work_delayable release_work;
  int64_t release_at;
  bool held;
};

static void button_scroll_timeout(struct k_work *work) {
  struct button_scroll_data *data =
      CONTAINER_OF(k_work_delayable_from_work(work), struct button_scroll_data,
                   release_work);
  const struct button_scroll_config *cfg = data->dev->config;
  k_mutex_lock(&data->lock, K_FOREVER);

  if (data->held) {
    int64_t remaining = data->release_at - k_uptime_get();
    if (remaining > 0) {
      /* A rotation may have extended the deadline as this work started. */
      k_work_reschedule(&data->release_work, K_MSEC(remaining));
    } else {
      int err =
          input_report_key(data->dev, cfg->button_code, 0, true, K_FOREVER);
      if (err < 0) {
        /* Do not forget a failed release. */
        k_work_reschedule(&data->release_work, K_MSEC(10));
      } else {
        data->held = false;
      }
    }
  }

  k_mutex_unlock(&data->lock);
}

static int button_scroll_pressed(struct zmk_behavior_binding *binding,
                                 struct zmk_behavior_binding_event event) {
  ARG_UNUSED(event);
  const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
  const struct button_scroll_config *cfg = dev->config;
  struct button_scroll_data *data = dev->data;
  int x = MOVE_X_DECODE(binding->param1);
  int y = MOVE_Y_DECODE(binding->param1);

  if ((x == 0) == (y == 0)) {
    return -EINVAL;
  }

  k_mutex_lock(&data->lock, K_FOREVER);
  if (!data->held) {
    int err = input_report_key(dev, cfg->button_code, 1, true, K_FOREVER);
    if (err < 0) {
      k_mutex_unlock(&data->lock);
      return err;
    }
    data->held = true;
  }

  /* Arm cleanup on press; encoder release events are deliberately unnecessary.
   */
  data->release_at = k_uptime_get() + cfg->timeout_ms;
  k_work_reschedule(&data->release_work, K_MSEC(cfg->timeout_ms));

  /* One relative wheel event, with no persistent &msc state to release. */
  int direction = x != 0 ? x : y;
  int err = input_report_rel(dev, x != 0 ? INPUT_REL_HWHEEL : INPUT_REL_WHEEL,
                             direction > 0 ? cfg->wheel_step : -cfg->wheel_step,
                             true, K_FOREVER);
  k_mutex_unlock(&data->lock);
  return err;
}

static int button_scroll_released(struct zmk_behavior_binding *binding,
                                  struct zmk_behavior_binding_event event) {
  ARG_UNUSED(binding);
  ARG_UNUSED(event);
  return 0;
}

static int button_scroll_init(const struct device *dev) {
  struct button_scroll_data *data = dev->data;
  data->dev = dev;
  k_mutex_init(&data->lock);
  k_work_init_delayable(&data->release_work, button_scroll_timeout);
  return 0;
}

static const struct behavior_driver_api button_scroll_api = {
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
    .binding_pressed = button_scroll_pressed,
    .binding_released = button_scroll_released,
};

#define BUTTON_SCROLL_INST(n)                                                  \
  BUILD_ASSERT(DT_INST_PROP(n, timeout_ms) > 0,                                \
               "timeout-ms must be positive");                                 \
  BUILD_ASSERT(DT_INST_PROP(n, wheel_step) > 0 &&                              \
                   DT_INST_PROP(n, wheel_step) <= 127,                         \
               "wheel-step must be 1..127");                                   \
  static struct button_scroll_data button_scroll_data_##n;                     \
  static const struct button_scroll_config button_scroll_config_##n = {        \
      .button_code = INPUT_BTN_0 + __builtin_ctz(DT_INST_PROP(n, button)),     \
      .timeout_ms = DT_INST_PROP(n, timeout_ms),                               \
      .wheel_step = DT_INST_PROP(n, wheel_step),                               \
  };                                                                           \
  BEHAVIOR_DT_INST_DEFINE(n, button_scroll_init, NULL,                         \
                          &button_scroll_data_##n, &button_scroll_config_##n,  \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,    \
                          &button_scroll_api);

DT_INST_FOREACH_STATUS_OKAY(BUTTON_SCROLL_INST)
#endif
