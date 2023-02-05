#pragma once

#include <cstring>
#include <cstdint>
#include <string>
#include <memory>

#ifdef A
#undef A
#endif
#ifdef B
#undef B
#endif

#if defined(TARGET_CORTEX) || defined(__SAMD21G18A__)
#define memcpy memops::copy
#define memset memops::set

namespace memops {

    static void __attribute__((naked)) copy(void *to, const void *from, std::size_t length) {
    __asm__ volatile (
        ".syntax unified" "\n"

        "1: cmp r2, 8" "\n"
        "blt 1f" "\n"
        "ldrb r3, [r1, 0]" "\n"
        "strb r3, [r0, 0]" "\n"
        "ldrb r3, [r1, 1]" "\n"
        "strb r3, [r0, 1]" "\n"
        "ldrb r3, [r1, 2]" "\n"
        "strb r3, [r0, 2]" "\n"
        "ldrb r3, [r1, 3]" "\n"
        "strb r3, [r0, 3]" "\n"
        "ldrb r3, [r1, 4]" "\n"
        "strb r3, [r0, 4]" "\n"
        "ldrb r3, [r1, 5]" "\n"
        "strb r3, [r0, 5]" "\n"
        "ldrb r3, [r1, 6]" "\n"
        "strb r3, [r0, 6]" "\n"
        "ldrb r3, [r1, 7]" "\n"
        "strb r3, [r0, 7]" "\n"
        "adds r0, 8" "\n"
        "adds r1, 8" "\n"
        "subs r2, 8" "\n"
        "bne 1b" "\n"

        "1: cmp r2, 0" "\n"
        "beq 3f" "\n"

        "adds r0, r2" "\n"
        "adds r1, r2" "\n"

        // not aligned, do byte copy
        "subs r0, 1" "\n"
        "rsbs r2, 0" "\n"
        "1: ldrb r3, [r1, r2]" "\n"
        "adds r2, 1" "\n"
        "strb r3, [r0, r2]" "\n"
        "bne 1b" "\n"
        // "bx lr" "\n"

        // aligned, do word copy
        // "2: rsbs r2, 0" "\n"
        // "1: ldr r3, [r1, r2]" "\n"
        // "str r3, [r0, r2]" "\n"
        // "adds r2, 4" "\n"
        // "bne 1b" "\n"
        "3: bx lr" "\n"
        );
    }

    static void __attribute__((naked)) set(void* dst, std::uint8_t val, std::size_t count) {
        __asm__ volatile (
            ".syntax unified" "\n"
            "1: cmp r2, 8" "\n"
            "blt 1f" "\n"
            "strb r1, [r0, 0]" "\n"
            "strb r1, [r0, 1]" "\n"
            "strb r1, [r0, 2]" "\n"
            "strb r1, [r0, 3]" "\n"
            "strb r1, [r0, 4]" "\n"
            "strb r1, [r0, 5]" "\n"
            "strb r1, [r0, 6]" "\n"
            "strb r1, [r0, 7]" "\n"
            "adds r0, 8" "\n"
            "subs r2, 8" "\n"
            "bne 1b" "\n"

            "1:rsbs r2, 0" "\n"
            "beq 2f" "\n"
            "subs r0, r2" "\n"
            "1: strb r1, [r0, r2]" "\n"
            "adds r2, 1" "\n"
            "bne 1b" "\n"
            "2: bx lr" "\n"
            );
    }

    static void __attribute__((naked)) set(void* dst, std::uint8_t val, std::size_t count, std::size_t stride) {
        __asm__ volatile (
            ".syntax unified"   "\n"
            "muls r2, r3"       "\n"
            "adds r0, r2"       "\n"
            "subs r0, r3"       "\n"
            "rsbs r2, 0"        "\n"
            "beq 2f"            "\n"
            "1: adds r2, r3"    "\n"
            "strb r1, [r0, r2]" "\n"
            "ble 1b"            "\n"
            "2: bx lr"          "\n"
        );
    }

}

