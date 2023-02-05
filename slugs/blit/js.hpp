#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <math.h>
#include <stdio.h>
#include <array>
#include <variant>
#include <algorithm>

#undef PI

#ifndef PRINT
#define PRINT(x)
#define PRINTLN()
#endif

#ifndef V_null
#define V_ js::BufferRef{}
#define V_this js::BufferRef{}
#define V___proto__ js::BufferRef{}
#define V_91Function93 js::BufferRef{}
#define V_91Object93 js::BufferRef{}
#define V_91Array93 js::BufferRef{}
#define V_91Resource93 js::BufferRef{}
#define V_buffer js::BufferRef{}
#define V_null js::BufferRef{}
#define V_0 js::BufferRef{}
#define V_1 js::BufferRef{}
#define V_2 js::BufferRef{}
#define V_3 js::BufferRef{}
#define V_true js::BufferRef{}
#define V_false js::BufferRef{}
#define V_length js::BufferRef{}
#define V_buffer js::BufferRef{}
#define V_undefined js::BufferRef{}
#define V_4747method js::BufferRef{}
#define V_4747new js::BufferRef{}
#endif

#define ENABLE_PROFILER 0

#if ENABLE_PROFILER
#define PROFILER js::Profiler _profiler(__func__)
#else
#define PROFILER 0
#endif

namespace js {
    struct Buffer;
    class Object;
    class Local;
}

extern js::Buffer* const stringTable[];
extern const uint32_t stringTableSize;

namespace js {

    inline uint8_t markGen = 1;
    inline uint32_t heapSize = 0;
    inline uint32_t maxHeapSize = 16 * 1024;
    inline const char* volatile _currentFunction = "NONE";
    inline uint32_t recycleCount = 0;
    inline uint32_t gcCount = 0;
    inline uint32_t freeCount = 0;

    class Profiler {
    public:
        const char* prev;
        Profiler(const char* current) {
            prev = _currentFunction;
            _currentFunction = current;
        }
        ~Profiler() {
            _currentFunction = prev;
        }
    };

    inline void debug(){
        printf("dbg");
    }

    inline void* aligned_malloc(size_t size) {
        PROFILER;
        if (size & 3) {
            size += 4 - (size & 3);
        }
        return new uint32_t[size >> 2];
        // auto ret = reinterpret_cast<uint8_t*>(malloc(size + 4)) + 1;
        // auto off = 4 - (reinterpret_cast<uintptr_t>(ret) & 3);
        // if (off == 4)
        //     off = 0;
        // ret += off;
        // ret[-1] = off;
        // return ret;
    }

    inline void aligned_free(void* ptr) {
        PROFILER;
        auto arr = reinterpret_cast<uint32_t*>(ptr);
        delete[] arr;
        // if (!ptr)
        //     return;
        // auto ret = reinterpret_cast<uint8_t*>(ptr);
        // auto off = ret[-1];
        // ret -= off;
        // free(ret - 1);
    }

    template<typename ... Ts>
    struct Overload : Ts ... {
        using Ts::operator() ...;

        template<typename variant>
        auto operator() (variant&& v) const {
            return std::visit(*this, v);
        }
    };
    template<class ... Ts> Overload(Ts...) -> Overload<Ts...>;


    inline uint32_t hash(const char* ptr) {
        PROFILER;
        uint32_t v1 = 5381, v2 = 2166136261;
        while(uint32_t c = *ptr++){
            v1 = (v1 * 251) ^ c;
            v2 = (v2 ^ c) * 16777619;
        }
        return v1 * 13 + v2;

        // uint32_t acc = 991;
        // while (*ptr) {
        //     acc ^= (acc << 7) ^ (acc >> 5);
        //     acc ^= *ptr++ * 7919;
        // }
        // return acc;
    }

    inline constexpr uint32_t nextPOT(uint32_t o) {
        uint32_t s = o;
        s |= s >> 16;
        s |= s >> 8;
        s |= s >> 4;
        s |= s >> 2;
        s |= s >> 1;
        s += 1;
        return (s == (o << 1)) ? o : s;
    }

    enum class Undefined {Undefined};

    struct Buffer {
        uint16_t refCount;
        uint16_t size;
        uint32_t hash;
    };

    template <uint32_t size>
    inline constexpr auto bufferFrom(const char data[size]) -> std::array<uint8_t, size + sizeof(Buffer)> {
        union {
            std::array<uint8_t, sizeof(Buffer) + size> ret;
            struct {
                Buffer buffer;
                uint8_t data[size];
            };
        } u;
        for (uint32_t i = 0; i < size; ++i) {
            u.data[i] = data[i];
        }
        u.buffer.size = size;
        u.buffer.refCount = 0;
        u.buffer.hash = hash(data);
        return u.ret;
    }

    class BufferRef {
        Buffer* buffer = nullptr;

    public:
        BufferRef() = default;

        BufferRef(const BufferRef& other) noexcept {buffer = other.buffer; hold();}

        BufferRef(BufferRef&& other) noexcept {std::swap(buffer, other.buffer);}

