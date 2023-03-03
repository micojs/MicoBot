#pragma once

#include "ESPboyInit.h"
#include "ESPboyLED.h"
#include "nbSPI.h"

#include <cstdio>
#include <cstring>
#include <cstdint>

#define DEBUG {Serial.printf("%s:%d\n", __PRETTY_FUNCTION__, __LINE__);}
inline constexpr uint16_t colorFromRGB(uint8_t R, uint8_t G, uint8_t B){
    uint16_t s = (int(R>>3) << 11) + (int(G>>2) << 5) + int(B>>3);
    return (s >> 8) | (s << 8);
}

#include "js.hpp"
#include "assets.hpp"

ESPboyInit myESPboy;
ESPboyLED myLED;

extern js::Local D; // var
extern js::Local C; // var
extern js::Local B; // var
extern js::Local A; // var
extern js::Local RIGHT; // var
extern js::Local LEFT; // var
extern js::Local DOWN; // var
extern js::Local UP; // var
extern js::Local FRAMETIME; // var


uint32_t pen;
const uint8_t* fontData = res::fontMini;
__attribute__((__aligned__(4))) uint8_t framebuffer[TFT_WIDTH * TFT_HEIGHT];
uint8_t* texture;
// uint16_t palette[256]; // general.hpp
bool mirrored = false;
bool flipped = false;
bool transparent = true;

void JSinit();
void JSupdate(uint32_t, uint32_t);

int state = 0;

template<typename Type, bool swap>
class PGM {
public:
    Type* ptr;

    template<typename Ptr>
    constexpr PGM(Ptr* ptr) : ptr{(Type*)ptr} {}

    Type operator [] (size_t offset) {
        if constexpr (sizeof(Type) == 1) {
            uint32_t res;
            pgm_read_with_offset(ptr + offset, res);
            return res;
        }
        if constexpr (sizeof(Type) == 2) {
            if (!swap) {
                uint32_t res;
                pgm_read_with_offset(ptr + offset, res);
                return res;
            } else {
                uint32_t res;
                pgm_read_with_offset(ptr + offset, res);
                auto v = uint16_t(res); // pgm_read_word(ptr + offset);
                return (v >> 8) | (v << 8);
            }
        }
        if constexpr (sizeof(Type) == 4) {
            return pgm_read_dword(ptr + offset);
        }
        static_assert(sizeof(Type) < 5 && sizeof(Type) != 3);
    }

    PGM& operator ++ () {
        ptr++;
        return *this;
    }

    PGM operator ++ (int) {
        auto ret = *this;
        ptr++;
        return ret;
    }

    PGM& operator += (size_t off) {
        ptr += off;
        return *this;
    }

    PGM& operator -= (size_t off) {
        ptr -= off;
        return *this;
    }

    Type operator* () {
        return (*this)[0];
    }

    operator Type* () {
        return ptr;
    }
};

using PGMU8 = PGM<uint8_t, false>;
using PGMU8X = PGM<uint8_t, true>;
using PGMU16 = PGM<uint16_t, false>;
using PGMU16X = PGM<uint16_t, true>;
using PGMU32 = PGM<uint32_t, false>;
using PGMU32X = PGM<uint32_t, true>;
using PGMS8 = PGM<int8_t, false>;
using PGMS8X = PGM<int8_t, true>;
using PGMS16 = PGM<int16_t, false>;
using PGMS16X = PGM<int16_t, true>;
using PGMS32 = PGM<int32_t, false>;
using PGMS32X = PGM<int32_t, true>;

uint32_t startTime;
uint32_t updateCount = 0;
uint32_t updateFrequency = 1000 / 30;

void setup(){
    PI = 3.1415926535897932384626433f;
    HALF_PI = 3.1415926535897932384626433f / 2;
    TWO_PI = 3.1415926535897932384626433f * 2;
    myESPboy.begin("");
    myLED.begin();
    JSinit();
    startTime = millis();
}