#endif

namespace ns_internal {
    template<uint32_t size = 64> struct _umin { using type = uint64_t; };
    template<> struct _umin<4> { using type = uint32_t; };
    template<> struct _umin<2> { using type = uint16_t; };
    template<> struct _umin<1> { using type = uint8_t; };

    template<uint64_t maxVal>
    using umin = typename _umin<(maxVal >> 32) ? 8 :
                                (maxVal >> 16) ? 4 :
                                (maxVal >> 8) ? 2 :
                                1>::type;

    template<uint32_t size = 64> struct _smin { using type = int64_t; };
    template<> struct _smin<4> { using type = int32_t; };
    template<> struct _smin<2> { using type = int16_t; };
    template<> struct _smin<1> { using type = int8_t; };

    template<uint64_t maxVal>
    using smin = typename _smin<(maxVal >> 31) ? 8 :
                                (maxVal >> 15) ? 4 :
                                (maxVal >> 7) ? 2 :
                                1>::type;
}

template<int screenWidth, int screenHeight>
class vsgl_t {
    static inline uint32_t usedCount = 0;
    static inline uint32_t reservedCount = 0;

public:

    using small = ns_internal::smin<screenWidth | screenHeight>;
    using usmall = ns_internal::umin<screenWidth | screenHeight>;

    class Command {
    public:
        std::unique_ptr<Command> next;
        usmall minY = 0, maxY = screenHeight, A = 0, B = 0;
        Command() = default;
        virtual ~Command() = default;
        virtual void draw(uint8_t* framebuffer, int y) = 0;
    };

    class TextCommand : public Command {
    public:
        std::string text;
        const uint8_t* font = vsgl_t::font;
        int x;

        TextCommand(const char* text, int X, int Y) : text{text}, x{X} {
            auto h = font[1];
            this->A = ((h>>3) + ((h != 8) && (h != 16)));
            this->B = pen;
            this->minY = Y;
            this->maxY = Y + h;
        }

        void draw(uint8_t* framebuffer, int Yoffset) override {
            auto color = this->B;
            auto minY = int(this->minY) - Yoffset;
            auto maxY = std::min<int>(this->maxY - Yoffset, 8);
            auto dest = framebuffer;
            int x = this->x;

            int w = font[0];
            // int h = font[1];
            int hbytes = this->A;
            int skip = 0;
            if (minY >= 0) {
                dest += minY * screenWidth;
                maxY -= minY;
                minY = 0;
            } else {
                skip = -minY;
                minY = 0;
            }

            for (int index : text) {
                if (font[3] && index >= 'a' && index <= 'z') {
                    index = (index - 'a') + 'A';
                }
                index -= font[2];
                if (index < 0)
                    continue;

                auto bitmap = font + 4 + index * (w * hbytes + 1);
                int numBytes = *bitmap++;
                if (uint32_t(numBytes + x) > uint32_t(screenWidth))
                    numBytes = screenWidth - x;
                for (int i = 0; i < numBytes; i++) {
                    if (x + i < 0)
                        continue;

                    uint32_t bitcolumn = *bitmap++;
                    if (hbytes == 2)
                        bitcolumn |= (*bitmap++)<<8;

                    auto line = dest + x + i;
                    for (int y = minY; y < maxY; ++y, line += screenWidth) {
                        if (bitcolumn & (1<<(y + skip)))
                            *line = color;
                    }
                }
                x += numBytes + 1;
            }
        }
    };

    template <bool mirror, bool transparent, bool recolor>
    class RotoZoomCommand : public Command {
    public:
        const uint8_t* source;
        int ax, ay, sx, sy/*, lx, ly*/, cx, cy;
        uint16_t stride, height;
        int16_t minX, maxX;

