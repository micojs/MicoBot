#pragma once

#include <cstdint>
#include <32blit.hpp>

#include "js.hpp"
#include "vsgl.hpp"

// inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
//     uint16_t s = (int(R>>3) << 11) + (int(G>>2) << 5) + int(B>>3);
//     return (s >> 8) | (s << 8);
// }
inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
    return (int(B>>3) << 11) + (int(G>>2) << 5) + int(R>>3);
}

#define PROGMEM
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
#undef PROGMEM

using vsgl = vsgl_t<240, 240>;

void JSinit();
void JSupdate(uint32_t time, uint32_t updateCount);
void JSrender(uint32_t time);

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

void setupProfiler() {
    // tcConfigure(333); //configure the timer to run at <sampleRate>Hertz
}
/* END PROFILER */


void init() {
    PI = 3.1415926535897932384626433f;
    HALF_PI = 3.1415926535897932384626433f / 2;
    TWO_PI = 3.1415926535897932384626433f * 2;

    setupProfiler();
    vsgl::font = res::fontMini;
    blit::set_screen_mode(blit::ScreenMode::hires);

    FRAMETIME = 1;
    JSinit();
    startTime = blit::now();
}

int32_t updateDelta;
uint32_t startUpdate;

void update(uint32_t) {
    PROFILER;
    auto time = blit::now();
    auto now = time - startTime;
    auto targetUpdateCount = updateFrequency ? now / updateFrequency : updateCount + 1;
    updateDelta = int32_t(targetUpdateCount - updateCount);
    if (updateDelta < 1)
        return;
    updateCount += updateDelta;

    D = !!(blit::buttons.state & blit::Button::Y);
    C = !!(blit::buttons.state & blit::Button::X);
    B = !!(blit::buttons.state & blit::Button::B);
    A = !!(blit::buttons.state & blit::Button::A);
    RIGHT = !!(blit::buttons.state & blit::Button::DPAD_RIGHT);
    LEFT = !!(blit::buttons.state & blit::Button::DPAD_LEFT);
    DOWN = !!(blit::buttons.state & blit::Button::DPAD_DOWN);
    UP = !!(blit::buttons.state & blit::Button::DPAD_UP);

    startUpdate = time;

    // vsgl::pen = 1;
    // vsgl::clear();

    {
        js::Profiler prof("js");
        JSupdate(time, updateDelta);
    }

}

void render(uint32_t) {
    auto time = blit::now();
    JSrender(updateDelta);

    #if ENABLE_PROFILER != 0
    {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d %d %d %d %d %d %s",
                 js::gcCount, js::freeCount, js::recycleCount, js::heapSize,
                 updateDelta, 1000 / js::to<int32_t>(FRAMETIME),
                 profilerSample
            );
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
        js::Profiler prof("draw");
        auto ptr = (uint16_t*) blit::screen.ptr(0, 0);
        vsgl::draw([&ptr](const uint8_t* framebuffer) {
            for (int i = 0; i < 240 * 8; ++i)
                    *ptr++ = palette[*framebuffer++];
        });
    }

    FRAMETIME = uint32_t{time - startUpdate};
}

inline js::Local setScreenMode(js::Local& args, bool) {
    return {};
}

inline js::Local getTime(js::Local& args, bool) {
    return {blit::now()};
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
        // packed = ((packed >> 8) | (packed << 8)) & 0xFFFF;
        int db = pen[2] - (packed >> 11);
        int dg = pen[1] - ((packed >> 5) & 0x3F);
        int dr = pen[0] - (packed & 0x1F);
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
    auto argc = js::to<uint32_t>(js::get(args, V_length));
    if (argc == 1) {
        auto index = js::to<uint32_t>(js::get(args, V_0));
        uint32_t packed = palette[index % 256];
        blit::LED.r = (packed >> 11) << 3;
        blit::LED.g = ((packed >> 5) & 0x3F) << 2;
        blit::LED.b = (packed & 0x1F) << 3;
    } else {
        blit::LED.r = js::to<uint32_t>(js::get(args, V_0));
        blit::LED.g = js::to<uint32_t>(js::get(args, V_1));
        blit::LED.b = js::to<uint32_t>(js::get(args, V_2));
    }
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
    return {240};
}

