### Hoverboard-Firmware-Hack-Gen2

Open-source firmware for generation-2 hoverboards with two mainboards (GD32F130C8 per board) instead of sensor boards.

Based on the structure of [Niklas Fauth's hoverboard firmware hack](https://github.com/NiklasFauth/hoverboard-firmware-hack/). The MCU and pinout differ from Gen1 (STM32F103), so this firmware was written from scratch.

This fork adds **sinusoidal commutation**, a **PlatformIO** build, **CI**, and **host unit tests** for the motor control math.

---

#### Hardware

![otter](https://github.com/flo199213/Hoverboard-Firmware-Hack-Gen2/blob/master/Hardware_Overview_small.png)

Two mainboards communicate over USART. LED boards on X1/X2 show battery and error state. An SWD header (GND, SWDIO, SWCLK) is available for flashing.

Schematics: [Schematics/HoverBoard_CoolAndFun.pdf](Schematics/HoverBoard_CoolAndFun.pdf)

| Board | MCU | Role |
|-------|-----|------|
| Master | GD32F130C8 | Steering input, buzzer, battery LEDs, USART to slave |
| Slave | GD32F130C8 | Second motor bridge, USART to master |

Flash **different firmware** on master and slave (see build environments below).

---

#### Quick start (PlatformIO)

**Requirements:** [PlatformIO](https://platformio.org/) (CLI or IDE extension)

```bash
cd HoverBoardGigaDevice
pio run -e GD32F130C8T6_MASTER    # master board, sinusoidal PWM
pio run -e GD32F130C8T6_SLAVE     # slave board, sinusoidal PWM
```

Firmware output:

```
.pio/build/GD32F130C8T6_MASTER/firmware.bin
.pio/build/GD32F130C8T6_SLAVE/firmware.bin
```

Full build options, flashing, configuration, tests, and CI are documented in **[HoverBoardGigaDevice/README.md](HoverBoardGigaDevice/README.md)**.

---

#### Flashing

1. Connect **GND**, **SWDIO**, **SWCLK** to an ST-Link (or compatible SWD programmer).
2. If the chip was never flashed, unlock flash first (STM32 ST-LINK Utility or OpenOCD).
3. **Hold the power button** while flashing — the MCU releases the power latch during reset.
4. Flash master firmware to the master board and slave firmware to the slave board.

With PlatformIO and ST-Link:

```bash
cd HoverBoardGigaDevice
pio run -e GD32F130C8T6_MASTER -t upload    # requires upload_protocol in platformio.ini
```

Default `platformio.ini` uses J-Link; change `upload_protocol` to `stlink` if needed.

**Keil (legacy):** open `HoverBoardGigaDevice/Hoverboard.uvprojx`. Set `MASTER` or `SLAVE` and optionally `SINUSOIDAL` in `Inc/config.h`, then build and flash from Keil.

---

#### Motor commutation

| Mode | PlatformIO env suffix | Description |
|------|----------------------|-------------|
| Sinusoidal (default) | `GD32F130C8T6_MASTER` / `_SLAVE` | Smooth three-phase PWM, Hall-based angle tracking |
| 6-step block | `GD32F130C8T6_MASTER_BLOCK` / `_SLAVE_BLOCK` | Classic trapezoidal commutation |

Sinusoidal logic lives in `Src/bldc_sinusoidal.c` and is covered by host tests (`test/`).

---

#### CI

GitHub Actions (`.github/workflows/build.yml`) on push/PR when firmware code changes (not on docs-only commits):

- Host unit tests (`make -C HoverBoardGigaDevice/test`)
- Static analysis (`pio check`, cppcheck)
- Build MASTER + SLAVE firmware
- Flash/RAM size check (64 KB / 8 KB limits for GD32F130C8)
- Upload `firmware.bin` artifacts

---

#### Project layout

```
HoverBoardGigaDevice/
  Src/           Application source
  Inc/           Headers and config.h
  platformio.ini Build environments
  boards/        Board definition for PIO
  test/          Host unit tests (sinusoidal math)
  scripts/       CI helper scripts
  RTE/           Keil SPL sources (legacy Keil build)
```

---

#### License

GPLv3 — see [LICENSE](LICENSE).