        RotoZoomCommand(int minY, int maxY, const uint8_t* source, int stride,
                        int width, int height,
                        int ax, int ay,
                        int sx, int sy,
                        // int lx, int ly,
                        int cx, int cy,
                        int minX, int maxX) :
            source{source},
            ax{ax}, ay{ay},
            sx{sx}, sy{sy},
            // lx{lx}, ly{ly},
            cx{cx}, cy{cy},
            stride(stride)
            {
                this->minY = minY;
                this->maxY = maxY;
                this->minX = minX;
                this->maxX = maxX;
                this->height = height;
                this->A = width;
                this->B = pen;
            }

        void draw(uint8_t* framebuffer, int Yoffset) {
            auto minY = int(this->minY) - Yoffset;
            auto source = this->source;
            auto dest = framebuffer;
            if (minY >= 0) {
                dest += minY * screenWidth;
            } else {
                minY = 0;
            }
            uint32_t width = abs(int(stride)) << 16;
            uint32_t height = int(this->height) << 16;

            auto maxY = std::min<int>(this->maxY - Yoffset, 8);

            auto cx = this->cx;
            auto cy = this->cy;
            auto sx = this->sx;
            auto sy = this->sy;
            auto ax = this->ax;
            auto ay = this->ay;
            dest += minX;
            int B = this->B;
            int maxX = this->maxX - minX;

            for (int y = minY; y < maxY; ++y, dest += screenWidth) {
                int px = ax + sx;
                int py = ay + sy;
                ax -= cy; ay += cx;

                for (int x = 0; x < maxX; ++x/*, px += cx, py += cy*/) {
                    uint32_t cpx = px + cx * x;
                    if (cpx >= width)
                        continue;

                    uint32_t cpy = py + cy * x;
                    if (cpy >= height)
                        continue;

                    uint32_t tx = cpx >> 16;
                    uint32_t ty = cpy >> 16;

                    if (mirror)
                        tx *= -1;
                    int pixel = source[ty * stride + tx];
                    if (!transparent || pixel) {
                        if (recolor)
                            pixel += B;
                        dest[x] = pixel;
                    }
                }
            }

            this->ax = ax;
            this->ay = ay;
        }
    };

    template<bool mirror, bool transparent, bool recolor>
    class SpriteCommand : public Command {
    public:
        const uint8_t* source;
        int16_t stride, x;
        // int16_t width, add;

        SpriteCommand(int x, int y, int height, const uint8_t* source, int stride, int width) :
            source{source},
            stride(stride),
            x(x)
            {
                this->minY = y;
                this->maxY = y + height;
                this->A = width;
                this->B = pen;
            }

        void draw(uint8_t* framebuffer, int Yoffset) override {
            auto y = int(this->minY) - Yoffset;
            auto source = this->source;
            auto dest = framebuffer + x;
            if (y >= 0) {
                dest += y * screenWidth;
            } else {
                source += -y * stride;
                y = 0;
            }

            auto maxY = std::min<int>(int(this->maxY) - Yoffset, 8);
            for (; y < maxY; ++y, source += stride, dest += screenWidth) {
                P_P(source, dest, this->A, this->B);
            }
        }

        static void P_P(const uint8_t* src, uint8_t* dest, uint32_t count, int add) {
            if (!mirror && !transparent && !recolor) {
                memcpy(dest, src, count);
            } else while (count--) {
                uint32_t index = *src;
                if (!transparent || index) {
                    if (recolor)
                        index += add;
                    *dest = index;
                }
                dest++;
                if (mirror) src--;
                else src++;
            }
        }

    };

    class RectCommand : public Command {
    public:
        int x;

        RectCommand(int x, int y, int width, int height) :
            x(x)
            {
                this->minY = y;
                this->maxY = y + height;
                this->A = width;
                this->B = pen;
            }

        void draw(uint8_t* framebuffer, int Yoffset) override {
            auto y = int(this->minY) - Yoffset;
            auto dest = framebuffer + x;
            if (y >= 0) {
                dest += y * screenWidth;
            } else {
                y = 0;
            }

            auto maxY = std::min<int>(this->maxY - Yoffset, 8);
            auto A = this->A;
            auto B = this->B;
            for (; y < maxY; ++y, dest += screenWidth) {
                memset(dest, B, A);
            }
        }
    };

