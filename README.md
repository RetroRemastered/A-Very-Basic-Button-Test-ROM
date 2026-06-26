# A Very Basic Button Test ROM

A tiny Game Boy Advance button-test ROM. It starts with the Retro Remastered splash screen, then switches to a GBA SP-style button tester when any button is pressed.

## Download

A Very Basic Button Test ROM.zip

## Controls

Press any GBA button. The matching control changes to its pressed artwork while held, and a square-wave tone plays while the button is held.

## Sounds

- Left: Do
- Down: Re
- Up: Me
- Right: Fa
- Select: So
- Start: La
- B: Ti
- A: Do
- L: lowest pitch available from the GBA square-wave channel
- R: highest pitch available from the GBA square-wave channel

## Build From Source

Install an ARM embedded toolchain that provides `arm-none-eabi-gcc` and `arm-none-eabi-objcopy`, then run:

```sh
make
```

The ROM will be written to:

```text
build/button_test.gba
```

The build includes `tools/fix-gba-header.js`, so no separate `gbafix` install is required.