inline js::Local getHeight(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(((const uint8_t*)*res)[1])};
    }
    return {240};
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

inline js::Local image(js::Local& args, bool) {
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
            js::to<float>(js::get(args, V_3)), 1.0f,
            mirrored, flipped, transparent);
        break;

    case 5:
    default:
        setTexture(args, false);
        vsgl::rotozoom(
            texture,
            js::to<int32_t>(js::get(args, V_1)),
            js::to<int32_t>(js::get(args, V_2)),
            js::to<float>(js::get(args, V_3)),
            js::to<float>(js::get(args, V_4)),
            mirrored, flipped, transparent);
        break;
    }
    return {};
}

inline js::Local text(js::Local& args, bool) {
    auto str = js::toString(js::get(args, V_0));
    auto x = js::to<int32_t>(js::get(args, V_1));
    auto y = js::to<int32_t>(js::get(args, V_2));
    vsgl::text((char*)str.data(), x, y);
    return {};
}












/* inline js::Local setScreenMode(js::Local& args, bool) { */
/*     blit::ScreenMode mode; */
/*     auto arg = js::get(args, V_0); */
/*     auto str = js::toString(arg); */
/*     if (str == V_lores) { */
/*         mode = blit::ScreenMode::lores; */
/*     } else if (str == V_hires) { */
/*         mode = blit::ScreenMode::hires; */
/*     } else if (str == V_hires_palette) { */
/*         mode = blit::ScreenMode::hires_palette; */
/*     } else { */
/*         PRINT("Invalid screen mode"); */
/*         PRINTLN(); */
/*         return {}; */
/*     } */
/*     blit::set_screen_mode(mode); */
/*     return {}; */
/* } */

/* inline blit::Pen getPen(js::Local& args, bool) { */
/*     blit::Pen pen{255}; */
/*     auto argc = js::to<uint32_t>(js::get(args, V_length)); */
/*     pen.r = js::to<uint32_t>(js::get(args, V_0)); */
/*     pen.g = js::to<uint32_t>(js::get(args, V_1)); */
/*     pen.b = js::to<uint32_t>(js::get(args, V_2)); */
/*     if (argc >= 4) { */
/*         pen.a = js::to<uint32_t>(js::get(args, V_3)); */
/*     } */
/*     return pen; */
/* } */

/* inline js::Local setPen(js::Local& args, bool) { */
/*     blit::screen.pen = getPen(args, false); */
/*     return {}; */
/* } */

/* inline js::Local setLED(js::Local& args, bool) { */
/*     auto pen = getPen(args, false); */
/*     blit::LED.r = pen.g; */
/*     blit::LED.g = pen.r; */
/*     blit::LED.b = pen.b; */
/*     return {}; */
/* } */

/* inline js::Local clear(js::Local&, bool) { */
/*     blit::screen.clear(); */
/*     return {}; */
/* } */

/* inline const blit::Font* font = &blit::minimal_font; */

/* inline js::Local setFont(js::Local& args, bool) { */
/*     auto name = js::toString(js::get(args, V_0)); */
/*     /\* if (name == V_minimal) { *\/ */
/*     /\*     font = &blit::minimal_font; *\/ */
/*     /\* } *\/ */
/*     return {}; */
/* } */

/* /\* inline js::ResourceRef texture; *\/ */

/* inline js::Local setTexture(js::Local& args, bool) { */
/*     /\* if (auto ref = std::get_if<js::ResourceRef>(&js::get(args, V_0))) { *\/ */
/*     /\*     texture = *ref; *\/ */
/*     /\* } else { *\/ */
/*     /\*     texture.data = nullptr; *\/ */
/*     /\*     texture.size = 0; *\/ */
/*     /\* } *\/ */
/* } */

/* inline js::Local text(js::Local& args, bool) { */
/*     auto str = js::toString(js::get(args, V_0)); */
/*     auto x = js::to<int32_t>(js::get(args, V_1)); */
/*     auto y = js::to<int32_t>(js::get(args, V_2)); */
/*     blit::screen.text((char*)str.data(), *font, blit::Point(x, y)); */
/*     return {}; */
/* } */
