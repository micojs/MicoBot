const util = require('util');
const exec = util.promisify(require('child_process').exec);
// const { execFile, execSync } = require('child_process');
const fs = require('fs');
const process = require("process");
const afs = require('fs/promises');

const PokittoLibPrefix = './slugs/PokittoLib/Pokitto/';

const blacklist = [
    'POKITTO_XTERNALS/Arduboy/src/ab_logo.c',
    'POKITTO_XTERNALS/Arduboy/src/glcdfont.c',
    'POKITTO_XTERNALS/Arduboy2/src/ab_logo.c',
    'POKITTO_XTERNALS/Arduboy2/src/glcdfont.c',
    'POKITTO_SIM/tinydir/tests/cbehave/cbehave.c',
    'POKITTO_SIM/tinydir/tests/util.c',
    'POKITTO_SIM/tinydir/tests/file_open_test.c',
    'POKITTO_SIM/tinydir/tests/windows_unicode_test.c',
    'POKITTO_SIM/tinydir/samples/random_access_sample.c',
    'POKITTO_SIM/tinydir/samples/interactive_sample.c',
    'POKITTO_SIM/tinydir/samples/iterate_sample.c',
    'POKITTO_SIM/tinydir/samples/list_to_file.c',
    'POKITTO_SIM/tinydir/samples/file_open_sample.c',
    'POKITTO_XTERNALS/ImpulseEngine/Render.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Manifold.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Scene.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Precompiled.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Clock.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Body.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/main.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Collision.cpp',
    'POKITTO_XTERNALS/ArduboyTones/src/ArduboyTones.cpp',
    'POKITTO_XTERNALS/Arduboy/examples/ArduBreakout/breakout_bitmaps.cpp',
    'POKITTO_XTERNALS/Arduboy/src/core/core.cpp',
    'POKITTO_XTERNALS/Arduboy/src/audio/audio.cpp',
    'POKITTO_XTERNALS/Arduboy/src/ab_printer.cpp',
    'POKITTO_XTERNALS/Arduboy/src/Arduboy.cpp',
    'POKITTO_XTERNALS/Arduino/Print.cpp',
    'POKITTO_XTERNALS/Arduino/delay.cpp',
    'POKITTO_XTERNALS/Arduboy2/examples/ArduBreakout/ardubkout.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Sprites.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2Audio.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2Core.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2.cpp',
    'POKITTO_XTERNALS/ArduboyPlaytune/src/ArduboyPlaytune.cpp',
    'POKITTO_SIM/WriteWav.cpp',
    'POKITTO_SIM/Pokitto_simport.cpp',
    'POKITTO_SIM/PokittoSimulator.cpp',
    'POKITTO_SIM/SimSound.cpp',
    'POKITTO_SIM/PokittoBattery.cpp',
    'POKITTO_SIM/Pokitto_joystick.cpp',
    'POKITTO_SIM/tinydir/samples/cpp_sample.cpp',
    'POKITTO_SIM/Pokitto_simsound.cpp',
    'POKITTO_SIM/SimEEPROM.cpp',
    'POKITTO_SIM/SimLCD.cpp',
    'POKITTO_SIM/PokittoSimCore.cpp',
    'POKITTO_SIM/PokittoSimSound.cpp',
    'POKITTO_SIM/mbed_sim/mbed_sim.cpp',
    'POKITTO_SIM/PokittoSimButtons.cpp',
    'POKITTO_SIM/PokittoClock.cpp',
    'POKITTO_LIBS/FileIO/FileIO_SIM.cpp',
    'POKITTO_LIBS/MicroPython/PythonBindings.cpp',
    'POKITTO_LIBS/MicroPython/UsbSerialPrint.cpp',
    'POKITTO_LIBS/MicroPython/main.cpp',
    'POKITTO_LIBS/ImageFormat/BmpImage.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/ff.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/ccsbcs.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/diskio.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATFileHandle.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATFileSystem.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATDirHandle.cpp',
    'POKITTO_LIBS/SDFileSystem/CRC16.cpp',
    'POKITTO_LIBS/SDFileSystem/SDFileSystem.cpp',
    'POKITTO_LIBS/SDFileSystem/CRC7.cpp',
    'POKITTO_LIBS/Physics/Manifold.cpp',
    'POKITTO_LIBS/Physics/Scene.cpp',
    'POKITTO_LIBS/Physics/Clock.cpp',
    'POKITTO_LIBS/Physics/Body.cpp',
    'POKITTO_LIBS/Physics/Collision.cpp'
];

