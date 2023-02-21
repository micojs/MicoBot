#pragma once

#include <cstdint>
#include <cmath>

// #define FIXED_POINTS_USE_NAMESPACE
// #define FIXED_POINTS_NO_RANDOM
// #include "../FixedPoints/FixedPoints.h"
// #define FLOAT FixedPoints::SFixed<23, 8>

#include "vsgl.hpp"
#include "Pokitto.h"
#include "assets.hpp"
#include <LibLog>
#include "js.hpp"

using vsgl = vsgl_t<220, 176>;
using PC = Pokitto::Core;
using PD = Pokitto::Display;

void JSinit();
void JSupdate();

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

#if ENABLE_PROFILER != 0
inline const char* volatile profilerSample = "NONE2";
#endif

void init() {
    PI = 3.1415926535897932384626433f;
    HALF_PI = 3.1415926535897932384626433f / 2;
    TWO_PI = 3.1415926535897932384626433f * 2;
    FRAMETIME = 1;
    vsgl::font = resource::fontMini;
    PD::persistence = 1;
    PD::load565Palette(palette);
    JSinit();
    startTime = PC::getTime();
}

void update() {
}

int main(){
    PC::begin();
    init();
    while(true){
        auto now = PC::getTime() - startTime;

        auto targetUpdateCount = updateFrequency ? now / updateFrequency : updateCount + 1;
        auto updateDelta = int32_t(targetUpdateCount - updateCount);
        if (updateDelta < 1) {
            PROFILER_NAMED("updateHook");
            PC::updateHook(false);
            continue;
        }
        updateCount += updateDelta;

        PC::updateHook(true);
        PC::buttons.update();

        C = !!PC::cBtn();
        B = !!PC::bBtn();
        A = !!PC::aBtn();
        RIGHT = !!PC::rightBtn();
        LEFT = !!PC::leftBtn();
        DOWN = !!PC::downBtn();
        UP = !!PC::upBtn();

        auto startUpdate = PC::getTime();
        JSupdate();

#if ENABLE_PROFILER != 0
        {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d %d %s", int(js::heapSize), int(1000 / js::to<int32_t>(FRAMETIME)), profilerSample);

            js::recycleCount = 0;
            js::gcCount = 0;
            js::freeCount = 0;

            auto oldPen = vsgl::pen;
            vsgl::pen = 0;
            vsgl::text(tmp, 0, 176 - 8);
            vsgl::pen = oldPen;
        }
#endif

        {
            PROFILER_NAMED("flush")
            vsgl::draw([Y=0](const uint8_t* framebuffer) mutable {
                for (int y = 0; y < 8; ++y) {
                    flushLine(palette, framebuffer + y * 220);
                }
            });
        }

        FRAMETIME = uint32_t{PC::getTime() - startUpdate};
    }
}

inline js::Local getTime(js::Local& args, bool) {
    return {PC::getTime()};
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
    for (int i = 1; i < PALETTE_SIZE; ++i) {
        uint32_t packed = PD::palette[i];
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

inline uint32_t pen;

inline js::Local setPen(js::Local& args, bool) {
    vsgl::pen = pen = getPenFromArgs(args);
    PD::color = pen;
    return {pen};
}

inline js::Local setLED(js::Local& args, bool) {
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
    PC::setFrameRate(js::to<uint32_t>(js::get(args, V_0)));
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
    if (auto str = std::get_if<js::BufferRef>(&arg)) {
        if (!str->data())
            return {0};

    }
    return {LCDWIDTH};
}

inline js::Local getHeight(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(((const uint8_t*)*res)[1])};
    }
    return {LCDHEIGHT};
}

/* */

inline js::Local setFont(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        vsgl::font = (const uint8_t*) *ref;
    }
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
    auto str = js::toString(js::get(args, V_0));
    auto x = js::to<int32_t>(js::get(args, V_1));
    auto y = js::to<int32_t>(js::get(args, V_2));
    vsgl::text((char*)str.data(), x, y);
    return {};
}

inline js::Local clear(js::Local&, bool) {
    vsgl::clear();
    return {};
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

/*/

inline void blitInternal(int x, int y) {
    if (!texture)
        return;
    x -= texture[0] / 2;
    y -= texture[1] / 2;
#if PROJ_SCREENMODE != TASMODE
    if (flipped)
        PD::drawBitmapYFlipped(x, y, texture);
    else if (mirrored)
        PD::drawBitmapXFlipped(x, y, texture);
    else
        PD::drawBitmap(x, y, texture);
#else
    PD::drawSprite(x, y, texture, flipped, mirrored, pen);
#endif
}

inline js::Local image(js::Local& args, bool) {
    auto argc = js::to<uint32_t>(get(args, V_length));
    switch (argc) {
    case 0:
        blitInternal(0, 0);
        break;

    case 1:
        setTexture(args, false);
        blitInternal(0, 0);
        break;

    case 2:
        blitInternal(js::to<int32_t>(js::get(args, V_0)),
                     js::to<int32_t>(js::get(args, V_1)));
        break;

    case 3:
    default:
        setTexture(args, false);
        blitInternal(js::to<int32_t>(js::get(args, V_1)),
                     js::to<int32_t>(js::get(args, V_2)));
        break;
    }
    return {};
}

inline js::Local text(js::Local& args, bool) {
    auto str = js::toString(js::get(args, V_0));
    auto x = js::to<int32_t>(js::get(args, V_1));
    auto y = js::to<int32_t>(js::get(args, V_2));
    PD::print(x, y, (char*)str.data());
    return {};
}

inline js::Local clear(js::Local&, bool) {
#if PROJ_SCREENMODE != TASMODE
    uint8_t c = pen & (PALETTE_SIZE-1); //don't let palette go out of bounds
    if (PROJ_COLORDEPTH==1 && pen) {
        c = 0xFF; // pen !=0, set all pixels
    } else if (PROJ_COLORDEPTH==2) {
        c = pen & 0x3;
        c = c | (c << 2);
        c = c | (c << 4);
    } else if (PROJ_COLORDEPTH==4) {
        c |= c << 4;
    }
    memset((void*)PD::screenbuffer, c, POK_SCREENBUFFERSIZE);
#endif
    return {};
}

inline js::Local setFont(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        PD::font = (const uint8_t*) *ref;
    }
    return {};
}
/* */