        explicit BufferRef(const Buffer* buffer) : buffer{const_cast<Buffer*>(buffer)} {hold();}

        explicit BufferRef(const uint8_t* buffer) : buffer{reinterpret_cast<Buffer*>(const_cast<uint8_t*>(buffer))} {}

        ~BufferRef() {release();}

        BufferRef& operator = (const BufferRef& other) noexcept {
            PROFILER;
            release();
            buffer = other.buffer;
            hold();
            return *this;
        }

        operator bool () const {
            return !!buffer;
        }

        Buffer* operator -> () const {
            return buffer;
        }

        uint8_t* data() const {
            if (!buffer)
                return nullptr;
            return reinterpret_cast<uint8_t*>(buffer + 1);
        }

        bool operator == (const BufferRef& other) const {
            PROFILER;
            if (buffer == other.buffer)
                return true;
            if (!buffer || !other.buffer) {
                // printf("1 %d, %d\n", !buffer, !other.buffer);
                return false;
            }
            if (std::max<uint32_t>(13, buffer->size) != std::max<uint32_t>(13, other.buffer->size)) {
                // printf("2 %d, %d\n", buffer->size, other.buffer->size);
                return false;
            }
            if (!buffer->hash)
                buffer->hash = hash((char*)data());
            if (!other.buffer->hash)
                other.buffer->hash = hash((char*)other.data());
            if (buffer->hash != other.buffer->hash) {
                // printf("3 %d, %d\n", buffer->hash, other.buffer->hash);
                return false;
            }
            return true;
        }

        bool operator != (const BufferRef& other) const {
            return !(*this == other);
        }

        BufferRef& operator = (BufferRef&& other) noexcept {
            std::swap(buffer, other.buffer);
            return *this;
        }

        void hold() {
            PROFILER;
            if (buffer && buffer->refCount) {
                buffer->refCount++;
            }
        }

        void release() {
            PROFILER;
            if (buffer && buffer->refCount) {
                buffer->refCount--;
                if (buffer->refCount == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
                    aligned_free(buffer);
#pragma GCC diagnostic pop
                }
            }
            buffer = nullptr;
        }
    };

    using RawFunction = Local (*)(Local&, bool);

    struct ResourceRef;

    using Tagged = std::variant<
        Undefined,      // 0
        Object*,        // 1
        RawFunction,    // 2
        ResourceRef*,   // 3
        BufferRef,      // 4
        int32_t,        // 5
        uint32_t,       // 6
        bool,           // 7
        float           // 8
        >;

    inline const char* tagType(const Tagged& val) {
        PROFILER;
        const char* types[] = {
            "{undefined}",
            "{Object}",
            "{Function}",
            "{ResourceRef}",
            "{BufferRef}",
            "{int32_t}",
            "{uint32_t}",
            "{bool}",
            "{float}"
        };
        return types[val.index()];
    }

    template<typename T>
    bool has(const Tagged& v) {
        PROFILER;
        return v.index() == Tagged{T{}}.index();
    }

    BufferRef toString(const Tagged& val);

    struct Bucket {
        BufferRef key;
        Tagged value;
    };

#define ctz(v) __builtin_ctz(v)

    inline constexpr const uint32_t minObjectSize = 32;
    inline constexpr const uint32_t maxObjectSize = 2048;
    inline Object* objectList[ctz(maxObjectSize) - ctz(minObjectSize) + 1];

    class Object final {
        Object* next = nullptr;
        Object* prev = nullptr;
    public:

        const uint32_t _bucketCount;
        uint16_t rootRefCount = 0;
        uint8_t mark = 0;
        uint8_t flags = 0;

        enum class Flag : uint32_t {
            String = 0,
            Array,
            Frozen
        };

        Object(uint32_t bucketCount) : _bucketCount{bucketCount} {
            PROFILER;
            auto list = getObjectList(bucketCount);
            // printf("add to ol%d\n", list);
            if (objectList[list])
                objectList[list]->next = this;
            prev = objectList[list];
            next = nullptr;
            objectList[list] = this;

            auto buckets = this->buckets();
            for (uint32_t i = 0; i < _bucketCount; ++i) {
                new (&buckets[i]) Bucket();
            }
        }

        ~Object() {
            PROFILER;
            auto list = getObjectList(_bucketCount);
            // printf("del from ol%d\n", list);
            if (objectList[list] == this)
                objectList[list] = prev;
            if (prev)
                prev->next = next;
            if (next)
                next->prev = prev;
            auto buckets = this->buckets();
            for (uint32_t i = 0; i < _bucketCount; ++i) {
                buckets[i].~Bucket();
            }
        }

        void clear() {
            PROFILER;
            flags = 0;
            mark = 0;
            auto buckets = this->buckets();
            for (uint32_t i = 0; i < _bucketCount; ++i) {
                buckets[i].~Bucket();
                new (&buckets[i]) Bucket();
            }
        }

        void hold() {
            PROFILER;
            rootRefCount++;
        }

        void release() {
            PROFILER;
            rootRefCount--;
            if (int(rootRefCount) < 0)
                debug();
        }