const PokittoLibSources = [
    'POKITTO_HW/timer_11u6x.c',
    'POKITTO_HW/dma_11u6x.c',
    'POKITTO_HW/clock_11u6x.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/us_ticker.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/serial_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/gpio_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/spi_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/i2c_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/gpio_irq_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/pwmout_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/sleep.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/analogin_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/rtc_api.c',
    'mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X/pinmap.c',
    'mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/system_LPC11U6x.c',
    'mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/cmsis_nvic.c',
    'mbed-pokitto/common/mbed_interface.c',
    'mbed-pokitto/common/pinmap_common.c',
    'mbed-pokitto/common/gpio.c',
    'mbed-pokitto/common/board.c',
    'mbed-pokitto/common/wait_api.c',
    'mbed-pokitto/common/semihost_api.c',
    'mbed-pokitto/common/assert.c',
    'mbed-pokitto/common/rtc_time.c',
    'mbed-pokitto/common/error.c',
    'mbed-pokitto/common/ticker_api.c',
    'mbed-pokitto/common/lp_ticker_api.c',
    'mbed-pokitto/common/us_ticker_api.c',
    'POKITTO_XTERNALS/Arduboy/src/ab_logo.c',
    'POKITTO_XTERNALS/Arduboy/src/glcdfont.c',
    'POKITTO_XTERNALS/Arduboy2/src/ab_logo.c',
    'POKITTO_XTERNALS/Arduboy2/src/glcdfont.c',
    'POKITTO_SIM/tinydir/tests/cbehave/cbehave.c',
    'POKITTO_SIM/tinydir/tests/util.c',
    'POKITTO_SIM/tinydir/tests/file_open_test.c',
    'POKITTO_SIM/tinydir/tests/windows_unicode_test.c',
    'POKITTO_SIM/tinydir/samples/random_access_sample.c',
    'POKITTO_SIM/tinydir/samples/interactive_sample.c',
    'POKITTO_SIM/tinydir/samples/iterate_sample.c',
    'POKITTO_SIM/tinydir/samples/list_to_file.c',
    'POKITTO_SIM/tinydir/samples/file_open_sample.c',
    'POKITTO_LIBS/FixMath/fix16_exp.c',
    'POKITTO_LIBS/FixMath/fix16_trig.c',
    'POKITTO_LIBS/FixMath/fix16_sqrt.c',
    'POKITTO_LIBS/FixMath/fract32.c',
    'POKITTO_LIBS/FixMath/uint32.c',
    'POKITTO_LIBS/FixMath/fix16.c',
    'POKITTO_HW/HWLCD.cpp',
    'POKITTO_HW/PokittoHW.cpp',
    'POKITTO_HW/HWButtons.cpp',
    'POKITTO_HW/SoftwareI2C.cpp',
    'POKITTO_HW/iap.cpp',
    'POKITTO_HW/Pokitto_extport.cpp',
    'POKITTO_HW/HWSound.cpp',
    'POKITTO_HW/PokittoClock.cpp',
    'mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM/TARGET_LPC11U68/startup_LPC11U68.cpp',
    'mbed-pokitto/common/I2C.cpp',
    'mbed-pokitto/common/SPISlave.cpp',
    'mbed-pokitto/common/InterruptIn.cpp',
    'mbed-pokitto/common/Timer.cpp',
    'mbed-pokitto/common/CallChain.cpp',
    'mbed-pokitto/common/FileLike.cpp',
    'mbed-pokitto/common/Ethernet.cpp',
    'mbed-pokitto/common/FilePath.cpp',
    'mbed-pokitto/common/retarget.cpp',
    'mbed-pokitto/common/SPI.cpp',
    'mbed-pokitto/common/I2CSlave.cpp',
    'mbed-pokitto/common/Serial.cpp',
    'mbed-pokitto/common/RawSerial.cpp',
    'mbed-pokitto/common/Stream.cpp',
    'mbed-pokitto/common/Ticker.cpp',
    'mbed-pokitto/common/CAN.cpp',
    'mbed-pokitto/common/FileBase.cpp',
    'mbed-pokitto/common/BusIn.cpp',
    'mbed-pokitto/common/BusInOut.cpp',
    'mbed-pokitto/common/InterruptManager.cpp',
    'mbed-pokitto/common/SerialBase.cpp',
    'mbed-pokitto/common/FileSystemLike.cpp',
    'mbed-pokitto/common/LocalFileSystem.cpp',
    'mbed-pokitto/common/Timeout.cpp',
    'mbed-pokitto/common/BusOut.cpp',
    'mbed-pokitto/common/TimerEvent.cpp',
    'POKITTO_CORE/PokittoButtons.cpp',
    'POKITTO_CORE/PokittoConsole.cpp',
    'POKITTO_CORE/PokittoDisplay.cpp',
    'POKITTO_CORE/PokittoSound.cpp',
    'POKITTO_CORE/PokittoPrintf.cpp',
    'POKITTO_CORE/PokittoCore.cpp',
    'POKITTO_CORE/PokittoDisk.cpp',
    'POKITTO_CORE/PokittoCookie.cpp',
    'POKITTO_CORE/PokittoPalette.cpp',
    'POKITTO_CORE/PokittoBattery.cpp',
    'POKITTO_CORE/PokittoFramebuffer.cpp',
    'POKITTO_CORE/PokittoItoa.cpp',
    'POKITTO_CORE/FONTS/font5x7.cpp',
    'POKITTO_CORE/FONTS/karateka8x11.cpp',
    'POKITTO_CORE/FONTS/font3x3.cpp',
    'POKITTO_CORE/FONTS/font3x5.cpp',
    'POKITTO_CORE/FONTS/runes6x8.cpp',
    'POKITTO_CORE/FONTS/adventurer12x16.cpp',
    'POKITTO_CORE/FONTS/tight4x7.cpp',
    'POKITTO_CORE/FONTS/dragon6x8.cpp',
    'POKITTO_CORE/FONTS/koubit7x7.cpp',
    'POKITTO_CORE/FONTS/donut7x10.cpp',
    'POKITTO_CORE/FONTS/fontMonkey.cpp',
    'POKITTO_CORE/FONTS/TIC806x6.cpp',
    'POKITTO_CORE/FONTS/fontC64.cpp',
    'POKITTO_CORE/FONTS/tiny5x7.cpp',
    'POKITTO_CORE/FONTS/fontC64UIGfx.cpp',
    'POKITTO_CORE/FONTS/ZXSpec.cpp',
    'POKITTO_CORE/FONTS/mini4x6.cpp',
    'POKITTO_CORE/PokittoLogos.cpp',
    'POKITTO_CORE/PokittoBacklight.cpp',
    'POKITTO_CORE/PALETTES/palRainbow.cpp',
    'POKITTO_CORE/PALETTES/palDefault.cpp',
    'POKITTO_CORE/PALETTES/palPico.cpp',
    'POKITTO_CORE/PALETTES/palMagma.cpp',
    'POKITTO_CORE/PALETTES/palMono.cpp',
    'POKITTO_CORE/PALETTES/palGameboy.cpp',
    'POKITTO_CORE/PALETTES/palDB16.cpp',
    'POKITTO_CORE/PALETTES/palCGA.cpp',
    'POKITTO_CORE/PALETTES/palZXSpec.cpp',
    'POKITTO_CORE/PALETTES/palAction.cpp',
    'POKITTO_CORE/TASMODE.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Render.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Manifold.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Scene.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Precompiled.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Clock.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Body.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/main.cpp',
    'POKITTO_XTERNALS/ImpulseEngine/Collision.cpp',
    'POKITTO_XTERNALS/ArduboyTones/src/ArduboyTones.cpp',
    'POKITTO_XTERNALS/Arduboy/examples/ArduBreakout/breakout_bitmaps.cpp',
    'POKITTO_XTERNALS/Arduboy/src/core/core.cpp',
    'POKITTO_XTERNALS/Arduboy/src/audio/audio.cpp',
    'POKITTO_XTERNALS/Arduboy/src/ab_printer.cpp',
    'POKITTO_XTERNALS/Arduboy/src/Arduboy.cpp',
    'POKITTO_XTERNALS/Arduino/Print.cpp',
    'POKITTO_XTERNALS/Arduino/delay.cpp',
    'POKITTO_XTERNALS/Arduboy2/examples/ArduBreakout/ardubkout.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Sprites.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2Audio.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2Core.cpp',
    'POKITTO_XTERNALS/Arduboy2/src/Arduboy2.cpp',
    'POKITTO_XTERNALS/ArduboyPlaytune/src/ArduboyPlaytune.cpp',
    'libpff/mmc.cpp',
    'libpff/pff.cpp',
    'POKITTO_SIM/WriteWav.cpp',
    'POKITTO_SIM/Pokitto_simport.cpp',
    'POKITTO_SIM/PokittoSimulator.cpp',
    'POKITTO_SIM/SimSound.cpp',
    'POKITTO_SIM/PokittoBattery.cpp',
    'POKITTO_SIM/Pokitto_joystick.cpp',
    'POKITTO_SIM/tinydir/samples/cpp_sample.cpp',
    'POKITTO_SIM/Pokitto_simsound.cpp',
    'POKITTO_SIM/SimEEPROM.cpp',
    'POKITTO_SIM/SimLCD.cpp',
    'POKITTO_SIM/PokittoSimCore.cpp',
    'POKITTO_SIM/PokittoSimSound.cpp',
    'POKITTO_SIM/mbed_sim/mbed_sim.cpp',
    'POKITTO_SIM/PokittoSimButtons.cpp',
    'POKITTO_SIM/PokittoClock.cpp',
    'POKITTO_LIBS/JoyHat/JoyHat.cpp',
    'POKITTO_LIBS/FileIO/FileIO_SIM.cpp',
    'POKITTO_LIBS/File/ChaN/ff.cpp',
    'POKITTO_LIBS/File/ChaN/ccsbcs.cpp',
    'POKITTO_LIBS/File/ChaN/diskio.cpp',
    'POKITTO_LIBS/MicroPython/PythonBindings.cpp',
    'POKITTO_LIBS/MicroPython/UsbSerialPrint.cpp',
    'POKITTO_LIBS/MicroPython/main.cpp',
    'POKITTO_LIBS/USBDevice/USBMSD/USBMSD.cpp',
    'POKITTO_LIBS/USBDevice/USBHID/USBHID.cpp',
    'POKITTO_LIBS/USBDevice/USBHID/USBKeyboard.cpp',
    'POKITTO_LIBS/USBDevice/USBHID/USBMouseKeyboard.cpp',
    'POKITTO_LIBS/USBDevice/USBHID/USBMouse.cpp',
    'POKITTO_LIBS/USBDevice/USBMIDI/USBMIDI.cpp',
    'POKITTO_LIBS/USBDevice/USBSerial/USBCDC.cpp',
    'POKITTO_LIBS/USBDevice/USBSerial/USBSerial.cpp',
    'POKITTO_LIBS/USBDevice/USBAudio/USBAudio.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBDevice.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_Maxim.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_LPC11U.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_RZ_A1H.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_LPC40.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_KL25Z.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_STM32F4.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_EFM32.cpp',
    'POKITTO_LIBS/USBDevice/USBDevice/USBHAL_LPC17.cpp',
    'POKITTO_LIBS/Synth/Synth_wavefuncs.cpp',
    'POKITTO_LIBS/Synth/Synth_oscfuncs.cpp',
    'POKITTO_LIBS/Synth/Synth_mixfuncs.cpp',
    'POKITTO_LIBS/Synth/Synth.cpp',
    'POKITTO_LIBS/Synth/Synth_songfuncs.cpp',
    'POKITTO_LIBS/Synth/Synth_envfuncs.cpp',
    'POKITTO_LIBS/Synth/Synth_helpers.cpp',
    'POKITTO_LIBS/PokittoTUI/tasui_UI.cpp',
    'POKITTO_LIBS/ImageFormat/BmpImage.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/ff.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/ccsbcs.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/ChaN/diskio.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATFileHandle.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATFileSystem.cpp',
    'POKITTO_LIBS/SDFileSystem/FATFileSystem/FATDirHandle.cpp',
    'POKITTO_LIBS/SDFileSystem/CRC16.cpp',
    'POKITTO_LIBS/SDFileSystem/SDFileSystem.cpp',
    'POKITTO_LIBS/SDFileSystem/CRC7.cpp',
    'POKITTO_LIBS/Physics/Manifold.cpp',
    'POKITTO_LIBS/Physics/Scene.cpp',
    'POKITTO_LIBS/Physics/Clock.cpp',
    'POKITTO_LIBS/Physics/Body.cpp',
    'POKITTO_LIBS/Physics/Collision.cpp',
    'POKITTO_LIBS/Tilemap/Tilemap.cpp',
    'POKITTO_HW/asm/mode13.s',
    'POKITTO_HW/asm/flushLine2X.s',
    'POKITTO_HW/asm/mode1c.s',
    'POKITTO_HW/asm/pixelCopy.s',
    'POKITTO_HW/asm/unlz4.s',
    'POKITTO_HW/asm/pixelExpand.s',
    'POKITTO_HW/asm/mode13c.s',
    'POKITTO_HW/asm/mode15.s',
    'POKITTO_HW/asm/pixelCopySolid.s',
    'POKITTO_HW/asm/mode2.s',
    'POKITTO_HW/asm/mode15c.s',
    'POKITTO_HW/asm/mode64.s',
    'POKITTO_HW/asm/mode64c.s',
    'POKITTO_HW/asm/pixelCopyMirror.s',
    'POKITTO_HW/asm/flushLine.s',
    'POKITTO_HW/asm/mode2c.s',
    'POKITTO_HW/asm/mode1.s'
].filter(file => blacklist.indexOf(file) == -1).map(src => PokittoLibPrefix + src);