void loop(){
    auto now = millis() - startTime;

    auto targetUpdateCount = updateFrequency ? now / updateFrequency : updateCount + 1;
    auto updateDelta = int32_t(targetUpdateCount - updateCount);
    if (updateDelta < 1)
        return;
    updateCount += updateDelta;

    auto keypressed = myESPboy.getKeys();
    LEFT = !!(keypressed&PAD_LEFT);
    RIGHT = !!(keypressed&PAD_RIGHT);
    UP = !!(keypressed&PAD_UP);
    DOWN = !!(keypressed&PAD_DOWN);
    A = !!(keypressed&PAD_ACT);
    B = !!(keypressed&PAD_ESC);
    C = !!(keypressed&PAD_RGT);
    D = !!(keypressed&PAD_LFT);

    auto startUpdate = millis();
    JSupdate(now, updateDelta);

    {
        uint16_t pal[ 256 ] ;

        /* * /
        PGMU16 palr{palette};
        for (int i = 0; i < 256; ++i) {
            pal[i] = palr[i];
        }

        myESPboy.tft.startWrite();
        uint16_t scanline[TFT_WIDTH];
        for (int y = 0; y < TFT_HEIGHT; ++y) {
            for (int x = 0; x < TFT_WIDTH; ++x) {
                scanline[x] = pal[framebuffer[y * TFT_WIDTH + x]];
            }
            myESPboy.tft.pushPixels(scanline, TFT_WIDTH);
        }
        myESPboy.tft.endWrite();

        /*/

        memcpy(pal, palette, sizeof(pal));
        uint16_t scanline[2][TFT_WIDTH];
        auto f = framebuffer;
        for (uint32_t i = 0; i < TFT_HEIGHT; i++) {
            auto s = scanline[i & 1];
            for (uint32_t j = 0; j < TFT_WIDTH;) {
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
                s[j] = pal[ f[j] ]; j++;
            }
            while (nbSPI_isBusy());
            nbSPI_writeBuffer((uint32_t*)s, 4);
            f += TFT_WIDTH;
        }

        /* */
    }
    FRAMETIME = uint32_t{millis() - startUpdate};
}


js::Local setScreenMode(js::Local& args, bool) {
    return {};
}

inline js::Local getTime(js::Local& args, bool) {
    return {millis()};
}

inline uint32_t getPenFromArgs(js::Local& args) {
    auto argc = js::to<uint32_t>(js::get(args, V_length));
    if (argc == 1) {
        return js::to<uint32_t>(js::get(args, V_0));
    }

    int R = (js::to<uint32_t>(js::get(args, V_0)) >> 3) & 0x1F;
    int G = (js::to<uint32_t>(js::get(args, V_1)) >> 2) & 0x3F;
    int B = (js::to<uint32_t>(js::get(args, V_2)) >> 3) & 0x1F;

    if (argc >= 4) {
        uint8_t alpha = js::to<uint32_t>(js::get(args, V_3));
        if (alpha < 128)
          return 0;
    }

    PGMU16X pal{palette};

    uint32_t closest = 0;
    uint32_t closestDistance = -1;
    for (int i = 1; i < 256; ++i) {
        uint32_t packed = pal[i];

        int tr = (packed >> 11 & 0x1F);
        int dr = R - tr;

        int tg = (packed >> 5) & 0x3F;
        int dg = G - tg;

        int tb = packed & 0x1F;
        int db = B - tb;

        int d = dr*dr + dg*dg + db*db;
        if (d < closestDistance) {
            closest = i;
            closestDistance = d;
            if (!closestDistance)
                break;
        }
    }

    return closest;
}

js::Local setPen(js::Local& args, bool) {
    pen = getPenFromArgs(args);
    return {pen};
}

js::Local setLED(js::Local& args, bool) {
    auto argc = js::to<uint32_t>(js::get(args, V_length));
    auto r = js::to<uint32_t>(js::get(args, V_0));
    auto g = js::to<uint32_t>(js::get(args, V_1));
    auto b= js::to<uint32_t>(js::get(args, V_2));
    myLED.setRGB(r, g, b);
    return {};
}

js::Local clear(js::Local&, bool) {
    memset(framebuffer, pen, sizeof(framebuffer));
    return {};
}

inline js::Local setTileMap(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        // vsgl::map = (const uint8_t*) *ref;
        // vsgl::tse = (const TileSetEntry*) (vsgl::map[2] | (vsgl::map[3] << 8) | (vsgl::map[4] << 16) | (vsgl::map[5] << 24));
    } else {
        // vsgl::map = nullptr;
    }
    return {};
}

