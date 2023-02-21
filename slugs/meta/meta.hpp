#pragma once

#include <cstdint>
#include <cmath>

#include "vsgl.hpp"

inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
    uint16_t s = (int(R>>3) << 11) + (int(G>>2) << 5) + int(B>>3);
    return (s >> 8) | (s << 8);
}

#include <Gamebuino-Meta.h>
#include <SPI.h>
#undef min
#undef max
#undef abs
#undef round

// #define FIXED_POINTS_USE_NAMESPACE
// #define FIXED_POINTS_NO_RANDOM
// #include "../FixedPoints/FixedPoints.h"
// #define FLOAT FixedPoints::SFixed<23, 8>

#include "js.hpp"
#include "general.hpp"

#include "TIC806x6.hpp"
#include "ZXSpec.hpp"
#include "adventurer12x16.hpp"
#include "donut7x10.hpp"
#include "dragon6x8.hpp"
#include "fontC64.hpp"
#include "fontC64UIGfx.hpp"
#include "fontMonkey.hpp"
#include "karateka8x11.hpp"
#include "koubit7x7.hpp"
#include "mini4x6.hpp"
#include "runes6x8.hpp"
#include "tight4x7.hpp"
#include "tiny5x7.hpp"

using vsgl = vsgl_t<160, 128>;

void JSinit();
void JSupdate(uint32_t time, uint32_t updateCount);

extern js::Local D; // var
extern js::Local C; // var
extern js::Local B; // var
extern js::Local A; // var
extern js::Local RIGHT; // var
extern js::Local LEFT; // var
extern js::Local DOWN; // var
extern js::Local UP; // var
extern js::Local FRAMETIME; // var

inline uint32_t startTime;
inline uint32_t updateCount = 0;
inline uint32_t updateFrequency = 1000 / 30;


/* BEGIN PROFILER */
inline const char* volatile profilerSample = "NONE2";
inline uint32_t sampleCount = 1;

//this function gets called by the interrupt at <sampleRate>Hertz
void TC5_Handler (void) {
    sampleCount--;
    if (sampleCount == 0) {
        profilerSample = js::_currentFunction;
        sampleCount = 1000;
    }
    TC5->COUNT16.INTFLAG.bit.MC0 = 1; //Writing a 1 to INTFLAG.bit.MC0 clears the interrupt so that it will run again
}

//Function that is used to check if TC5 is done syncing
//returns true when it is done syncing
bool tcIsSyncing() {
    return TC5->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY;
}

//This function enables TC5 and waits for it to be ready
void tcStartCounter() {
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE; //set the CTRLA register
    while (tcIsSyncing()); //wait until snyc'd
}

//Reset TC5
void tcReset() {
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (tcIsSyncing());
    while (TC5->COUNT16.CTRLA.bit.SWRST);
}

void tcConfigure(int sampleRate) {
    // select the generic clock generator used as source to the generic clock multiplexer
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5)) ;
    while (GCLK->STATUS.bit.SYNCBUSY);

    tcReset(); //reset TC5

    // Set Timer counter 5 Mode to 16 bits, it will become a 16bit counter ('mode1' in the datasheet)
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
    // Set TC5 waveform generation mode to 'match frequency'
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
    //set prescaler
    //the clock normally counts at the GCLK_TC frequency, but we can set it to divide that frequency to slow it down
    //you can use different prescaler divisons here like TC_CTRLA_PRESCALER_DIV1 to get a different range
    TC5->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024 | TC_CTRLA_ENABLE; //it will divide GCLK_TC frequency by 1024
    //set the compare-capture register.
          //The counter will count up to this value (it's a 16bit counter so we use uint16_t)
          //this is how we fine-tune the frequency, make it count to a lower or higher value
          //system clock should be 1MHz (8MHz/8) at Reset by default
          TC5->COUNT16.CC[0].reg = (uint16_t) (SystemCoreClock / sampleRate);
    while (tcIsSyncing());

    // Configure interrupt request
    NVIC_DisableIRQ(TC5_IRQn);
    NVIC_ClearPendingIRQ(TC5_IRQn);
    NVIC_SetPriority(TC5_IRQn, 0);
    NVIC_EnableIRQ(TC5_IRQn);

    // Enable the TC5 interrupt request
    TC5->COUNT16.INTENSET.bit.MC0 = 1;
    while (tcIsSyncing()); //wait until TC5 is done syncing
}

//disable TC5
void tcDisable() {
    TC5->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
    while (tcIsSyncing());
}