const FLAGS = [
    '-I./slugs/pokitto',
    '-I./slugs/PokittoLib/Pokitto',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_CORE',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_CORE/FONTS',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_CORE/PALETTES',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_HW',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/PokittoTUI',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/PokittoTUI/UITILESETS',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/Tilemap',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/MemOps',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibAudio',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibLog',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibEntity',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibHotswap',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibSchedule',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/LibTrig',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/File',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/ImageFormat',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/Synth',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/USBDevice',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/USBDevice/USBDevice',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_LIBS/USBDevice/USBSerial',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_XTERNALS',
    '-I./slugs/PokittoLib/Pokitto/POKITTO_XTERNALS/Arduino',
    '-I./slugs/PokittoLib/Pokitto/libpff',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/api',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/common',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/hal',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis/TARGET_NXP',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM/TARGET_LPC11U68',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/cmsis/TOOLCHAIN_GCC',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/hal',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/hal/TARGET_NXP',
    '-I./slugs/PokittoLib/Pokitto/mbed-pokitto/targets/hal/TARGET_NXP/TARGET_LPC11U6X'
];

const C_FLAGS=[
    '-c',
    '-Wall',
    '-Wextra',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-fmessage-length=0',
    '-fno-exceptions',
    '-fno-builtin',
    '-ffunction-sections',
    '-fdata-sections',
    '-funsigned-char',
    '-MMD',
    '-fno-delete-null-pointer-checks',
    '-fomit-frame-pointer',
    '-O3',
    '-g1',
    '-DMBED_RTOS_SINGLE_THREAD',
    '-mcpu=cortex-m0plus',
    '-mthumb',
    '-D_OSCT=2',
    '-std=gnu99',
    '-DTARGET_LPC11U68',
    '-D__MBED__=1',
    '-DDEVICE_I2CSLAVE=1',
    '-DTARGET_LIKE_MBED',
    '-DTARGET_NXP',
    '-D__MBED_CMSIS_RTOS_CM',
    '-DDEVICE_RTC=1',
    '-DTOOLCHAIN_object',
    '-D__CMSIS_RTOS',
    '-DTOOLCHAIN_GCC',
    '-DTARGET_CORTEX_M',
    '-DTARGET_M0P',
    '-DTARGET_UVISOR_UNSUPPORTED',
    '-DMBED_BUILD_TIMESTAMP=1526086019.89',
    '-DDEVICE_SERIAL=1',
    '-DDEVICE_INTERRUPTIN=1',
    '-DTARGET_LPCTarget',
    '-DTARGET_CORTEX',
    '-DDEVICE_I2C=1',
    '-D__CORTEX_M0PLUS',
    '-DTARGET_FF_ARDUINO',
    '-DTARGET_RELEASE',
    '-DARM_MATH_CM0PLUS',
    '-DTARGET_LPC11U6X',
    '-DDEVICE_SLEEP=1',
    '-DTOOLCHAIN_GCC_ARM',
    '-DDEVICE_SPI=1',
    '-DDEVICE_ANALOGIN=1',
    '-DDEVICE_PWMOUT=1',
    '-DTARGET_LIKE_CORTEX_M0',
    '-DPOKITTO'
];

