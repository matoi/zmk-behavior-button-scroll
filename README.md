# zmk-behavior-button-scroll

A ZMK behavior that sends discrete wheel events while holding a mouse button.
The button releases after an idle timeout, extended by every invocation. This can
turn a rotary encoder into pinch-style zoom through Mac Mouse Fix's configured
MB4 + scroll action. The module itself sends standard mouse input, not pinch events.

## Installation

Add the project to your keyboard's `config/west.yml` after publishing this repository:

```yaml
manifest:
  remotes:
    - name: matoi
      url-base: https://github.com/matoi
  projects:
    - name: zmk-behavior-button-scroll
      remote: matoi
      revision: main
```

The remote URL above is the intended publication location; creating a local
repository does not publish it. For local builds, add this checkout to the existing
semicolon-separated `ZMK_EXTRA_MODULES` CMake option. Preserve other modules.

Enable pointing support on the central side:

```conf
CONFIG_ZMK_POINTING=y
```

The behavior is enabled automatically when a matching Devicetree node exists.
Build from a clean build directory after adding the module.

## Basic usage

```dts
#include <behaviors/button_scroll.dtsi>

/ {
    behaviors {
        enc_scaler: encoder_mb4_wheel_scrl {
            compatible = "zmk,behavior-sensor-rotate-var";
            #sensor-binding-cells = <2>;
            bindings = <&button_scroll>, <&button_scroll>;
            tap-ms = <0>;
        };
    };
};
```

In the desired keymap layer:

```dts
sensor-bindings = <&enc_scaler SCRL_UP SCRL_DOWN>;
```

Swap the arguments to reverse direction. Horizontal scrolling uses
`SCRL_RIGHT` and `SCRL_LEFT`. Exactly one nonzero axis is accepted.

Override the defaults if needed:

```dts
&button_scroll {
    button = <MB4>;
    timeout-ms = <150>;
    wheel-step = <1>;
};
```

| Property | Default | Meaning |
| --- | --- | --- |
| `button` | `MB4` | One of `MB1`, `MB2`, `MB3`, `MB4`, `MB5` |
| `timeout-ms` | `150` | Positive idle interval in milliseconds |
| `wheel-step` | `1` | Wheel units per invocation, from 1 to 127 |

The parameter supplies direction only; the magnitude in `SCRL_*` or `MOVE_*` is
ignored. Amount per invocation is controlled by `wheel-step`. The timeout is a
starting setting, not a measured optimum.

## Shared common.keymap configuration

The current `matoi/zmk-config-common` detects whether
`behaviors/button_scroll.dtsi` is available in the build's module include paths.
When this module is present, common.keymap includes its default behavior, configures
MB4 / 150ms / one wheel unit, and binds `enc_scaler` to it with `tap-ms = <0>`.
Without this module, `enc_scaler` is not defined. Keyboards that reference it must
include this module. There is no macro fallback.
The detection happens during Devicetree preprocessing, before Kconfig, so it does
not depend on the Devicetree-derived `CONFIG_ZMK_BEHAVIOR_BUTTON_SCROLL` setting.

Individual keyboards opt in through their `west.yml` or local `ZMK_EXTRA_MODULES`.
They only need to use `&enc_scaler SCRL_UP SCRL_DOWN` (or reversed directions) in the
layer. Do not duplicate the include or encoder override in each keyboard. A board
can override `&button_scroll` after including common.keymap when it needs different
settings. Hardware-specific `steps` and `triggers-per-rotation` remain in the board
configuration.

For configurations that do not use the shared common.keymap,
[examples/common-keymap.dtsi](examples/common-keymap.dtsi) is a self-contained
replacement for the old `mb4_msc` macro and `enc_scaler` behavior. That example
includes its own input listener and must not be combined with the default instance
for the same purpose.

## Operation and limits

- The first invocation sends button-down, then one wheel event. Further invocations
  send wheel events and extend the same deadline. The encoder release callback is
  a no-op. A timer sends button-up directly through Zephyr's input subsystem.
- The timer checks the current deadline even if it was dispatched before a new
  invocation extended that deadline. Failed button releases are retried after 10ms.
- There is no held `&msc` state and no nested macro sequence. This removes the
  dependency on receiving the encoder release event. The outer encoder and input
  subsystem still have queues; this module does not promise zero latency under load.
- The timeout is measured from the last **processed** invocation, not physical
  rotation. Other keys or layer changes do not cancel it early. While the button
  remains held, cursor motion may be interpreted as dragging by the host.
- Use one instance for both directions. Instances sharing the same mouse button,
  or manual `&mkp` bindings holding that button concurrently, do not coordinate
  ownership. Avoid that combination. Multiple encoders using the same instance
  share one deadline.
- Button-down and the first wheel pulse are separate synchronized input reports
  without a fixed delay. Mac Mouse Fix recognition and host-side effects still
  require hardware testing. Smooth-scrolling resolution multipliers may change
  the effective amount per pulse.

## Tests

```sh
make -C tests/host test
```

The host tests compile the actual behavior source with mocked Zephyr input and
work interfaces. They cover deadline extension, stale callbacks, reversal,
missing encoder release, failed input delivery and retry, sustained rotation,
button selection, axes and invalid parameters. They do not emulate Zephyr thread
scheduling, BLE/USB or Mac Mouse Fix.

A firmware smoke build also passed with local ZMK v0.3 commit `dbfb6a35`,
Zephyr 3.5 and Zephyr SDK 0.17.0 for `seeeduino_xiao_ble`. The compile-only
fixture uses the `settings_reset` shield with a mock scanner; it is not an
application keymap for flashing. The new behavior, its bindings and input listener
were compiled and linked into the firmware.

To reproduce in a configured ZMK workspace:

```sh
./tests/firmware/build.sh /absolute/path/to/zmk/app /absolute/path/to/build-directory
```

With CMake 4, the older Zephyr checkout may additionally need
a trailing `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` argument to the script and
`ZEPHYR_TOOLCHAIN_VARIANT=zephyr` in the environment. Configure your SDK and compiler cache for your
machine. Firmware build success does not verify USB/BLE or Mac Mouse Fix behavior.
Test slow rotation,
fast rotation, reversal, stopping during layer changes, and both actual output
connections before relying on the behavior.

## References

- [ZMK mouse emulation](https://zmk.dev/docs/keymaps/behaviors/mouse-emulation)
- [ZMK sensor rotation](https://zmk.dev/docs/keymaps/behaviors/sensor-rotate)
- [ZMK mouse button implementation](https://github.com/zmkfirmware/zmk/blob/main/app/src/behaviors/behavior_mouse_key_press.c)
- [Zephyr workqueue documentation](https://docs.zephyrproject.org/latest/kernel/services/threads/workqueue.html)

MIT licensed.