    static inline uint32_t pen;
    static inline uint32_t clearColor;
    static inline const uint8_t* font = nullptr;
    static inline std::unique_ptr<Command> first;
    static inline Command* last = nullptr;

    static void clear() {
        clearColor = pen;
        for (std::unique_ptr<Command> cmd = std::move(first), next; cmd; cmd = std::move(next)) {
            next = std::move(cmd->next);
        }
        last = nullptr;
    }

    static void push(std::unique_ptr<Command> cmd) {
        auto copy = cmd.get();
        if (last) last->next = std::move(cmd);
        else first = std::move(cmd);
        last = copy;
    }

    static void draw(uint8_t* framebuffer) {
        auto cursor = framebuffer;
        for (int y = 0; y < screenHeight; y += 8, cursor += screenWidth * 8) {
            if (clearColor)
                memset(cursor, clearColor, screenWidth * 8);
            int lowerBound = y + 8;
            auto prev = &first;
             while (auto cmd = prev->get()) {
                if (lowerBound > cmd->minY) {
                    if (y >= cmd->maxY) {
                        prev = &cmd->next;
                        *prev = std::move(cmd->next);
                        continue;
                    }
                    cmd->draw(cursor, y);
                }
                prev = &cmd->next;
            }
        }
    }

    template<typename Func>
    static void draw(Func&& func) {
        uint8_t framebuffer[screenWidth * 8];
        for (int y = 0; y < screenHeight; y += 8) {
            if (clearColor)
                memset(framebuffer, clearColor, screenWidth * 8);
            int lowerBound = y + 8;
            auto prev = &first;
             while (auto cmd = prev->get()) {
                if (lowerBound > cmd->minY) {
                    if (y >= cmd->maxY) {
                        prev = &cmd->next;
                        *prev = std::move(cmd->next);
                        continue;
                    }
                    cmd->draw(framebuffer, y);
                }
                prev = &cmd->next;
            }
            func(framebuffer);
        }
    }

    static void text(const char* text, int x, int y) {
        push(std::make_unique<TextCommand>(text, x, y));
    }

    static void rect(int x, int y, int w, int h) {
        if (x < 0) {
            w += x;
            x = 0;
        }
        if (y < 0) {
            h += y;
            y = 0;
        }
        if (x + w >= screenWidth) {
            w = screenWidth - x;
        }
        if (y + h >= screenHeight) {
            h = screenHeight - y;
        }
        if (w <= 0 || h <= 0) {
            return;
        }
        push(std::make_unique<RectCommand>(x, y, w, h));
    }