js::Local setFont(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        fontData = (uint8_t*) *ref;
    }
    return {};
}

template<bool mirror, bool transparent, bool recolor>
IRAM_ATTR void P_P(const uint8_t* src, uint8_t* dest, uint32_t cnt) {
    // if (!mirror && !transparent && !recolor && cnt > 4) {
    //     auto misalign = 4 - (reinterpret_cast<intptr_t>(src) & 3);
    //     memcpy_P(dest + misalign, src + misalign, cnt - misalign);
    //     cnt = misalign;
    // }
    while (cnt--) {
        // auto index = pgm_read_byte(src);
        uint32_t index;
        pgm_read_with_offset(src, index);
        if (!transparent || (index << 24)) {
            if (recolor)
                index += pen;
            *dest = index;
        }
        dest++;
        if (mirror) src--;
        else src++;
    }
}

js::Local setTexture(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto ref = std::get_if<js::ResourceRef*>(&arg0)) {
        texture = ((uint8_t*) *ref);
    } else {
        texture = nullptr;
    }
    return {};
}

js::Local setFPS(js::Local& args, bool) {
    auto targetFPS = js::to<uint32_t>(js::get(args, V_0));
    updateFrequency = targetFPS ? 1000 / targetFPS : 0;
    return {};
}

void blitInternal(int x, int y) {
    if (!texture)
        return;
    auto data = PGMU8(texture);
    uint32_t width = data[0];
    uint32_t height = data[1];
    x -= width / 2;
    y -= height / 2;
    const uint32_t stride = width;

    if (x >= int(TFT_WIDTH))
        return;
    if (x <= -int(width))
        return;
    if (y >= int(TFT_HEIGHT))
        return;
    if (y <= -int(height))
        return;

    auto source = data;
    source += 4;

    if (flipped) {
        if (y < 0) {
            height += y;
            y = 0;
        }
        if (int32_t(y + height) > int32_t(TFT_HEIGHT)) {
            source += (int32_t(y + height) - int32_t(TFT_HEIGHT)) * stride;
            height = TFT_HEIGHT - y;
        }
    } else {
        if (y < 0) {
            height += y;
            source -= y * int32_t(width);
            y = 0;
        }
        if (int32_t(y + height) > int32_t(TFT_HEIGHT)) {
            height = TFT_HEIGHT - y;
        }
    }

    if (mirrored) {
        source += stride - 1;
        if (x < 0) {
            width += x;
            source += x;
            x = 0;
        }
        if (int32_t(x + width) > int32_t(TFT_WIDTH)) {
            width = TFT_WIDTH - x;
        }
    } else {
        if (x < 0) {
            width += x;
            source += -x;
            x = 0;
        }
        if (int32_t(x + width) > int32_t(TFT_WIDTH)) {
            width = TFT_WIDTH - x;
        }
    }

    auto dest = framebuffer + y * TFT_WIDTH + x;
    auto blit = P_P<false, false, false>;

    if (pen) {
        if (mirrored) {
            if (transparent) {
                blit = P_P<true, true, true>;
            } else {
                blit = P_P<true, false, true>;
            }
        } else {
            if (transparent) {
                blit = P_P<false, true, true>;
            } else {
                blit = P_P<false, false, true>;
            }
        }
    } else {
        if (mirrored) {
            if (transparent) {
                blit = P_P<true, true, false>;
            } else {
                blit = P_P<true, false, false>;
            }
        } else {
            if (transparent) {
                blit = P_P<false, true, false>;
            } else {
                blit = P_P<false, false, false>;
            }
        }
    }

    if (flipped) {
        source += stride * (height - 1);
        auto maxY = y + height;
        for (; uint32_t(y) < maxY; ++y, source -= stride, dest += TFT_WIDTH) {
            blit(source, dest, width);
        }
    } else {
        auto maxY = y + height;
        for (; uint32_t(y) < maxY; ++y, source += stride, dest += TFT_WIDTH) {
            blit(source, dest, width);
        }
    }
}

