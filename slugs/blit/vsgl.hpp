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

struct TileSetEntry {
    const uint8_t* image;
    uint16_t data;
    uint16_t stride;
};

template<int screenWidth, int screenHeight, int windowHeight = 8>
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
        virtual void dispose(std::unique_ptr<Command>& ref) {
            auto tmp = std::move(next);
            ref = std::move(tmp);
        }
        static void clean() {}
    };

    template<typename Derived>
    class CreateCommand : public Command {
    public:
        template<typename ... Args>
        static std::unique_ptr<Derived> create(Args&& ... args) {
            std::unique_ptr<Derived> ret = std::make_unique<Derived>();
            ret->init(std::forward<Args>(args)...);
            return ret;
        }
    };

    template<typename Derived>
    class RecycleCommand : public Command {
    public:
        static inline std::unique_ptr<Derived> recycle;

        static void clean() {
            recycle.reset();
        }

        void dispose(std::unique_ptr<Command>& ref) override {
            std::unique_ptr<Command> tmp = std::move(this->next);
            this->next = std::move(recycle);
            recycle.reset(static_cast<Derived*>(ref.release()));
            ref = std::move(tmp);
        }

        template<typename ... Args>
        static std::unique_ptr<Derived> create(Args&& ... args) {
            std::unique_ptr<Derived> ret;
            if (!recycle) {
                ret = std::make_unique<Derived>();
                // puts("Create");
            } else {
                ret = std::move(recycle);
                recycle.reset(static_cast<Derived*>(ret->next.release()));
                // puts("Recycle");
            }
            ret->init(std::forward<Args>(args)...);
            return ret;
        }
    };

    template<typename Derived>
    using BaseCommand = RecycleCommand<Derived>;

    class TextCommand : public BaseCommand<TextCommand> {
    public:
        std::string text;
        const uint8_t* font;
        int x;

        void init(const char* text, int X, int Y, const uint8_t* font) {
            this->font = font;
            this->text = text;
            this->x = X;
            auto h = font[1];
            this->A = ((h>>3) + ((h != 8) && (h != 16)));
            this->B = pen;
            this->minY = Y;
            this->maxY = Y + h;
        }

        void draw(uint8_t* framebuffer, int Yoffset) override {
            auto color = this->B;
            auto minY = int(this->minY) - Yoffset;
            auto maxY = std::min<int>(this->maxY - Yoffset, windowHeight);
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
                skip = -minY - 1;
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

            auto maxY = std::min<int>(this->maxY - Yoffset, windowHeight);

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
    class SpriteCommand : public BaseCommand<SpriteCommand<mirror, transparent, recolor>> {
    public:
        const uint8_t* source;
        int16_t stride, x;
        // int16_t width, add;

        void init(int x, int y, int height, const uint8_t* source, int stride, int width) {
            this->source = source;
            this->stride = stride;
            this->x = x;
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

            auto maxY = std::min<int>(int(this->maxY) - Yoffset, windowHeight);
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

    using SpriteCreate = std::unique_ptr<Command> (*) (int x, int y, int height, const uint8_t* source, int stride, int width);

    static inline const SpriteCreate spriteCreate[] = {
        &SpriteCommand<false, false, false>::create,
        &SpriteCommand< true, false, false>::create,
        &SpriteCommand<false,  true, false>::create,
        &SpriteCommand< true,  true, false>::create,
        &SpriteCommand<false, false, true>::create,
        &SpriteCommand< true, false, true>::create,
        &SpriteCommand<false,  true, true>::create,
        &SpriteCommand< true,  true, true>::create
    };

    class RectCommand : public BaseCommand<RectCommand> {
    public:
        int x;

        void init(int x, int y, int width, int height) {
            this->x = x;
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

            auto maxY = std::min<int>(this->maxY - Yoffset, windowHeight);
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
    static inline const uint8_t* map = nullptr;
    static inline const TileSetEntry* tse = nullptr;
    static inline int32_t cameraX = 0, cameraY = 0;

    static void clear() {
        clearColor = pen;
        for (std::unique_ptr<Command> cmd = std::move(first); cmd;)
            cmd->dispose(cmd);
        last = nullptr;
    }

    static void push(std::unique_ptr<Command> cmd) {
        auto copy = cmd.get();
        if (last) last->next = std::move(cmd);
        else first = std::move(cmd);
        last = copy;
    }

    static void drawSprites(int y, uint8_t* framebuffer) {
        int lowerBound = y + windowHeight;
        auto prev = &first;
        while (auto cmd = prev->get()) {
            if (lowerBound > cmd->minY) {
                if (y >= cmd->maxY) {
                    cmd->dispose(*prev);
                    continue;
                }
                cmd->draw(framebuffer, y);
            }
            prev = &cmd->next;
        }
    }

    struct Header {
        uint16_t z;
        uint32_t tseOffset;
        uint8_t layerCount;
        uint16_t mapWidth;
        uint16_t mapHeight;
        uint16_t tileWidth;
        uint16_t tileHeight;
    } __attribute__ ((packed));

    static const uint8_t* drawTiles(int y, uint8_t* framebuffer, Header& header, const uint8_t* layer) {
        auto srclayer = layer;
        const int mapStride = header.mapWidth;
        const int mapHeight = header.mapHeight;
        const int tileHeight = header.tileHeight;
        const int tileWidth = header.tileWidth;

        int rowsPerWindow = windowHeight / tileHeight;
        const int rowRemainder = windowHeight % tileHeight;
        int colsPerWindow = screenWidth / header.tileWidth;
        const int colRemainder = screenWidth % header.tileWidth;

        int startY = y + cameraY;
        if (startY < 0) {
            startY %= mapHeight * tileHeight;
            startY += mapHeight * tileHeight;
        }

        int startX = cameraX;
        if (startX < 0) {
            startX %= mapStride * tileWidth;
            startX += mapStride * tileWidth;
        }

        int startRow = startY / tileHeight;
        int startRowHeight = startY % tileHeight;
        startRow %= mapHeight;

        int startCol = startX / tileWidth;
        int startColWidth = startX % tileWidth;
        startCol %= mapStride;

        if (startRowHeight) {
            startRowHeight = tileHeight - startRowHeight;
            if (startRowHeight <= rowRemainder) {
                rowsPerWindow++;
            }
        }

        if (startColWidth) {
            startColWidth = tileWidth - startColWidth;
            if (startColWidth <= colRemainder) {
                colsPerWindow++;
            }
        }

        int endRow = startRow + rowsPerWindow;
        int endRowHeight = windowHeight - startRowHeight - (rowsPerWindow - (startRowHeight ? 1 : 0)) * tileHeight;
        int endColWidth = screenWidth - startColWidth - (colsPerWindow - (startColWidth ? 1 : 0)) * tileWidth;

        auto line = framebuffer;

        // memset(framebuffer, 1, windowHeight * screenWidth);

        if (startRowHeight) {
            layer = srclayer + startRow * mapStride;
            int lineOffset = 0;
            int colOffset = startCol;

            if (startColWidth) {
                int id = layer[colOffset];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (id) {
                    id--;
                    int srcStride = tse[id].stride;
                    auto src = tse[id].image + (tileHeight - startRowHeight) * srcStride + (tileWidth - startColWidth);
                    auto cursor = line;
                    for (int i = 0; i < startRowHeight; ++i, src += srcStride, cursor += screenWidth)
                        SpriteCommand<0, 1, 0>::P_P(src, cursor, startColWidth, 0);
                }
                lineOffset = startColWidth;
            }

            for (int tx = startColWidth ? 1 : 0; tx < colsPerWindow; ++tx, lineOffset += tileWidth) {
                int id = layer[colOffset];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (!id)
                    continue;
                id--;
                int srcStride = tse[id].stride;
                auto src = tse[id].image + (tileHeight - startRowHeight) * srcStride;
                auto cursor = line + lineOffset;
                for (int i = 0; i < startRowHeight; ++i, src += srcStride, cursor += screenWidth)
                    SpriteCommand<0, 1, 0>::P_P(src, cursor, tileWidth, 0);
            }

            if (endColWidth) {
                int id = layer[colOffset];
                if (id) {
                    id--;
                    int srcStride = tse[id].stride;
                    auto src = tse[id].image + (tileHeight - startRowHeight) * srcStride;
                    auto cursor = line + lineOffset;
                    for (int i = 0; i < startRowHeight; ++i, src += srcStride, cursor += screenWidth)
                        SpriteCommand<0, 1, 0>::P_P(src, cursor, endColWidth, 0);
                }
            }

            line += startRowHeight * screenWidth;
        }

        for (int row = startRowHeight ? 1 : 0; row < rowsPerWindow; ++row, line += screenWidth * tileHeight) {
            int colOffset = startCol;

            int y = startRow + row;
            while (y >= mapHeight)
                y -= mapHeight;
            layer = srclayer + y * mapStride;

            int lineOffset = 0;

            if (startColWidth) {
                int id = layer[startCol];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (id) {
                    id--;
                    int srcStride = tse[id].stride;
                    auto src = tse[id].image + (tileWidth - startColWidth);
                    auto cursor = line;
                    for (int i = 0; i < tileHeight; ++i, src += srcStride, cursor += screenWidth)
                        SpriteCommand<0, 1, 0>::P_P(src, cursor, startColWidth, 0);
                }
                lineOffset = startColWidth;
            }

            for (int tx = startColWidth ? 1 : 0; tx < colsPerWindow; ++tx, lineOffset += tileWidth) {
                int id = layer[colOffset];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (!id)
                    continue;
                id--;
                auto src = tse[id].image;
                int srcStride = tse[id].stride;
                auto cursor = line + lineOffset;
                for (int i = 0; i < tileHeight; ++i, src += srcStride, cursor += screenWidth)
                    SpriteCommand<0, 1, 0>::P_P(src, cursor, tileWidth, 0);
            }
            if (endColWidth) {
                int id = layer[colOffset];
                if (!id)
                    continue;
                id--;
                auto src = tse[id].image;
                int srcStride = tse[id].stride;
                auto cursor = line + lineOffset;
                for (int i = 0; i < tileHeight; ++i, src += srcStride, cursor += screenWidth)
                    SpriteCommand<0, 1, 0>::P_P(src, cursor, endColWidth, 0);
            }
        }

        if (endRowHeight) {
            int lineOffset = 0;
            int colOffset = startCol;

            endRow %= mapHeight;
            layer = srclayer + endRow * mapStride;
            if (endRowHeight > tileHeight)
                endRowHeight = tileHeight;


            if (startColWidth) {
                int id = layer[colOffset];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (id) {
                    id--;
                    int srcStride = tse[id].stride;
                    auto src = tse[id].image + (tileWidth - startColWidth);
                    auto cursor = line;
                    for (int i = 0; i < endRowHeight; ++i, src += srcStride, cursor += screenWidth)
                        SpriteCommand<0, 1, 0>::P_P(src, cursor, startColWidth, 0);
                }
                lineOffset = startColWidth;
            }

            for (int tx = startColWidth ? 1 : 0; tx < colsPerWindow; ++tx, lineOffset += tileWidth) {
                int id = layer[colOffset];
                if (++colOffset == mapStride)
                    colOffset = 0;
                if (!id)
                    continue;
                id--;
                int srcStride = tse[id].stride;
                auto src = tse[id].image;
                auto cursor = line + lineOffset;
                for (int i = 0; i < endRowHeight; ++i, src += srcStride, cursor += screenWidth)
                    SpriteCommand<0, 1, 0>::P_P(src, cursor, tileWidth, 0);
            }

            if (endColWidth) {
                auto id = layer[colOffset];
                if (id) {
                    id--;
                    int srcStride = tse[id].stride;
                    auto src = tse[id].image;
                    auto cursor = line + lineOffset;
                    for (int i = 0; i < endRowHeight; ++i, src += srcStride, cursor += screenWidth)
                        SpriteCommand<0, 1, 0>::P_P(src, cursor, endColWidth, 0);
                }
            }
        }

        // for (int i = 0; i < screenWidth; i += 2)
        //     framebuffer[(windowHeight - 1) * screenWidth + i] = 40;


        return srclayer + header.mapHeight * mapStride;
    }

    static int32_t getTileProperty(int32_t x, int32_t y, uint32_t key, uint32_t layerNumber) {
        if (!map)
            return 0;

        auto& header = *(Header*)map;
        auto layer = map + sizeof(Header);
        for (int i = 0, max = header.layerCount; i < max; ++i) {
            switch (layer[0]) {
            case 0:
                layer++;
                if (i == layerNumber) {
                    const int mapStride = header.mapWidth;
                    const int mapHeight = header.mapHeight;
                    const int tileHeight = header.tileHeight;
                    const int tileWidth = header.tileWidth;

                    y += cameraY;
                    if (y < 0) {
                        y %= mapHeight * tileHeight;
                        y += mapHeight * tileHeight;
                    }

                    x += cameraX;
                    if (x < 0) {
                        x %= mapStride * tileWidth;
                        x += mapStride * tileWidth;
                    }

                    int row = (y / tileHeight) % mapHeight;
                    int col = (x /  tileWidth)  % mapStride;
                    int id = layer[row * mapStride + col];

                    if (id == 0 || tse[id - 1].data == 0)
                        return 0;

                    uint32_t* hashmap = ((uint32_t*) tse) + tse[id - 1].data;
                    auto length = *hashmap++;
                    for (uint32_t j = 0; j < length; ++j, hashmap += 2) {
                        if (hashmap[0] == key) {
                            return hashmap[1];
                        }
                    }

                    return 0;
                }
                layer += header.mapHeight * header.mapWidth;
                break;
            case 1:
                layer++;
                break;
            default:
                return 0;
            }
        }

        return 0;
    }

    static void drawMap(int y, uint8_t* framebuffer) {
        auto& header = *(Header*)map;
        auto layer = map + sizeof(Header);
        bool didDrawSprites = false;
        for (int i = 0, max = header.layerCount; i < max; ++i) {
            switch (layer[0]) {
            case 0:
                layer = drawTiles(y, framebuffer, header, layer + 1);
                break;
            case 1:
                didDrawSprites = true;
                drawSprites(y, framebuffer);
                layer++;
                break;
            default:
                return;
            }
        }
        if (!didDrawSprites)
            drawSprites(y, framebuffer);
    }

    static void preDrawClean() {
        RectCommand::clean();
        TextCommand::clean();
        SpriteCommand<true,  true,  true>::clean();
        SpriteCommand<true,  false, true>::clean();
        SpriteCommand<false, true,  true>::clean();
        SpriteCommand<false, false, true>::clean();
        SpriteCommand<true,  true,  false>::clean();
        SpriteCommand<true,  false, false>::clean();
        SpriteCommand<false, true,  false>::clean();
        SpriteCommand<false, false, false>::clean();
    }

    static void draw(uint8_t* framebuffer) {
        preDrawClean();
        auto cursor = framebuffer;
        for (int y = 0; y < screenHeight; y += windowHeight, cursor += screenWidth * windowHeight) {
            if (clearColor)
                memset(cursor, clearColor, screenWidth * windowHeight);
            if (map && tse)
                drawMap(y, cursor);
            else
                drawSprites(y, cursor);
        }
    }

    template<typename Func>
    static void draw(Func&& func) {
        // cameraY++;
        preDrawClean();
        uint8_t framebuffer[screenWidth * windowHeight];
        for (int y = 0; y < screenHeight; y += windowHeight) {
            if (clearColor)
                memset(framebuffer, clearColor, screenWidth * windowHeight);
            if (map && tse)
                drawMap(y, framebuffer);
            else
                drawSprites(y, framebuffer);
            func(framebuffer);
        }
    }

    static void text(const char* text, int x, int y) {
        push(TextCommand::create(text, x, y, font));
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
        push(RectCommand::create(x, y, w, h));
    }

    static void image(const uint8_t* data, int x, int y, bool mirrored, bool flipped, bool transparent) {
        if (!data)
            return;
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
                    push(SpriteCommand<true, true, true>::create(x, y, height, source, stride, width));
                } else {
                    push(SpriteCommand<true, false, true>::create(x, y, height, source, stride, width));
                }
            } else {
                if (transparent) {
                    push(SpriteCommand<false, true, true>::create(x, y, height, source, stride, width));
                } else {
                    push(SpriteCommand<false, false, true>::create(x, y, height, source, stride, width));
                }
            }
        } else {
            if (mirrored) {
                if (transparent) {
                    push(SpriteCommand<true, true, false>::create(x, y, height, source, stride, width));
                } else {
                    push(SpriteCommand<true, false, false>::create(x, y, height, source, stride, width));
                }
            } else {
                if (transparent) {
                    push(SpriteCommand<false, true, false>::create(x, y, height, source, stride, width));
                } else {
                    push(SpriteCommand<false, false, false>::create(x, y, height, source, stride, width));
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