const CXX_FLAGS=[
    '-std=c++17',
    '-fno-rtti',
    '-Wvla',
    '-c',
    '-Wall',
    '-Wextra',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-unused-value',
    '-fmessage-length=0',
    '-fno-exceptions',
    '-fno-builtin',
    '-ffunction-sections',
    '-fdata-sections',
    '-funsigned-char',
    '-MMD',
    '-fno-delete-null-pointer-checks',
    '-fomit-frame-pointer',
    '-O3',
    '-g1',
    '-DMBED_RTOS_SINGLE_THREAD',
    '-mcpu=cortex-m0plus',
    '-mthumb',
    '-D_OSCT=2',
    '-DTARGET_LPC11U68',
    '-D__MBED__=1',
    '-DDEVICE_I2CSLAVE=1',
    '-DTARGET_LIKE_MBED',
    '-DTARGET_NXP',
    '-D__MBED_CMSIS_RTOS_CM',
    '-DDEVICE_RTC=1',
    '-DTOOLCHAIN_object',
    '-D__CMSIS_RTOS',
    '-DTOOLCHAIN_GCC',
    '-DTARGET_CORTEX_M',
    '-DTARGET_M0P',
    '-DTARGET_UVISOR_UNSUPPORTED',
    '-DMBED_BUILD_TIMESTAMP=1526086019.89',
    '-DDEVICE_SERIAL=1',
    '-DDEVICE_INTERRUPTIN=1',
    '-DTARGET_LPCTarget',
    '-DTARGET_CORTEX',
    '-DDEVICE_I2C=1',
    '-D__CORTEX_M0PLUS',
    '-DTARGET_FF_ARDUINO',
    '-DTARGET_RELEASE',
    '-DARM_MATH_CM0PLUS',
    '-DTARGET_LPC11U6X',
    '-DDEVICE_SLEEP=1',
    '-DTOOLCHAIN_GCC_ARM',
    '-DDEVICE_SPI=1',
    '-DDEVICE_ANALOGIN=1',
    '-DDEVICE_PWMOUT=1',
    '-DTARGET_LIKE_CORTEX_M0'
];

