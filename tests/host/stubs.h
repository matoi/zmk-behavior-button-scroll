#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#define DT_HAS_COMPAT_STATUS_OKAY(x) 1
#define DT_INST_FOREACH_STATUS_OKAY(x)
#define K_FOREVER -1
#define K_MSEC(x) (x)
#define CONTAINER_OF(p,t,m) ((t *)((char *)(p)-offsetof(t,m)))
#define MOVE_Y_DECODE(v) ((int16_t)((v)&0xffff))
#define MOVE_X_DECODE(v) ((int16_t)(((v)>>16)&0xffff))
#define INPUT_BTN_0 0
#define INPUT_REL_HWHEEL 9
#define ARG_UNUSED(x) ((void)(x))
#define INPUT_REL_WHEEL 8
#define BEHAVIOR_LOCALITY_CENTRAL 0
struct device {void *data; const void *config;};
struct k_mutex {int unused;};
struct k_work {int unused;};
struct k_work_delayable {struct k_work work; void (*handler)(struct k_work *); int64_t due;};
static int64_t now;
static int button_downs, button_ups, wheel_sum, wheel_reports, fail_release, fail_press, fail_wheel, last_button, last_axis;
static void k_mutex_init(struct k_mutex *m) {}
static void k_mutex_lock(struct k_mutex *m,int wait) {}
static void k_mutex_unlock(struct k_mutex *m) {}
static int64_t k_uptime_get(void) {return now;}
static void k_work_init_delayable(struct k_work_delayable *w,void (*f)(struct k_work *)) {w->handler=f;}
static struct k_work_delayable *k_work_delayable_from_work(struct k_work *w) {return CONTAINER_OF(w,struct k_work_delayable,work);}
static int k_work_reschedule(struct k_work_delayable *w,int64_t delay) {w->due=now+delay;return 0;}
static int input_report_key(const struct device *d,int code,int val,bool sync,int wait) {
 last_button=code;
 if (val && fail_press) {fail_press=0;return -EIO;}
 if (!val && fail_release) {fail_release=0;return -EIO;}
 if(val) button_downs++; else button_ups++; return 0;
}
static int input_report_rel(const struct device *d,int code,int val,bool sync,int wait) {last_axis=code;if(fail_wheel){fail_wheel=0;return -EIO;}wheel_reports++;wheel_sum+=val;return 0;}
struct zmk_behavior_binding {const char *behavior_dev; uint32_t param1;};
struct zmk_behavior_binding_event {int unused;};
static const struct device *test_dev;
static const struct device *zmk_behavior_get_binding(const char *name) {return test_dev;}
struct behavior_driver_api {
 int locality;
 int (*binding_pressed)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event);
 int (*binding_released)(struct zmk_behavior_binding *,struct zmk_behavior_binding_event);
};