        uint32_t isRooted() {
            PROFILER;
            return rootRefCount;
        }

        bool isMarked() {
            PROFILER;
            return mark == markGen;
        }

        void setMark() {
            PROFILER;
            if (mark == markGen)
                return;
            mark = markGen;
            auto buckets = this->buckets();
            for (uint32_t i = 0; i < _bucketCount; ++i) {
                if (auto obj = std::get_if<Object*>(&buckets[i].value); obj && *obj) {
                    (*obj)->setMark();
                }
            }
        }

        bool canRecycle() {
            PROFILER;
            return !rootRefCount && !mark;
        }

        void setFlagString() {
            PROFILER;
            flags |= 1 << uint32_t(Flag::String);
        }

        void setFlagArray() {
            PROFILER;
            flags |= 1 << uint32_t(Flag::Array);
        }

        bool isString() {
            PROFILER;
            return flags & (1 << uint32_t(Flag::String));
        }

        bool isArray() {
            PROFILER;
            return flags & (1 << uint32_t(Flag::Array));
        }

        class iterator {
            Object* ptr;
        public:
            iterator(Object* ptr = nullptr) : ptr{ptr} {}

            Object& operator * () {
                return *ptr;
            }

            Object* operator -> () {
                return ptr;
            }

            operator bool () {
                return ptr != nullptr;
            }

            iterator& operator ++ () {
                ptr = ptr->prev;
                return *this;
            }
        };

        static constexpr uint32_t getTotalSize(uint32_t bucketCount) {
            uint32_t size = sizeof(Object);
            size += (bucketCount) * sizeof(Bucket);
            auto pot = nextPOT(size);
            return pot;
        }

        static constexpr uint32_t getObjectList(uint32_t bucketCount) {
            auto size = getTotalSize(bucketCount);
            auto index = uint32_t(ctz(size) - ctz(minObjectSize));
            auto limit = sizeof(objectList)/sizeof(objectList[0]) - 1;
            return index > limit ? limit : index;
        }

        static iterator erase(iterator it) {
            PROFILER;
            if (!it)
                return {};

            auto ret = it;
            ++ret;

            auto obj = &*it;
            auto size = getTotalSize(obj->bucketCount());
            obj->~Object();
            aligned_free(obj);
            heapSize -= size;

            return ret;
        }

        iterator begin() {
            PROFILER;
            return {objectList[getObjectList(_bucketCount)]};
        }

        iterator end() {
            return {nullptr};
        }

        // Bucket[] buckets;

        Bucket* buckets() {
            return reinterpret_cast<Bucket*>(this + 1);
        }

        uint32_t bucketCount() {
            return _bucketCount;
        }
    };

    class Local {
    public:
        Tagged data;
        Local() = default;

        Local(const Tagged& ptr) {
            *this = ptr;
        }

        Local(const Local& other) {
            *this = other.data;
        }

        Local(Local&& other) {
            std::swap(data, other.data);
        }

        ~Local() {
            PROFILER;
            if (auto ptr = std::get_if<Object*>(&data); ptr && *ptr) {
                (*ptr)->release();
            }
        }

        operator Tagged& () {
            return data;
        }

        Object* object() {
            PROFILER;
            auto ptr = std::get_if<Object*>(&data);
            if (ptr)
                return *ptr;
            return nullptr;
        }

        BufferRef bufferRef() {
            PROFILER;
            auto ptr = std::get_if<BufferRef>(&data);
            if (ptr)
                return *ptr;
            return {};
        }

        void reset() {
            PROFILER;
            if (auto ptr = object()) {
                ptr->release();
            }
            data = Undefined::Undefined;
        }

        Local& operator = (Local&& other) {
            std::swap(data, other.data);
            return *this;
        }

        Local& operator = (const Local& other) {
            *this = other.data;
            return *this;
        }

        Local& operator = (const Tagged& ptr) {
            if (auto obj = object()) {
                data = ptr;
                if (auto nobj = object())
                    nobj->hold();
                obj->release();
            } else {
                data = ptr;
                if (auto nobj = object())
                    nobj->hold();
            }
            return *this;
        }
    };

    inline Tagged* set(Tagged& container, const BufferRef& key, const Tagged& val) {
        PROFILER;
        auto ptr = std::get_if<Object*>(&container);
        if (!ptr || !*ptr)
            return nullptr;

        auto objptr = *ptr;
        if (!objptr)
            return nullptr;

        auto buckets= objptr->buckets();
        auto bucketCount = objptr->bucketCount();

        if (!bucketCount)
            return nullptr;

        uint32_t h = key->hash;
        if (!h)
            h = key->hash = hash((const char*)key.data());
        uint32_t pos = h % bucketCount;

        // printf("Looking for %lu ", (uintptr_t) h);
        // PRINT(key.data());
        // PRINTLN();

        for (uint32_t i = 0; i < bucketCount; ++i) {
            auto& bucket = buckets[pos++];
            if (pos == bucketCount)
                pos = 0;
            if (!bucket.key) {
                bucket.key = key;
                // printf("Found empty bucket %d\n", i);
            }else if (!(bucket.key == key)) {
                // printf("collision with key ");
                // PRINT(bucket.key.data());
                // PRINTLN();
                continue;
            }
            bucket.value = val;
            if (auto obj = std::get_if<Object*>(&val); obj && *obj) {
                (*obj)->setMark();
            }
            return &bucket.value;
        }
        return nullptr;
    }