const ASM_FLAGS=[
    "-mcpu=cortex-m0plus",
    "-mthumb"
];

function obj(src){
    // console.log(src);
    return src + '.o';
}

// function dependencies(src){
//     var lines;
//     try {
//         lines = fs.readFileSync(src.replace(/.*\/([^\/]+)/, '$1').replace(/\.[^.]+$/i, '.d'), 'utf-8').split(/\s+\\\s+|\s+/);
//     } catch(ex){
//         return [];
//     }
//     lines.shift();

//     return lines.map(line => line.trim()).filter(line => line.length > 0);
// }

let ASMSOURCES;
let CXXSOURCES;
let CSOURCES;

const prefix = "";
const CC = prefix + 'arm-none-eabi-gcc';
const AS = prefix + 'arm-none-eabi-as';
const CPP = prefix + 'arm-none-eabi-g++';
const LD = prefix + 'arm-none-eabi-gcc';
const ELF2BIN =  prefix + 'arm-none-eabi-objcopy';
const LINKER_SCRIPT = PokittoLibPrefix + 'mbed-pokitto/targets/cmsis/TARGET_NXP/TARGET_LPC11U6X/TOOLCHAIN_GCC_ARM/TARGET_LPC11U68/LPC11U68.ld';
const LD_FLAGS ='-Wl,--gc-sections -Wl,--wrap,main -Wl,--wrap,_memalign_r -Wl,-n --specs=nano.specs -mcpu=cortex-m0plus -mthumb';
const LD_SYS_LIBS ='-Wl,--start-group -lstdc++ -lsupc++ -lm -lc -lgcc -lnosys  -Wl,--end-group';