    static void image(const uint8_t* data, int x, int y, bool mirrored, bool flipped, bool transparent) {
        int width = data[0];
        int height = data[1];
        int headerSize = 2;
        if (width == 0) {
            width = data[1]; width <<= 8;
            width |= data[2];
            height = data[3]; height <<= 8;
            height |= data[4];
            headerSize = 5;
        }
        headerSize++; // int bpp = data[headerSize++];
        headerSize++; // int recolor = data[headerSize++];

        x -= width / 2;
        y -= height / 2;
        uint32_t stride = width;

        if (x >= int(screenWidth))
            return;
        if (x <= -width)
            return;
        if (y >= int(screenHeight))
            return;
        if (y <= -height)
            return;

        auto source = data;
        source += headerSize;

        if (flipped) {
            if (y < 0) {
                height += y;
                y = 0;
            }
            if (int32_t(y + height) > int32_t(screenHeight)) {
                source += (int32_t(y + height) - int32_t(screenHeight)) * stride;
                height = screenHeight - y;
            }
        } else {
            if (y < 0) {
                height += y;
                source -= y * width;
                y = 0;
            }
            if (int32_t(y + height) > int32_t(screenHeight)) {
                height = screenHeight - y;
            }
        }

        if (mirrored) {
            source += stride - 1;
            if (x < 0) {
                width += x;
                source += x;
                x = 0;
            }
            if (x + width > int32_t(screenWidth)) {
                width = screenWidth - x;
            }
        } else {
            if (x < 0) {
                width += x;
                source += -x;
                x = 0;
            }
            if (x + width > int32_t(screenWidth)) {
                width = screenWidth - x;
            }
        }

        if (flipped) {
            source += stride * (height - 1);
            stride = -stride;
        }

        if (pen) {
            if (mirrored) {
                if (transparent) {
                    push(std::make_unique<SpriteCommand<true, true, true>>(x, y, height, source, stride, width));
                } else {
                    push(std::make_unique<SpriteCommand<true, false, true>>(x, y, height, source, stride, width));
                }
            } else {
                if (transparent) {
                    push(std::make_unique<SpriteCommand<false, true, true>>(x, y, height, source, stride, width));
                } else {
                    push(std::make_unique<SpriteCommand<false, false, true>>(x, y, height, source, stride, width));
                }
            }
        } else {
            if (mirrored) {
                if (transparent) {
                    push(std::make_unique<SpriteCommand<true, true, false>>(x, y, height, source, stride, width));
                } else {
                    push(std::make_unique<SpriteCommand<true, false, false>>(x, y, height, source, stride, width));
                }
            } else {
                if (transparent) {
                    push(std::make_unique<SpriteCommand<false, true, false>>(x, y, height, source, stride, width));
                } else {
                    push(std::make_unique<SpriteCommand<false, false, false>>(x, y, height, source, stride, width));
                }
            }
        }
    }

    static void rotozoom(const uint8_t* data, int x, int y, float angle, float scale, bool mirrored, bool flipped, bool transparent) {
        if (!data || scale == 0.0f)
            return;

        if (angle == 0 && scale == 1) {
            image(data, x, y, mirrored, flipped, transparent);
            return;
        }

        int width = data[0];
        int height = data[1];
        int headerSize = 2;
        if (width == 0) {
            width = data[1]; width <<= 8;
            width |= data[2];
            height = data[3]; height <<= 8;
            height |= data[4];
            headerSize = 5;
        }
        headerSize++; // int bpp = data[headerSize++];
        headerSize++; // int recolor = data[headerSize++];

        int32_t stride = width;
        auto source = data;
        source += headerSize;

        float anchorX = 0.5;
        float anchorY = 0.5;

        int iscale = (1<<16) / scale;
        float sa = sinf(angle);
        float ca = cosf(angle);
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
        if (maxY > screenHeight) {
            maxY = screenHeight;
        }
        if (maxX > int(screenWidth)) {
            maxX = screenWidth;
        }
        if (maxY <= 0 || minY >= int(screenHeight)) {
            return;
        }

        int sx = 0x8000;
        int sy = 0x8000;
        if (minX < 0) {
            sx += cx * -minX;
            sy += cy * -minX;
            minX = 0;
        }

        if (mirrored) {
            source += width - 1;
        }

        if (flipped) {
            source += stride * (height - 1);
            stride = -stride;
        }

        if (pen) {
            if (mirrored) {
                if (transparent) {
                    push(std::make_unique<RotoZoomCommand<true, true, true>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                } else {
                    push(std::make_unique<RotoZoomCommand<true, false, true>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                }
            } else {
                if (transparent) {
                    push(std::make_unique<RotoZoomCommand<false, true, true>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                } else {
                    push(std::make_unique<RotoZoomCommand<false, false, true>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                }
            }
        } else {
            if (mirrored) {
                if (transparent) {
                    push(std::make_unique<RotoZoomCommand<true, true, false>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                } else {
                    push(std::make_unique<RotoZoomCommand<true, false, false>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                }
            } else {
                if (transparent) {
                    push(std::make_unique<RotoZoomCommand<false, true, false>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                } else {
                    push(std::make_unique<RotoZoomCommand<false, false, false>>(minY, maxY, source, stride, width, height, ax, ay, sx, sy, cx, cy, minX, maxX));
                }
            }
        }
    }

};
