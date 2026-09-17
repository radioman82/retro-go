# Retro-Go – ESP32-S3 / nano-s3 Fork

This repository is a personal development fork of Retro-Go focused on the **ESP32-S3 DevKitC-1 N16R8** and a small SPI TFT based retro-gaming system.

**Original project:** Retro-Go
**Fork:** `radioman82/retro-go`
**Development branch:** `nano-s3`

This fork contains my hardware-specific adaptations, configuration changes, emulator work and documentation for the ESP32-S3 platform.

> This is a development fork. Not all components described below are finished or fully integrated.

---

## Hardware

### ESP32-S3

Target board:

* ESP32-S3 DevKitC-1
* N16R8
* ESP32-S3
* 16 MB Flash
* 8 MB PSRAM

### ILI9341 SPI TFT

Current display configuration:

| Signal    |   GPIO |
| --------- | -----: |
| MOSI      | GPIO12 |
| CLK       | GPIO48 |
| DC        | GPIO47 |
| RST       |  GPIO3 |
| Backlight | GPIO39 |

Touch functionality is **not used**.

### SD card

The SD card is connected through SPI:

| Signal |   GPIO |
| ------ | -----: |
| MISO   |  GPIO9 |
| MOSI   | GPIO11 |
| CLK    | GPIO13 |
| CS     | GPIO10 |

The SD card is used for ROMs, disk images and emulator data.

---

## Input

The current button configuration is:

| Function |                GPIO |
| -------- | ------------------: |
| SELECT   |              GPIO16 |
| START    |              GPIO17 |
| MENU     |              GPIO18 |
| OPTION   |               GPIO8 |
| A        |              GPIO15 |
| B        |               GPIO5 |
| D-pad    | GPIO6 / GPIO7 (ADC) |

The input handling has been adapted for the ESP32-S3 target.

---

## Current Development Status

The ESP32-S3 port is actively being developed.

### Working / tested

* ESP32-S3 DevKitC-1 N16R8 target
* Retro-Go builds successfully for ESP32-S3
* ILI9341 SPI display
* SD card support
* ESP32-S3 input/button configuration
* emulator-specific input adaptations
* NGP build completed successfully
* firmware binaries can be generated with ESP-IDF

The current ESP32-S3 build system uses **ESP-IDF** rather than the Arduino build system used by some of the earlier experiments.

---

## Emulator Development

Several emulator components have been tested or integrated on the ESP32-S3.

The development focus is on getting the existing Retro-Go emulators running reliably on the ESP32-S3 while keeping the hardware configuration small and suitable for a handheld-style system.

---

## Amiga 500 Development

An Amiga 500 emulator is currently being investigated for the ESP32-S3.

The starting point is a separate ESP32 project:

`amiga500-esp32`

The existing implementation is being used as a source for the Amiga emulation components rather than rewriting the Amiga emulator from scratch.

The existing ESP32-S3 A500 implementation has already demonstrated:

* video output
* audio output
* button/input handling

The current work is therefore focused on **cleanly integrating the Amiga emulation core into the ESP-IDF/Retro-Go environment**.

### Current A500 status

The A500 emulator is **not yet integrated into Retro-Go**.

An earlier Arduino-based build attempt successfully compiled large parts of the emulator but ultimately failed during linking because the ESP32-S3 internal DRAM region was exceeded.

This is considered a build/memory-layout issue of that port, not proof that Amiga emulation is impossible on the ESP32-S3.

The planned approach is to:

1. reuse the existing Amiga emulation code,
2. separate the emulator core from the Arduino-specific code,
3. use the ESP32-S3 PSRAM for large Amiga memory buffers,
4. build the core using the existing ESP-IDF environment,
5. reuse the already working video, audio and input implementation where appropriate,
6. integrate the result into Retro-Go.

The A500 port is therefore currently **experimental / work in progress**.

---

## Documentation

Additional development documentation is included in:

`Retro go mit esp32s3 R16N8.docx`

This document records the ESP32-S3 Retro-Go development, hardware configuration and experiments carried out during the porting work.

---

## Build

The ESP32-S3 target is built using the ESP-IDF based build system included with this fork.

A successful application build produces an ESP32-S3 binary in the corresponding emulator `build` directory.

For example:

```text
ngp/build/ngp.bin
```

The build output also provides the corresponding `idf.py` / `esptool` commands for flashing the application.

---

## Development Notes

This repository is primarily a development project and should not be considered a finished distribution.

Hardware configuration and emulator support may change as development continues.

The goal is to maintain a documented ESP32-S3 based Retro-Go platform while experimenting with additional emulator support and hardware-specific improvements.

---

## Original Retro-Go

This project is based on Retro-Go.

Please refer to the original Retro-Go documentation and repository for the original project, supported platforms and licensing information.

