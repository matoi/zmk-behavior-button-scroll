# zmk-behavior-button-scroll

A ZMK behavior that sends wheel events while holding a mouse button, then releases
it after an idle timeout. Use it with a rotary encoder and
[Mac Mouse Fix](https://macmousefix.com/)'s button + scroll action for pinch-style zoom.
The held button is configurable (`MB1`–`MB5`, default `MB4`).
The module sends standard mouse input; the host handles zoom.

## Installation

Add to your keyboard's `config/west.yml`:

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

For local builds, append this checkout to `ZMK_EXTRA_MODULES` instead.
Enable pointing support on the central side and rebuild from a clean directory:

```conf
CONFIG_ZMK_POINTING=y
```

## Usage

Include the module's behaviors in your keymap:

```dts
#include <behaviors/button_scroll.dtsi>
```

This provides `button_scroll` and the encoder behavior `enc_scaler`.
Add to the desired layer:

```dts
sensor-bindings = <&enc_scaler SCRL_UP SCRL_DOWN>;
```

Swap the arguments to reverse direction, or use `SCRL_RIGHT` / `SCRL_LEFT` for
horizontal scrolling. Parameters specify direction only; exactly one nonzero axis
is required.

Override settings as needed, matching `button` to your host-side configuration:

```dts
&button_scroll {
    button = <MB3>;
    timeout-ms = <150>;
    wheel-step = <1>;
};
```

| Property | Default | Range |
| --- | --- | --- |
| `button` | `MB4` | `MB1`–`MB5` |
| `timeout-ms` | `150` | Positive idle timeout in milliseconds |
| `wheel-step` | `1` | 1–127 wheel units per invocation |

For a shared keymap, include the module there and remove any local `enc_scaler`
definition to avoid duplicate labels. Keep hardware-specific encoder settings in
the board configuration. See [examples/common-keymap.dtsi](examples/common-keymap.dtsi).

## Behavior and limitations

- Each invocation sends one wheel pulse and extends the button's idle timeout.
  Release is timer-driven and does not depend on the encoder release callback.
- The timeout starts from the last processed invocation. Keys and layer changes
  do not cancel it; pointer motion while the button is held may cause dragging.
- Use one instance for both directions. Avoid other instances or `&mkp` bindings
  holding the same button concurrently; button ownership is not coordinated.
- Host recognition, USB/BLE behavior and effective scroll amount need hardware
  testing. Adjust the timeout and wheel step for your setup.

## Testing

Run the host tests:

```sh
make -C tests/host test
```

They exercise the behavior source with mocked Zephyr interfaces, covering timing,
direction, input failures and parameter validation. For a compile-only firmware
smoke test in a configured ZMK workspace:

```sh
./tests/firmware/build.sh /absolute/path/to/zmk/app /absolute/path/to/build-directory
```

The fixture targets `seeeduino_xiao_ble` with `settings_reset`; it is not a keymap
for flashing. With CMake 4, older Zephyr versions may need
`-DCMAKE_POLICY_VERSION_MINIMUM=3.5` as an extra script argument.

MIT licensed.
