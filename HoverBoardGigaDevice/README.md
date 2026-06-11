# HoverBoardGigaDevice — firmware build guide

Firmware for the GD32F130C8 hoverboard mainboard. **PlatformIO is the recommended build system.**

## Requirements

- [PlatformIO](https://platformio.org/install/cli) 6.x (installs ARM GCC and SPL framework automatically)
- For flashing: ST-Link, J-Link, or Black Magic Probe (configure in `platformio.ini`)
- For host tests only: `gcc`, `make`

## Build environments

All environments are defined in `platformio.ini`.

| Environment | Board role | Commutation | `build_flags` |
|-------------|------------|-------------|---------------|
| `GD32F130C8T6_MASTER` | Master | Sinusoidal | `-DMASTER -DSINUSOIDAL` |
| `GD32F130C8T6_SLAVE` | Slave | Sinusoidal | `-DSLAVE -DSINUSOIDAL` |
| `GD32F130C8T6_MASTER_BLOCK` | Master | 6-step block | `-DMASTER` |
| `GD32F130C8T6_SLAVE_BLOCK` | Slave | 6-step block | `-DSLAVE` |
| `GD32F130C8T6` | Master (alias) | Sinusoidal | extends `GD32F130C8T6_MASTER` |

```bash
# Default (sinusoidal)
pio run -e GD32F130C8T6_MASTER
pio run -e GD32F130C8T6_SLAVE

# Classic block commutation
pio run -e GD32F130C8T6_MASTER_BLOCK
pio run -e GD32F130C8T6_SLAVE_BLOCK

# Build all four
pio run -e GD32F130C8T6_MASTER -e GD32F130C8T6_SLAVE \
        -e GD32F130C8T6_MASTER_BLOCK -e GD32F130C8T6_SLAVE_BLOCK
```

Artifacts:

```
.pio/build/<env>/firmware.bin
.pio/build/<env>/firmware.elf
```

## Configuration

### Build-time (PlatformIO)

Set in `platformio.ini` per environment:

| Flag | Effect |
|------|--------|
| `-DMASTER` | Master board firmware |
| `-DSLAVE` | Slave board firmware |
| `-DSINUSOIDAL` | Enable sinusoidal commutation in `bldc.c` |

`MASTER`/`SLAVE` and `SINUSOIDAL` are **not** hard-coded in `config.h` when using PlatformIO.

### Runtime parameters (`Inc/config.h`)

Tunable constants (same for all PIO environments unless you add custom `build_flags`):

| Define | Default | Description |
|--------|---------|-------------|
| `PWM_FREQ` | 16000 | PWM frequency (Hz) |
| `DEAD_TIME` | 60 | PWM dead time |
| `DC_CUR_LIMIT` | 15 | DC current limit (A) |
| `TIMEOUT_MS` | 2000 | Steering timeout before PWM off |
| `FILTER_SHIFT` | 12 | PWM low-pass filter strength |
| `BAT_LOW_LVL1/2/DEAD` | 35/33/31 V | Battery warning thresholds (master) |

### Keil (legacy)

Edit `Inc/config.h` directly:

- Uncomment `#define SLAVE` **or** leave default `MASTER`
- Uncomment `#define SINUSOIDAL` for sinusoidal mode

Open `Hoverboard.uvprojx` in Keil µVision and build.

## Flashing

**GD32F130C8:** 64 KB flash, 8 KB RAM.

1. SWD wiring: GND, SWDIO, SWCLK.
2. Hold the board power button during connect/flash.
3. Flash master env to master PCB, slave env to slave PCB.

```bash
pio run -e GD32F130C8T6_MASTER -t upload
pio run -e GD32F130C8T6_SLAVE -t upload
```

Adjust `upload_protocol` and `debug_tool` in `platformio.ini` for your probe (default: `jlink`).

## Testing

### Host unit tests

Pure C tests for sine LUT, Hall tables, angle tracking, and PWM calculation (`Src/bldc_sinusoidal.c`):

```bash
make -C test
# or: make -C test clean test
```

### Static analysis

```bash
pio check -e GD32F130C8T6_MASTER --fail-on-defect=high
```

Checks project `Src/` and `Inc/` only (framework packages skipped).

### Firmware size check

```bash
pio run -e GD32F130C8T6_MASTER
python3 scripts/check_firmware_size.py .pio/build/GD32F130C8T6_MASTER/firmware.elf
```

Fails if flash > 65536 bytes or RAM > 8192 bytes.

## CI

Workflow: [`.github/workflows/build.yml`](../.github/workflows/build.yml)

Runs only when firmware-related paths change (`Src/`, `Inc/`, `test/`, `platformio.ini`, etc.). Commits that touch only `README.md`, schematics, or other docs **do not** trigger the workflow.

| Job | What it runs |
|-----|----------------|
| `host-test` | `make -C HoverBoardGigaDevice/test` |
| `static-analysis` | `pio check -e GD32F130C8T6_MASTER` |
| `build` | `pio run` for MASTER + SLAVE, size check, artifact upload |

## Source layout

| Path | Purpose |
|------|---------|
| `Src/bldc.c` | Motor control loop, ADC, Hall, PWM output |
| `Src/bldc_sinusoidal.c` | Sine LUT, Hall→angle tables, sinusoidal PWM math |
| `Inc/bldc_sinusoidal.h` | Public API for sinusoidal module (host-testable) |
| `Inc/config.h` | Motor and application constants |
| `boards/gd32f130c8.json` | PlatformIO board definition |
| `Inc/gd32f1x0_libopt.h` | SPL peripheral include list (required by framework) |
| `add_nanolib.py` | PIO extra script: link with newlib-nano |
| `RTE/` | Keil SPL driver sources (not used by PlatformIO build) |

## Dependencies (automatic)

PlatformIO downloads on first build:

- `platform = ststm32`
- `framework-spl` via `maxgerhardt/framework-spl@2.10302.0`

No manual CMSIS/SDK install required for PIO builds.

## Ignored paths (`.gitignore`)

- `.pio/` — PlatformIO build cache
- `build/` — local Makefile artifacts (if used)
- `test/test_bldc_sinusoidal` — compiled host test binary