    inline Tagged* set(Tagged& container, const Tagged& key, const Tagged& val) {
        return set(container, toString(key), val);
    }

    inline Tagged* getTaggedPtr(Object* objptr, const BufferRef& key, bool recursive = false) {
        PROFILER;
        if (!objptr) {
            // PRINT("Null object lookup");
            // PRINTLN();
            return nullptr;
        }

        auto buckets = objptr->buckets();
        auto bucketCount = objptr->bucketCount();

        if (!bucketCount)
            return nullptr;

        uint32_t h = key->hash;
        if (!h)
            h = key->hash = hash((const char*)key.data());

        uint32_t pos = h % bucketCount;
        for (uint32_t i = 0; i < bucketCount; ++i) {
            auto& bucket = buckets[pos++];
            if (pos == bucketCount)
                pos = 0;
            if (!bucket.key) {
                break;
            }
            if (bucket.key == key) {
                // PRINT("Found ");
                // PRINT(key.data());
                // PRINTLN();
                return &bucket.value;
            }
        }

        if (recursive) {
            if (auto ptr = getTaggedPtr(objptr, V___proto__)) {
                if (auto objptr = std::get_if<Object*>(ptr); objptr && *objptr) {
                    // PRINT("Proto lookup for ");
                    // PRINT(key.data());
                    // PRINTLN();
                    return getTaggedPtr(*objptr, key, true);
                }
            } else {
                // PRINT("No Proto ");
            }
            // PRINT("Recursive Could not find ");
            // PRINT(key.data());
            // PRINTLN();
        } else {
            // PRINT("Could not find ");
            // PRINT(key.data());
            // PRINTLN();
        }

        return nullptr;
    }

    inline Tagged& get(const Tagged& container, const BufferRef& key) {
        PROFILER;
        static Tagged undef;
        auto object = std::get_if<Object*>(&container);
        if (object && *object) {
            if (auto ptr = getTaggedPtr(*object, key, true))
                return {*ptr};
        }

        undef = {};
        return undef;
    }

    inline Tagged& get(const Tagged& container, const Tagged& key) {
        return get(container, toString(key));
    }

    inline void gc() {
        PROFILER;
        gcCount++;
        markGen++;
        markGen += !markGen;

        {
            js::Profiler _profiler("mark");
            for (uint32_t i = 0; i < sizeof(objectList)/sizeof(objectList[0]); ++i ){
                // printf("ol%d = %p\n", i, objectList[i]);
                for (auto it = Object::iterator{objectList[i]}; it; ++it) {
                    // printf("rooted = %d\n", (int)it->isRooted());
                    if (it->rootRefCount && it->mark != markGen) {
                        it->setMark();
                    }
                }
            }
        }

        {
            js::Profiler _profiler("sweep");
                for (uint32_t i = 0; i < sizeof(objectList)/sizeof(objectList[0]); ++i ){
                    for (auto it = Object::iterator{objectList[i]}; it;) {
                        if (it->mark != markGen) {
                            js::freeCount += Object::getTotalSize(it->bucketCount());
                            it = Object::erase(it);
                            // printf("collect\n");
                        } else {
                            ++it;
                        }
                    }
                }
        }
    }

    inline Local alloc(uint32_t propCount, const Tagged& proto = {}) {
        PROFILER;
        auto hasProto = proto.index() != Tagged{}.index();
        if (hasProto)
            propCount++;

        auto size = Object::getTotalSize(propCount);
        auto adjustedCount = (size - sizeof(Object)) / sizeof(Bucket);
        auto list = Object::getObjectList(propCount);

        Local ret;

        for (auto it = Object::iterator{objectList[list]}; it; ++it) {
            if (it->canRecycle()) {
                ret = &*it;
                it->clear();
                recycleCount++;
                // printf("recycled\n");
                break;
            }
        }

        if (has<Undefined>(ret)) {
            heapSize += size;
            if (heapSize > maxHeapSize)
                gc();
            auto raw = aligned_malloc(size);
            ret = new (raw) Object(adjustedCount);
            // printf("created\n");
        }

        if (hasProto) {
            set(ret, V___proto__, proto);
        }
        return ret;
    }

    inline BufferRef allocBuffer(uint32_t size) {
        PROFILER;
        gc();
        auto raw = aligned_malloc(nextPOT(sizeof(Buffer) + size));
        auto buffer = new (raw) Buffer();
        buffer->refCount = 0;
        buffer->size = size;
        buffer->hash = 0;
        BufferRef ref{buffer};
        buffer->refCount = 1;
        return ref;
    }

    inline BufferRef toString(const char* src) {
        PROFILER;
        auto str = allocBuffer(strlen(src) + 1);
        strcpy((char*)str.data(), src);
        return str;
    }