void rotozoom(int32_t x, int32_t y, float angle, float scale) {
    if (!texture || scale == 0.0f)
        return;
    auto data = PGMU8(texture);
    uint32_t width = data[0];
    uint32_t height = data[1];
    int32_t stride = width;
    auto source = data;
    source += 4;

    float anchorX = 0.5;
    float anchorY = 0.5;

    int iscale = (1<<16) / scale;
    float sa = sin(angle);
    float ca = cos(angle);
    int cx = ca * iscale;
    int cy = sa * iscale;
    int lx = -cy;
    int ly = cx;
    float W = width * scale;
    float H = height * scale;
    int ax = 0;
    int ay = 0;

    float t;
    float corner0X = - W * anchorX;
    float corner0Y = - H * anchorY;
    t = ca * corner0X + sa * corner0Y;
    corner0Y = -sa * corner0X + ca * corner0Y;
    corner0X = t;

    float corner1X = W * (1 - anchorX);
    float corner1Y = - H * anchorY;
    t = ca * corner1X + sa * corner1Y;
    corner1Y = -sa * corner1X + ca * corner1Y;
    corner1X = t;

    float corner2X = - W * anchorX;
    float corner2Y = H * (1 - anchorY);
    t = ca * corner2X + sa * corner2Y;
    corner2Y = -sa * corner2X + ca * corner2Y;
    corner2X = t;

    float corner3X = W * (1 - anchorX);
    float corner3Y = H * (1 - anchorY);
    t = ca * corner3X + sa * corner3Y;
    corner3Y = -sa * corner3X + ca * corner3Y;
    corner3X = t;

    int minX = corner0X;
    int maxX = corner0X;
    int minY = corner0Y;
    int maxY = corner0Y;

    if (int(corner1X) < minX) minX = corner1X;
    if (int(corner1X) > maxX) maxX = corner1X;
    if (int(corner1Y) < minY) minY = corner1Y;
    if (int(corner1Y) > maxY) maxY = corner1Y;
    if (int(corner2X) < minX) minX = corner2X;
    if (int(corner2X) > maxX) maxX = corner2X;
    if (int(corner2Y) < minY) minY = corner2Y;
    if (int(corner2Y) > maxY) maxY = corner2Y;
    if (int(corner3X) < minX) minX = corner3X;
    if (int(corner3X) > maxX) maxX = corner3X;
    if (int(corner3Y) < minY) minY = corner3Y;
    if (int(corner3Y) > maxY) maxY = corner3Y;

    if (int32_t dy = minY - corner0Y; dy < 0){
        ay += ly * dy;
        ax += lx * dy;
    }

    if (int32_t dx = minX - corner0X; dx < 0){
        ay += cy * dx;
        ax += cx * dx;
    }

    minX += x;
    minY += y;
    maxX += x;
    maxY += y;

    if (minY < 0){
        ay -= ly * minY;
        ax -= lx * minY;
        minY = 0;
    }
    if (maxY > TFT_HEIGHT) {
        maxY = TFT_HEIGHT;
    }
    if (maxX > s32(TFT_WIDTH)) {
        maxX = TFT_WIDTH;
    }
    if (maxY <= 0 || minY >= s32(TFT_HEIGHT)) {
        return;
    }

    int sx = 0x8000;
    int sy = 0x8000;
    if (minX < 0) {
        sx += cx * -minX;
        sy += cy * -minX;
        minX = 0;
    }

    auto fb = framebuffer + minY * TFT_WIDTH;

    int sign = 1;
    if (mirrored) {
        source += width - 1;
        sign = -1;
    }

    if (flipped) {
        source += stride * (height - 1);
        stride = -stride;
    }

    for (int y = minY; y < maxY; ++y, fb += TFT_WIDTH) {
        int px = ax + sx;
        int py = ay + sy;
        ax += lx; ay += ly;

        for (int x = minX; x < maxX; ++x, px += cx, py += cy) {
            uint32_t tx = (px) >> 16;
            uint32_t ty = (py) >> 16;
            if (tx >= width || ty >= height)
                continue;
            uint16_t pixel = source[ty * stride + tx * sign];
            if (pixel)
                fb[x] = pixel + pen;
        }
    }
}

js::Local setMirrored(js::Local& args, bool) {
    mirrored = js::to<bool>(js::get(args, V_0));
    return {};
}

js::Local setFlipped(js::Local& args, bool) {
    flipped = js::to<bool>(js::get(args, V_0));
    return {};
}

js::Local setTransparent(js::Local& args, bool) {
    transparent = js::to<bool>(js::get(args, V_0));
    return {};
}