void setupProfiler() {
  tcConfigure(333); //configure the timer to run at <sampleRate>Hertz
  tcStartCounter(); //starts the timer
}

/* END PROFILER */

static SPISettings tftSPISettings;
namespace Gamebuino_Meta {
    extern volatile uint32_t dma_desc_free_count;
}
static inline void wait_for_transfers_done(void) {
    while (Gamebuino_Meta::dma_desc_free_count < 3);
}

void setup() {
    PI = 3.1415926535897932384626433f;
    HALF_PI = 3.1415926535897932384626433f / 2;
    TWO_PI = 3.1415926535897932384626433f * 2;

    gb.begin();
    SerialUSB.begin(9600);
    setupProfiler();
    vsgl::font = res::fontMini;
    tftSPISettings = SPISettings(24000000, MSBFIRST, SPI_MODE0);
    FRAMETIME = 1;
    JSinit();
    startTime = millis();
}

void loop() {
    PROFILER;
    gb.buttons.update();
    gb.metaMode.updateButtons();
    gb.checkHomeMenu();
    gb.metaMode.updateAnimations();
    gb.sound.update();

    auto now = millis() - startTime;

    auto targetUpdateCount = updateFrequency ? now / updateFrequency : updateCount + 1;
    auto updateDelta = int32_t(targetUpdateCount - updateCount);
    if (updateDelta < 1)
        return;
    updateCount += updateDelta;

    C = gb.buttons.repeat(Button::menu, 0);
    B = gb.buttons.repeat(Button::b, 0);
    A = gb.buttons.repeat(Button::a, 0);
    RIGHT = gb.buttons.repeat(Button::right, 0);
    LEFT = gb.buttons.repeat(Button::left, 0);
    DOWN = gb.buttons.repeat(Button::down, 0);
    UP = gb.buttons.repeat(Button::up, 0);

    auto startUpdate = millis();

    // vsgl::pen = 1;
    // vsgl::clear();

    {
        PROFILER_NAMED("js");
        JSupdate(now, updateDelta);
    }

    #if ENABLE_PROFILER != 0
    {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d %d %s", int(js::heapSize), int(1000 / js::to<int32_t>(FRAMETIME)), profilerSample);

        // snprintf(tmp, sizeof(tmp), "%d %d %d %d %d %d %s",
        //          js::gcCount, js::freeCount, js::recycleCount, js::heapSize,
        //          updateDelta, 1000 / js::to<int32_t>(FRAMETIME),
        //          profilerSample
        //     );

        js::recycleCount = 0;
        js::gcCount = 0;
        js::freeCount = 0;

        auto oldPen = vsgl::pen;
        vsgl::pen = 0;
        vsgl::text(tmp, 0, 110);
        vsgl::pen = oldPen;
    }
    #endif

    {
        PROFILER_NAMED("flush");

        gb.tft.setAddrWindow(0, 0, 160 - 1, 128 - 1);
        gb.tft.dataMode();

        vsgl::draw([Y=0](const uint8_t* framebuffer) mutable {
            static uint16_t out[160];
            auto fb = framebuffer;
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 160; ++x) {
                    out[x] = palette[*fb++];
                }
                // gb.tft.drawBuffer(0, Y++, out, 160, 1);

                if (Y++) {
                    wait_for_transfers_done();
                    SPI.endTransaction();
                }

                SPI.beginTransaction(tftSPISettings);
                gb.tft.sendBuffer(out, 160);
            }
        });
    }

    wait_for_transfers_done();
    gb.tft.idleMode();
    SPI.endTransaction();

    FRAMETIME = uint32_t{millis() - startUpdate};
}

inline js::Local getTime(js::Local& args, bool) {
    return {millis()};
}

inline js::Local setScreenMode(js::Local& args, bool) {
    return {};
}

inline uint32_t getPenFromArgs(js::Local& args) {
    auto argc = js::to<uint32_t>(js::get(args, V_length));
    if (argc == 1) {
        return js::to<uint32_t>(js::get(args, V_0));
    }

    uint8_t pen[4];
    pen[0] = js::to<uint32_t>(js::get(args, V_0)) >> 3;
    pen[1] = js::to<uint32_t>(js::get(args, V_1)) >> 2;
    pen[2] = js::to<uint32_t>(js::get(args, V_2)) >> 3;

    if (argc >= 4) {
        uint8_t alpha = js::to<uint32_t>(js::get(args, V_3));
        if (alpha < 128)
          return 0;
    }

    uint32_t closest = 0;
    uint32_t closestDistance = -1;
    for (int i = 1; i < 256; ++i) {
        uint32_t packed = palette[i];
        packed = ((packed >> 8) | (packed << 8)) & 0xFFFF;
        int dr = pen[0] - (packed >> 11);
        int dg = pen[1] - ((packed >> 5) & 0x3F);
        int db = pen[2] - (packed & 0x1F);
        uint32_t d = dr*dr + dg*dg + db*db;
        if (d < closestDistance) {
            closest = i;
            closestDistance = d;
        }
    }

    return closest;
}