    inline BufferRef toString(const Tagged& val) {
        PROFILER;
        return Overload {
            [] (Undefined) -> BufferRef {
                return V_undefined;
            },

            [] (Object* val) -> BufferRef {
                if (!val) {
                    return V_null;
                } else if (val->isString()) {
                    auto ptr = getTaggedPtr(val, V_buffer);
                    if (!ptr)
                        return V_;
                    return Local{*ptr}.bufferRef();
                } else if (val->isArray()) {
                    return V_91Array93;
                } else {
                    return V_91Object93;
                }
            },

            [] (ResourceRef*) -> BufferRef {
                return V_91Resource93;
            },

            [] (RawFunction) -> BufferRef {
                return V_91Function93;
            },

            [] (BufferRef val) -> BufferRef {
                return val;
            },

            [] (const uint32_t& v) -> BufferRef {
                auto val = v;
                if (val == 0) {
                    return V_0;
                }
                uint32_t p = 12;
                auto str = allocBuffer(p + 1);
                auto tmp = str.data();
                tmp[p--] = 0;
                while (val) {
                    auto n = val / 10;
                    tmp[p--] = '0' + (val - n * 10);
                    val = n;
                }
                for (uint32_t i = 0; p <= 12; ++i) {
                    tmp[i] = tmp[++p];
                }
                return str;
            },

            [] (const int32_t& v) -> BufferRef {
                auto val = v;
                if (val == 0) {
                    return V_0;
                }
                uint32_t p = 12;
                bool negative = val < 0;
                if (negative)
                    val = -val;
                auto str = allocBuffer(p + 1);
                auto tmp = str.data();
                tmp[p--] = 0;
                while (val) {
                    auto n = val / 10;
                    tmp[p--] = '0' + (val - n * 10);
                    val = n;
                }
                if (negative)
                    tmp[p--] = '-';
                for (uint32_t i = 0; p <= 12; ++i) {
                    tmp[i] = tmp[++p];
                }
                return str;
            },

            [] (const bool& v) -> BufferRef {
                auto val = v;
                return val ? V_true : V_false;
            },

            [] (const float& v) -> BufferRef {
                auto val = v;
                if (val == 0) {
                    return V_0;
                }
                uint32_t p = 16;
                int ival = val * 1000;
                bool negative = ival < 0;
                if (negative)
                    ival = -ival;
                auto str = allocBuffer(p + 1);
                auto tmp = str.data();
                tmp[p--] = 0;
                while (ival) {
                    auto n = ival / 10;
                    tmp[p--] = '0' + (ival - n * 10);
                    ival = n;
                    if (p == 12) {
                        tmp[p--] = '.';
                    }
                }
                if (negative)
                    tmp[p--] = '-';
                for (uint32_t i = 0; p <= 16; ++i) {
                    tmp[i] = tmp[++p];
                }
                return str;
            }
        }(val);
    }

    inline Local arguments(uint32_t len) {
        PROFILER;
        auto args = alloc(len + 2);
        if (!args.object()) {
            PRINT("NULL ALLOC");
            PRINTLN();
        }
        args.object()->setFlagArray();
        set(args, V_length, len);
        return args;
    }

    inline Local string(const BufferRef& ref) {
        PROFILER;
        // static Local strProto = ([]{
        //         auto ret = alloc(1);
        //         return ret;
        //     })();
        auto str = alloc(2);
        str.object()->setFlagString();
        set(str, V_buffer, ref);
        return str;
    }

    inline Local string(uint32_t index) {
        return string(BufferRef{stringTable[index]});
    }

    template <typename Type>
    Type to(const Tagged& l) { static_assert(std::holds_alternative<Type>(l)); return {};}

    template <>
    inline Object* to<Object*>(const Tagged& l) {
        auto ptr = std::get_if<Object*>(&l);
        return ptr ? *ptr : nullptr;
    }

    template <>
    inline float to<float>(const Tagged& l) {
        return Overload {
            [](Undefined) -> float {return 0.0f;},
            [](Object*) -> float {return 0.0f;},
            [](ResourceRef*) -> float {return 0.0f;},
            [](const RawFunction&) -> float {return 0.0f;},
            [](const BufferRef&) -> float {return 0.0f;},
            [](const int32_t& a) -> float {return a;},
            [](const uint32_t& a) -> float {return a;},
            [](const bool& a) -> float {return a ? 1.0f : 0.0f;},
            [](const float& v) -> float {return v;}
        }(l);
    }

    template <>
    inline int32_t to<int32_t>(const Tagged& l) {
        return Overload {
            [](Undefined) -> int32_t {return 0;},
            [](Object*) -> int32_t {return 0;},
            [](ResourceRef*) -> int32_t {return 0;},
            [](const RawFunction&) -> int32_t {return 0;},
            [](const BufferRef&) -> int32_t {return 0;},
            [](const int32_t& a) -> int32_t {return a;},
            [](const uint32_t& a) -> int32_t {return a;},
            [](const bool& a) -> int32_t {return a;},
            [](const float& v) -> int32_t {return v;}
        }(l);
    }