js::Local readByte(js::Local& args, bool) {
    auto arg = get(args, V_0);
    auto offset = js::to<int32_t>(js::get(args, V_1));
    if (auto res = std::get_if<js::ResourceRef*>(&arg); res && *res) {
        return {uint32_t(PGMU8{*res}[offset])};
    }
    return {};
}

js::Local getWidth(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(PGMU8{*res}[0])};
    }
    return {TFT_WIDTH};
}

js::Local getHeight(js::Local& args, bool) {
    auto arg = get(args, V_0);
    if (auto res = std::get_if<js::ResourceRef*>(&arg)) {
        if (!*res)
            return {};
        return {uint32_t(PGMU8{*res}[1])};
    }
    return {TFT_HEIGHT};
}

inline js::Local rect(js::Local& args, bool) {
    int x = js::to<int32_t>(js::get(args, V_0));
    int y = js::to<int32_t>(js::get(args, V_1));
    int w = js::to<int32_t>(js::get(args, V_2));
    int h = js::to<int32_t>(js::get(args, V_3));

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w >= TFT_WIDTH) {
        w = TFT_WIDTH - x;
    }
    if (y + h >= TFT_HEIGHT) {
        h = TFT_HEIGHT - y;
    }
    if (w <= 0 || h <= 0) {
        return {};
    }

    h += y;
    auto dest = framebuffer + x + y * TFT_WIDTH;
    for (; y < h; ++y, dest += TFT_WIDTH) {
        memset(dest, pen, w);
    }

    return {};
}

js::Local image(js::Local& args, bool) {
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
        setTexture(args, false);
        blitInternal(js::to<int32_t>(js::get(args, V_1)),
                     js::to<int32_t>(js::get(args, V_2)));
        break;

    case 4:
        setTexture(args, false);
        rotozoom(js::to<int32_t>(js::get(args, V_1)),
                 js::to<int32_t>(js::get(args, V_2)),
                 js::to<float>(js::get(args, V_3)), 1.0f);
        break;

    case 5:
        setTexture(args, false);
        rotozoom(js::to<int32_t>(js::get(args, V_1)),
                 js::to<int32_t>(js::get(args, V_2)),
                 js::to<float>(js::get(args, V_3)),
                 js::to<float>(js::get(args, V_4)));
        break;

    default:
        break;
    }
    return {};
}

void pixel(int32_t x, int32_t y) {
    if (uint32_t(x) < TFT_WIDTH && uint32_t(y) < TFT_HEIGHT) {
        framebuffer[y * TFT_WIDTH + x] = pen;
    }
}

int32_t drawChar(int32_t x, int32_t y, char ch){
    PGMU8 font{fontData};
    uint32_t fontW = font[0];
    uint32_t fontH = font[1];
    uint32_t hbytes = (fontH + 7) >> 3;
    if (font[3] && ch >= 'a' && ch <= 'z') {
        ch = (ch - 'a') + 'A';
    }
    uint32_t index = ch - font[2];

    auto bitmap = font;
    bitmap += 4 + index * (1 + fontW * hbytes); //add an offset to the pointer
    uint32_t charW = *bitmap; //first byte of char is char width
    ++bitmap;

    for (uint32_t i = 0; i < charW; ++i) {
        for(uint32_t byteNum = 0; byteNum < hbytes; ++byteNum) {
            uint8_t bitcolumn = *bitmap;
            ++bitmap;
            int32_t endRow = (8 + 8*byteNum < fontH) ? (8 + 8*byteNum) : fontH;
            for (int32_t j = 8*byteNum; j < endRow; ++j) { // was j<=h
                if (bitcolumn&1) {
                    pixel(x + i, y + j);
                }
                bitcolumn>>=1;
            }
        }
    }

    return charW + 1;
}

js::Local text(js::Local& args, bool) {
    auto str = js::toString(js::get(args, V_0));
    auto x = js::to<int32_t>(js::get(args, V_1));
    auto y = js::to<int32_t>(js::get(args, V_2));
    // blit::screen.text((char*)str.data(), *font, blit::Point(x, y));
    auto ptr = str.data();
    while (char c = *ptr++) {
        x += drawChar(x, y, c);
    }
    return {};
}