inline js::Local setPen(js::Local& args, bool) {
    vsgl::pen = getPenFromArgs(args);
    return {vsgl::pen};
}

inline js::Local setLED(js::Local& args, bool) {
    return {};
}

inline js::Local clear(js::Local&, bool) {
    vsgl::clear();
    return {};
}

inline js::Local setFont(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        vsgl::font = (const uint8_t*) *ref;
    }
    return {};
}

inline bool mirrored = false;
inline bool flipped = false;
inline bool transparent = true;

inline const uint8_t* texture;
inline js::Local setTexture(js::Local& args, bool) {
    if (auto ref = std::get_if<js::ResourceRef*>(&js::get(args, V_0))) {
        texture = ((const uint8_t*) *ref);
    } else {
        texture = nullptr;
    }
    return {};
}

inline js::Local setFPS(js::Local& args, bool) {

    return {};
}

inline js::Local setTransparent(js::Local& args, bool) {
    transparent = js::to<bool>(js::get(args, V_0));
    return {};
}

inline js::Local setMirrored(js::Local& args, bool) {
    mirrored = js::to<bool>(js::get(args, V_0));
    return {};
}

inline js::Local setFlipped(js::Local& args, bool) {
    flipped = js::to<bool>(js::get(args, V_0));
    return {};
}

inline js::Local readByte(js::Local& args, bool) {
    auto arg = get(args, V_0);
    auto offset = js::to<int32_t>(js::get(args, V_1));
    if (auto res = std::get_if<js::ResourceRef*>(&arg); res && *res) {
        return {uint32_t(((const unsigned char*)*res)[offset])};
    }
    return {};
}

inline js::Local getWidth(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(((const uint8_t*)*res)[0])};
    }
    return {160};
}

inline js::Local getHeight(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(((const uint8_t*)*res)[1])};
    }
    return {128};
}

inline js::Local rect(js::Local& args, bool) {
    vsgl::rect(
            js::to<int32_t>(js::get(args, V_0)),
            js::to<int32_t>(js::get(args, V_1)),
            js::to<int32_t>(js::get(args, V_2)),
            js::to<int32_t>(js::get(args, V_3))
        );
    return {};
}

OPT_FAST inline js::Local image(js::Local& args, bool) {
    PROFILER;
    auto argc = js::to<uint32_t>(get(args, V_length));
    switch (argc) {
    case 0:
        vsgl::image(texture, 0, 0, mirrored, flipped, transparent);
        break;

    case 1:
        setTexture(args, false);
        vsgl::image(texture, 0, 0, mirrored, flipped, transparent);
        break;

    case 2:
        vsgl::image(texture,
                    js::to<int32_t>(js::get(args, V_0)),
                    js::to<int32_t>(js::get(args, V_1)),
                    mirrored, flipped, transparent);
        break;

    case 3:
        setTexture(args, false);
        vsgl::image(texture,
                    js::to<int32_t>(js::get(args, V_1)),
                    js::to<int32_t>(js::get(args, V_2)),
                    mirrored, flipped, transparent);
        break;

    case 4:
        setTexture(args, false);
        vsgl::rotozoom(
            texture,
            js::to<int32_t>(js::get(args, V_1)),
            js::to<int32_t>(js::get(args, V_2)),
            (float) js::to<js::Float>(js::get(args, V_3)), 1.0f,
            mirrored, flipped, transparent);
        break;

    case 5:
    default:
        setTexture(args, false);
        vsgl::rotozoom(
            texture,
            js::to<int32_t>(js::get(args, V_1)),
            js::to<int32_t>(js::get(args, V_2)),
            (float) js::to<js::Float>(js::get(args, V_3)),
            (float) js::to<js::Float>(js::get(args, V_4)),
            mirrored, flipped, transparent);
        break;
    }
    return {};
}

inline js::Local text(js::Local& args, bool) {
    PROFILER;
    auto str = js::toString(js::get(args, V_0));
    auto x = js::to<int32_t>(js::get(args, V_1));
    auto y = js::to<int32_t>(js::get(args, V_2));
    vsgl::text((char*)str.data(), x, y);
    return {};
}