    template <>
    inline uint32_t to<uint32_t>(const Tagged& l) {
        return Overload {
            [](Undefined) -> uint32_t {return 0;},
            [](Object*) -> uint32_t {return 0;},
            [](ResourceRef* ) -> uint32_t {return 0;},
            [](const RawFunction& ) -> uint32_t {return 0;},
            [](const BufferRef&) -> uint32_t {return 0;},
            [](const int32_t& a) -> uint32_t {return a;},
            [](const uint32_t& a) -> uint32_t {return a;},
            [](const bool& a) -> uint32_t {return a;},
            [](const float& v) -> uint32_t {return v;}
        }(l);
    }

    template <>
    inline bool to<bool>(const Tagged& l) {
        return Overload {
            [](Undefined){return false;},
            [](Object* a){return !!a;},
            [](ResourceRef* a){return !!a;},
            [](const RawFunction&){return true;},
            [](const BufferRef& a){return a && a->size && a.data()[0];},
            [](const int32_t& a){return !!a;},
            [](const uint32_t& a){return !!a;},
            [](const bool& a){return a;},
            [](const float& v){return v != 0.0f;}
        }(l);
    }

    template <>
    inline BufferRef to<BufferRef>(const Tagged& l) {
        return toString(l);
    }

    inline void op_mul(Local& out, const Tagged& left, const Tagged& right) {
        if (has<float>(left) || has<float>(right)) {
            out = to<float>(left) * to<float>(right);
        } else {
            out = to<int32_t>(left) * to<int32_t>(right);
        }
    }