# Table of contents
- [Description](#description)
- [Installation](#installation)
- [Usage](#usage)
- [Issues](#issues)
- [Development](#development)
- [Acknowledgements](#acknowledgements)
- [License](#license)

# Description
Retro-Go is a firmware to play retro games on ESP32-based devices (officially supported are
ODROID-GO and MRGC-G32, check [this list for other devices](components/retro-go/README.md)).
The project consists of a launcher and half a dozen applications that have been heavily
optimized to reduce their cpu, memory, and flash needs without reducing compatibility!

### Supported systems:
- Nintendo: **NES, SNES (slow), Gameboy, Gameboy Color, Game & Watch**
- Sega: **SG-1000, Master System, Mega Drive / Genesis, Game Gear**
- Coleco: **Colecovision**
- NEC: **PC Engine**
- Atari: **Lynx**
- Others: **DOOM** (including mods!)

### Retro-Go features:
- In-game menu
- Favorites and recently played
- GB color palettes, RTC adjust and save
- NES color palettes, PAL roms, NSF support
- More emulators and applications
- Scaling and filtering options
- Better performance and compatibility
- Turbo Speed/Fast forward
- Customizable launcher
- Cover art and save state previews
- Multiple save slots per game
- Wifi file manager
- And more!

### Screenshots
![Preview](assets/retro-go-preview.jpg)


# Installation

### ODROID-GO
  1. Download `retro-go_1.x_odroid-go.fw` from the [release page](https://github.com/ducalex/retro-go/releases/) and copy it to `/odroid/firmware` on your sdcard.
  2. Power up the device while holding down B.
  3. Select retro-go in the files list and flash it.

### MyRetroGameCase G32 (GBC)
  1. Download `retro-go_1.x_mrgc-g32.fw` from the [release page](https://github.com/ducalex/retro-go/releases/) and copy it to `/espgbc/firmware` on your sdcard.
  2. Power up the device while holding down MENU (the volume knob).
  3. Select retro-go in the files list and flash it.

### Other devices
  1. Download the .img for your device from the [release page](https://github.com/ducalex/retro-go/releases/).
  2. Connect your device to a computer with a USB cable.
  3. Flash the image with esptool:
     - [Command line](https://github.com/espressif/esptool/releases/): Run `esptool.py write_flash --flash_size detect 0x0 retro-go_*.img`
     - [Web version](https://espressif.github.io/esptool-js/): Connect your device, click Erase Flash, then select your .img file and set address to 0x0, finally click Program)

Your particular device may require extra steps (like holding a button during power up) or different esptool flags or a special cable. If the above steps fail, you might need to ask the manufacturer for instructions on how to flash new firmware!

If your device is not already supported or if a prebuilt version isn't available for it you can check the [development section](#Development) for more information on how to build for your device.


# Usage

## Game covers / artwork
Game covers should be placed in the `romart` folder at the base of your sd card. You can obtain a pre-made pack [here](https://github.com/ducalex/retro-go-covers). Retro-Go is also compatible with the older Go-Play romart pack.

You can add missing cover art by creating a PNG image (160x168, 8bit). Two naming schemes are supported:
- Filename-based: `/romart/nes/Super Mario.png` (notice the rom extension is *not* included)
- CRC32-based: `/romart/nes/A/ABCDE123.png` where `nes` is the same as the rom folder, and `ABCDE123` is the CRC32 of the game (press A -> Properties in the launcher to find it), and `A` is the first character of the CRC32

_Note: CRC32-based, which is what is used in the pre-made pack, is much slower than name-based! This type is useful because filenames vary greatly despite having identical CRCs, but if you generate your own art I suggest you use filename-based format and delete all CRC-based art from your SD Card to improve responsiveness._


## BIOS files
Some emulators support loading a BIOS. The files should be placed as follows:
- GB: `/retro-go/bios/gb_bios.bin`
- GBC: `/retro-go/bios/gbc_bios.bin`
- FDS: `/retro-go/bios/fds_bios.bin`
- MSX: In folder `/retro-go/bios/msx/` put: `MSX.ROM` `MSX2.ROM` `MSX2EXT.ROM` `MSX2P.ROM` `MSX2PEXT.ROM` `FMPAC.ROM` `DISK.ROM` `MSXDOS2.ROM` `PAINTER.ROM` `KANJI.ROM`


## Game & Watch
The roms must be packed with [LCD-Game-Shrinker](https://github.com/bzhxx/LCD-Game-Shrinker) and a tutorial can be [found here](https://gist.github.com/DNA64/16fed499d6bd4664b78b4c0a9638e4ef).


## Wifi
To use wifi you will need to create a `/retro-go/config/wifi.json` config file. You can define up to 4 different networks, then selectable in the menu. Its content should look like this:

````json
{
  "ssid0": "my-network",
  "password0": "my-password",
  "ssid1": "my-other-network",
  "password1": "my-password",
  "ssid2": "my-third-network",
  "password2": "my-password",
  "ssid3": "my-last-network",
  "password3": "my-password"
}
````

### Time synchronization
Time synchronization happens in the launcher immediately after a successful connection to the network.
This is done via NTP by contacting `pool.ntp.org` and cannot be disabled at this time.
Timezone can be configured in the launcher's options menu.

### File manager
You can find the IP of your device in the *about* menu of retro-go. Then on your PC navigate to
http://192.168.x.x/ to access the file manager.


## External DAC (headphones)

Retro-Go supports [the external DAC mod for the ODROID-GO](https://github.com/backofficeshow/odroid-go-audio-hat)
which allows high quality audio through headphones. You can switch to it in the menu `Audio Out: Ext DAC`.

<details>
  <summary>Pinout</summary>

  | GO PIN | PCM5102A PIN |
  |--------|---------|
  | 1 | GND |
  | 2 | - |
  | 3 | LCK |
  | 4 | DIN |
  | 5 | BCK |
  | 6 | VIN |
  | 7 | - |
  | 8 | - |
  | 9 | - |
  | 10 | - |
</details>


# Issues

### Black screen / Boot loops
Retro-Go typically detects and resolves application crashes and freezes automatically. However, if you do
get stuck in a boot loop, you can hold `DOWN` while powering up the device to return to the launcher.

### Sound quality
The volume isn't correctly attenuated on the GO, resulting in upper volume levels that are too loud and
lower levels that are distorted due to DAC resolution. A quick way to improve the audio is to cut one
of the speaker wire and add a `33 Ohm (or thereabout)` resistor in series. Soldering is better but not
required, twisting the wires tightly will work just fine.
[A more involved solution can be seen here.](https://wiki.odroid.com/odroid_go/silent_volume)
Alternatively you can use the headphones DAC mod mentioned earlier in this document.

### Game Boy SRAM *(aka Save/Battery/Backup RAM)*
In Retro-Go, save states will provide you with the best and most reliable save experience. That being said, please
read on if you need or want SRAM saves. The SRAM format is compatible with VisualBoyAdvance so it may be used to
import or export saves.

You can configure automatic SRAM saving in the options menu. A longer delay will reduce stuttering at the cost
of losing data when powering down too quickly. Also note that when *resuming* a game, Retro-Go will give priority
to a save state if present.

### ZIP files
Most Retro-Go applications now support ZIP files. ZIP archives should contain only one ROM file and nothing else. ZIP support also depends on available memory and larger ROMs may fail to load on some devices unfortunately.


# Development
If you wish to build or modify Retro-Go, you can find help in the following documents:

- Build instructions in [BUILDING.md](BUILDING.md)
- Theming instructions [THEMING.md](THEMING.md)
- Porting instructions in [PORTING.md](PORTING.md)
- Translating instructions in [LOCALIZATION.md](LOCALIZATION.md)


# Acknowledgements
- The NES/GBC/SMS emulators and base library were originally from the "Triforce" fork of the [official Go-Play firmware](https://github.com/othercrashoverride/go-play) by crashoverride, Nemo1984, and many others.
- The design of the launcher was originally inspired/copied from [pelle7's go-emu](https://github.com/pelle7/odroid-go-emu-launcher).
- PCE-GO is a fork of [HuExpress](https://github.com/kallisti5/huexpress) and [pelle7's port](https://github.com/pelle7/odroid-go-pcengine-huexpress/) was used as reference.
- The Lynx emulator is a port of [libretro-handy](https://github.com/libretro/libretro-handy).
- The SNES emulator is a port of [Snes9x 2005](https://github.com/libretro/snes9x2005).
- The DOOM engine is a port of [PrBoom 2.5.0](http://prboom.sourceforge.net/).
- The Genesis emulator is a port of [Gwenesis](https://github.com/bzhxx/gwenesis/) by bzhxx.
- The Game & Watch emulator is a port of [lcd-game-emulator](https://github.com/bzhxx/lcd-game-emulator) by bzhxx.
- The MSX emulator is a port of [fMSX](https://fms.komkon.org/fMSX/) by Marat Fayzullin.
- PNG support is provided by [lodepng](https://github.com/lvandeve/lodepng/).
- PCE cover art is from [Christian_Haitian](https://github.com/christianhaitian).
- Some icons from [Rokey](https://iconarchive.com/show/seed-icons-by-rokey.html).
- Background images from [es-theme-gbz35](https://github.com/rxbrad/es-theme-gbz35).
- Special thanks to [RGHandhelds](https://www.rghandhelds.com/) and [MyRetroGamecase](https://www.myretrogamecase.com/) for sending me a [G32](https://www.myretrogamecase.com/products/game-mini-g32-esp32-retro-gaming-console-1) device.
- The [ODROID-GO](https://forum.odroid.com/viewtopic.php?f=159&t=37599) community for encouraging the development of retro-go!

# License
Everything in this project is licensed under the [GPLv2 license](COPYING) with the exception of the following components:
- fmsx/components/fmsx (MSX Emulator, custom non-commercial license)
- handy-go/components/handy (Lynx emulator, zlib)