async function getSources(project, library) {
    ASMSOURCES = library ? PokittoLibSources.filter(file => /\.s$/.test(file)) : [];
    CXXSOURCES = library ? PokittoLibSources.filter(file => /\.cpp$/.test(file)) : [];
    CSOURCES = library ? PokittoLibSources.filter(file => /\.c$/.test(file)) : [];
    if (project) {
        (await afs.readdir("./slugs/pokitto/")).forEach(file => {
            if (/\.cpp$/i.test(file)) {
                CXXSOURCES.push("./slugs/pokitto/" + file);
            }
        });
    }
}

async function doBuild() {
    let forceCompile = [];

    for (let src of CSOURCES) {
        let outFile = obj(src);
        // if( forceCompile.indexOf(src) == -1 && !cleanBuild && fs.existsSync(outFile) )
        //     return;

        await exec([
            CC,
            C_FLAGS.join(" "),
            FLAGS.join(" "),
            "-o", outFile,
            src
        ].join(" "));
    }


    for (let src of ASMSOURCES) {
        let outFile = obj(src);
        // if( forceCompile.indexOf(src) == -1 && !cleanBuild && fs.existsSync(outFile) )
        //     return;

        await exec([
            AS,
            ASM_FLAGS.join(" "),
            FLAGS.join(" "),
            "-o", outFile,
            src
        ].join(" "));
    }

    for (let src of CXXSOURCES) {
        let outFile = obj(src);
        let outStat = null;

        // if (!cleanBuild) {
        //     var stating = '?';
        //     try {
        //         outStat = afs.stat(stating = outFile);
        //         for (let dep of dependencies(src)) {
        //             const srcStat = fs.statSync(stating = dep);
        //             if (srcStat.mtimeMs > outStat.mtimeMs) {
        //                 outStat = null;
        //                 break;
        //             }
        //         }
        //     }catch(ex){
        //         outStat = null;
        //         throw ex + stating + "[";
        //     }
        // }

        // if (outStat)
        //     return;

        await exec([
            CPP,
            CXX_FLAGS.join(" "),
            FLAGS.join(" "),
            "-o", outFile,
            src
        ].join(" "));
    }
}

async function link() {
    await exec([
        LD,
        LD_FLAGS,
        '-T', LINKER_SCRIPT,
        '--output ./slugs/pokitto/build.elf',
        CSOURCES.map(obj).join(" "),
        CXXSOURCES.map(obj).join(" "),
        ASMSOURCES.map(obj).join(" "),
        LD_SYS_LIBS
    ].join(" "));

    await exec([
        ELF2BIN,
        "-O binary",
        "./slugs/pokitto/build.elf",
        "./slugs/pokitto/build.bin"
    ].join(" "));
}

async function go(lib) {
    if (lib) {
        console.log("Compiling lib");
        await getSources(false, true);
        await doBuild();
    }

    console.log("Compiling game");
    await getSources(true, false);
    await doBuild();

    console.log("Linking");
    await getSources(true, true);
    await link();
}

// go(false);
module.exports.PokittoBuilder = go;