    inline void op_mod(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) % to<int32_t>(right);
    }

    inline Local call(RawFunction func, Local& args, bool isNew) {
        if (!isNew)
            return func(args, false);

        auto ret = func(args, true);
        if (!ret.data.index())
            ret = get(args, V_this);
        return ret;
    }

    inline Local call(const Tagged& data, Local& args, bool isNew) {
            if (auto func = std::get_if<RawFunction>(&data))
                return call(*func, args, isNew);

            if (auto obj = get(data, V_4747method); obj.index()) {
                set(args, V___proto__, data);
                return call(obj, args, isNew);
            }

            PRINT(tagType(data));
            PRINT(" is not a function");
            PRINTLN();
            debug();
            return {};
    }

    inline void initThis(Local& that, Local& args, uint32_t size, bool isNew) {
        if (isNew) {
            that = alloc(size);
            set(args, V_this, that);
        } else {
            that = get(args, V_this);
        }
    }

    inline void op_div(Local& out, const Tagged& left, const Tagged& right) {
        out = to<float>(left) / to<float>(right);
    }

    inline void op_sub(Local& out, const Tagged& left, const Tagged& right) {
        out = to<float>(left) - to<float>(right);
    }

    inline void op_or(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) | to<int32_t>(right);
    }

    inline void op_and(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) & to<int32_t>(right);
    }

    inline void op_xor(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) ^ to<int32_t>(right);
    }

    inline void op_shl(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) << to<int32_t>(right);
    }

    inline void op_shr(Local& out, const Tagged& left, const Tagged& right) {
        out = to<int32_t>(left) >> to<int32_t>(right);
    }

    inline void op_sru(Local& out, const Tagged& left, const Tagged& right) {
        out = to<uint32_t>(left) >> to<uint32_t>(right);
    }

    inline void op_leq(Local& out, const Tagged& left, const Tagged& right) {
        if (has<float>(left) || has<float>(right)) {
            out = to<float>(left) <= to<float>(right);
        } else {
            out = to<int32_t>(left) <= to<int32_t>(right);
        }
    }

    inline void op_lt(Local& out, const Tagged& left, const Tagged& right) {
        if (has<float>(left) || has<float>(right)) {
            out = to<float>(left) < to<float>(right);
        } else {
            out = to<int32_t>(left) < to<int32_t>(right);
        }
    }

    inline void op_geq(Local& out, const Tagged& left, const Tagged& right) {
        if (has<float>(left) || has<float>(right)) {
            out = to<float>(left) >= to<float>(right);
        } else {
            out = to<int32_t>(left) >= to<int32_t>(right);
        }
    }

    inline void op_gt(Local& out, const Tagged& left, const Tagged& right) {
        if (has<float>(left) || has<float>(right)) {
            out = to<float>(left) > to<float>(right);
        } else {
            out = to<int32_t>(left) > to<int32_t>(right);
        }
    }

    inline void op_eq(Local& out, const Tagged& left, const Tagged& right) {
        if (has<Object*>(left) ||
            has<Object*>(right) ||
            has<BufferRef>(left) ||
            has<BufferRef>(right)) {
            auto l = to<BufferRef>(left);
            auto r = to<BufferRef>(right);
            out = l == r;
        } else if (has<float>(left) ||
                   has<float>(right)) {
            out = to<float>(left) == to<float>(right);
        } else {
            out = to<int32_t>(left) == to<int32_t>(right);
        }
    }

    inline void op_neq(Local& out, const Tagged& left, const Tagged& right) {
        if (has<Object*>(left) ||
            has<Object*>(right) ||
            has<BufferRef>(left) ||
            has<BufferRef>(right)) {
            auto l = to<BufferRef>(left);
            auto r = to<BufferRef>(right);
            out = l != r;
        } else if (has<float>(left) ||
                   has<float>(right)) {
            out = to<float>(left) != to<float>(right);
        } else {
            out = to<int32_t>(left) != to<int32_t>(right);
        }
    }

    inline void op_seq(Local& out, const Tagged& left, const Tagged& right) {
        if (left.index() != right.index()) {
            out = false;
            return;
        }
        out = Overload {
            [&](Undefined){return true;},
            [&](Object* left){return left == std::get<decltype(left)>(right);},
            [&](const RawFunction& left){return left == std::get<RawFunction>(right);},
            [&](ResourceRef* left){return left == std::get<ResourceRef*>(right);},
            [&](const BufferRef& left){return left == std::get<BufferRef>(right);},
            [&](const int32_t& left){return left == std::get<int32_t>(right);},
            [&](const uint32_t& left){return left == std::get<uint32_t>(right);},
            [&](const bool& left){return left == std::get<bool>(right);},
            [&](const float& left){return left == std::get<float>(right);}
        }(left);
    }

    inline void op_sneq(Local& out, const Tagged& left, const Tagged& right) {
        if (left.index() != right.index()) {
            out = true;
            return;
        }
        out = Overload {
            [&](Undefined){return false;},
            [&](Object* left){return left != std::get<decltype(left)>(right);},
            [&](const RawFunction& left){return left != std::get<RawFunction>(right);},
            [&](ResourceRef* left){return left != std::get<ResourceRef*>(right);},
            [&](const BufferRef& left){return left != std::get<BufferRef>(right);},
            [&](const int32_t& left){return left != std::get<int32_t>(right);},
            [&](const uint32_t& left){return left != std::get<uint32_t>(right);},
            [&](const bool& left){return left != std::get<bool>(right);},
            [&](const float& left){return left != std::get<float>(right);}
        }(left);
    }

    inline void op_add(Local& out, const Tagged& left, const Tagged& right) {
        if (has<Object*>(left) ||
            has<Object*>(right) ||
            has<BufferRef>(left) ||
            has<BufferRef>(right)) {
            auto l = to<BufferRef>(left);
            auto r = to<BufferRef>(right);
            auto o = allocBuffer(l->size + r->size - 1);
            strcpy((char*)o.data(), (const char*)l.data());
            strcat((char*)o.data(), (const char*)r.data());
            out = string(o);
        } else if (has<float>(left) ||
                   has<float>(right)) {
            out = to<float>(left) + to<float>(right);
        } else {
            out = to<int32_t>(left) + to<int32_t>(right);
        }
    }

    inline void op_inc(Local& out, Tagged& src) {
        out = src;
        if (auto sval = std::get_if<int32_t>(&src)) {
            src = (*sval) + 1;
        } else {
            src = to<float>(src) + 1;
        }
    }

    inline void op_preinc(Local& out, Tagged& src) {
        if (auto sval = std::get_if<int32_t>(&src)) {
            src = (*sval) + 1;
        } else {
            src = to<float>(src) + 1;
        }
        out = src;
    }

    inline void op_dec(Local& out, Tagged& src) {
        out = src;
        if (auto sval = std::get_if<int32_t>(&src)) {
            src = (*sval) - 1;
        } else {
            src = to<float>(src) - 1;
        }
    }

    inline void op_predec(Local& out, Tagged& src) {
        if (auto sval = std::get_if<int32_t>(&src)) {
            src = (*sval) - 1;
        } else {
            src = to<float>(src) - 1;
        }
        out = src;
    }

    inline void op_neg(Local& out, const Tagged& src) {
        if (auto sval = std::get_if<int32_t>(&src)) {
            out = -*sval;
        }else {
            out = -to<float>(src);
        }
    }

    inline void op_pos(Local& out, const Tagged& src) {
        out = to<float>(src);
    }

    inline void op_not(Local& out, const Tagged& src) {
        out = !to<bool>(src);
    }
}

extern js::Local PI; // var
extern js::Local HALF_PI; // var
extern js::Local TWO_PI; // var

inline js::Local debug(js::Local& args, bool) {
    auto len = js::to<uint32_t>(get(args, V_length));
    for (uint32_t i = 0; i < len; ++i) {
        auto key = js::toString(i);
        auto var = get(args, key);
        if (i)
            PRINT(" ");
        if (auto obj = std::get_if<js::Object*>(&var); obj && *obj && !(*obj)->isString()) {
            auto ptr = *obj;
            PRINT(ptr->isArray() ? "[" : "{");
            auto buckets = ptr->buckets();
            auto bucketCount = ptr->bucketCount();
            bool first = true;
            for (uint32_t b = 0; b < bucketCount; ++b) {
                auto& bucket = buckets[b];
                if (bucket.value.index() == 0)
                    continue;
                if (!first)
                    PRINT(", ");
                first = false;

                /*
                PRINT(js::toString(bucket.key->hash % bucketCount).data());
                PRINT("/");
                */

                PRINT(bucket.key.data());
                PRINT(":");
                PRINT(js::toString(bucket.value).data());
            }
            PRINT(ptr->isArray() ? "]" : "}");
        } else {
            PRINT(js::toString(var).data());
        }
    }
    PRINTLN();
    return {};
}

inline js::Local Array(js::Local& args, bool) {
    return js::arguments(js::to<uint32_t>(get(args, V_0)));
}

inline js::Local rand(js::Local& args, bool) {
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = int32_t(rand());
    switch (len) {
    case 0:
        return {ret / float(RAND_MAX)};

    case 1:
        return {ret % js::to<int32_t>(get(args, V_0))};

    case 2: {
        auto a = js::to<float>(get(args, V_0));
        auto b = js::to<float>(get(args, V_1));
        float min, range;
        if (a < b) {
            min = a;
            range = b - a;
        } else if (a > b) {
            min = b;
            range = a - b;
        } else {
            return {a};
        }
        return {min + ret / float(RAND_MAX) * range};
    }

    case 3: {
        auto a = js::to<int32_t>(get(args, V_0));
        auto b = js::to<int32_t>(get(args, V_1));
        return {ret % (b - a) + a};
    }

    }

    return {ret};
}

inline js::Local abs(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    if (auto sval = std::get_if<int32_t>(&arg0)) {
        if (*sval < 0)
            return {-*sval};
        return {*sval};
    }else{
        return {-js::to<float>(arg0)};
    }
}

inline js::Local floor(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {js::to<int32_t>(arg0)};
}

inline js::Local round(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(int32_t)(js::to<float>(arg0) + 0.5f)};
}

inline js::Local ceil(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(int32_t) ceilf(js::to<float>(arg0))};
}

inline js::Local cos(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(float) cosf(js::to<float>(arg0))};
}

inline js::Local sin(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(float) sinf(js::to<float>(arg0))};
}

inline js::Local atan2(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    auto& arg1 = js::get(args, V_1);
    return {(float) atan2f(js::to<float>(arg0), js::to<float>(arg1))};
}

inline js::Local tan(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(float) tanf(js::to<float>(arg0))};
}

inline js::Local sqrt(js::Local& args, bool) {
    auto& arg0 = js::get(args, V_0);
    return {(float) sqrtf(js::to<float>(arg0))};
}

inline js::Local min(js::Local& args, bool) {
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<float>(get(args, V_0));
    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            if (auto v = js::to<float>(get(args, js::toString(i))); v < ret)
                ret = v;
        }
        [[fallthrough]];
    case 4:
        if (auto v = js::to<float>(get(args, V_3)); v < ret)
            ret = v;
        [[fallthrough]];
    case 3:
        if (auto v = js::to<float>(get(args, V_2)); v < ret)
            ret = v;
        [[fallthrough]];
    case 2:
        if (auto v = js::to<float>(get(args, V_1)); v < ret)
            ret = v;
        [[fallthrough]];
    case 1:
    case 0: break;
    }
    return {ret};
}

inline js::Local max(js::Local& args, bool) {
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<float>(get(args, V_0));
    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            if (auto v = js::to<float>(get(args, js::toString(i))); v > ret)
                ret = v;
        }
        [[fallthrough]];
    case 4:
        if (auto v = js::to<float>(get(args, V_3)); v > ret)
            ret = v;
        [[fallthrough]];
    case 3:
        if (auto v = js::to<float>(get(args, V_2)); v > ret)
            ret = v;
        [[fallthrough]];
    case 2:
        if (auto v = js::to<float>(get(args, V_1)); v > ret)
            ret = v;
        [[fallthrough]];
    case 1:
    case 0: break;
    }
    return {ret};
}

inline js::Local angleDifference(js::Local& args, bool) {
    auto mod = [](float a, float n) {return a - floor(a / n) * n;};

    auto x = js::to<float>(get(args, V_0));
    auto y = js::to<float>(get(args, V_1));
    auto c = js::to<float>(get(args, V_2));

    if (c == 0)
        c = 3.1415926535897932384626433f;

    auto TAU = c * 2;
    auto a = mod(x - y, TAU);
    auto b = mod(y - x, TAU);

    return {a < b ? -a : b};
}

inline js::Local vectorLength(js::Local& args, bool) {
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<float>(get(args, V_0));

    if (len <= 1)
        return {ret};

    ret *= ret;

    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            auto v = js::to<float>(get(args, js::toString(i)));
            ret += v * v;
        }
    case 4:
    {
        auto v = js::to<float>(get(args, V_3));
        ret += v * v;
    }
    case 3:
    {
        auto v = js::to<float>(get(args, V_2));
        ret += v * v;
    }
    case 2:
    {
        auto v = js::to<float>(get(args, V_1));
        ret += v * v;
    }
    case 1:
    case 0: break;
    }

    return {(float) sqrt(ret)};
}
