#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <math.h>
#include <stdio.h>
#include <array>
#include <variant>
#include <algorithm>

#undef PI
#undef HALF_PI
#undef TWO_PI

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

#ifndef FLOAT
#define FLOAT float
#endif

#ifndef ENABLE_PROFILER
#define ENABLE_PROFILER 0
#endif

#if ENABLE_PROFILER
#define PROFILER js::Profiler _profiler(__func__);
#define PROFILER_NAMED(name) js::Profiler _profiler(name);
#else
#define PROFILER
#define PROFILER_NAMED(name)
#endif

#ifndef OPT_FAST
#define OPT_FAST __attribute__((optimize("-O3")))
#endif

namespace js {
    using Float = FLOAT;

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

    inline void gc();

    inline void* aligned_malloc(size_t size) {
        PROFILER;
        if (size & 3) {
            size += 4 - (size & 3);
        }
        auto ptr = new uint32_t[size >> 2];
        if (!ptr) {
            gc();
            ptr = new uint32_t[size >> 2];
        }
        return ptr;
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


    constexpr inline uint32_t hash(const char* ptr) {
        uint32_t v1 = 5381, v2 = 2166136261, v3 = 0;

        auto cursor = ptr;
        while(uint32_t c = *cursor++){
            if ((c - '0') > '9')
                break;
            auto copy = v3;
            v3 = v3 * 10 + (c - '0');
            if (copy > v3) {
                cursor--;
                break;
            }
        }

        if (!cursor[-1])
            return v3;

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

    OPT_FAST inline constexpr uint32_t nextPOT(uint32_t o) {
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

        OPT_FAST bool operator == (const BufferRef& other) const {
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

        OPT_FAST void hold() {
            PROFILER;
            if (buffer && buffer->refCount) {
                buffer->refCount++;
            }
        }

        OPT_FAST void release() {
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
        Float           // 8
        >;

    // class Tagged {
    // public:
    //     enum class Type {
    //         UNDEFINED,
    //         OBJECTREF,
    //         FUNCTION,
    //         RESOURCE,
    //         BUFFERREF,
    //         INT32,
    //         UINT32,
    //         BOOL,
    //         FLOAT
    //     };

    // private:
    //     Type currentType = Type::UNDEFINED;

    //     union {
    //         Object* object;
    //         RawFunction function;
    //         ResourceRef* resource;
    //         BufferRef buffer;
    //         uint32_t integer;
    //         Float number;
    //     } data;

    // public:
    //     Tagged() = default;
    //     Tagged(const Tagged& other) : currentType{other.currentType}, data{other.data} {}
    // };

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
        static int t = Tagged{T{}}.index();
        return v.index() == t;
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

        OPT_FAST Object(uint32_t bucketCount) : _bucketCount{bucketCount} {
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

        OPT_FAST ~Object() {
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

        OPT_FAST void clear() {
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
            // if (int16_t(rootRefCount) < 0)
            //     debug();
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

        OPT_FAST static constexpr uint32_t getTotalSize(uint32_t bucketCount) {
            uint32_t size = sizeof(Object);
            size += (bucketCount) * sizeof(Bucket);
            auto pot = nextPOT(size);
            return pot;
        }

        OPT_FAST static constexpr uint32_t getObjectList(uint32_t bucketCount) {
            auto size = getTotalSize(bucketCount);
            auto index = uint32_t(ctz(size) - ctz(minObjectSize));
            auto limit = sizeof(objectList)/sizeof(objectList[0]) - 1;
            return index > limit ? limit : index;
        }

        OPT_FAST static iterator erase(iterator it) {
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

    template<typename T>
    bool has(const Local& v) {
        // PROFILER;
        return v.data.index() == Tagged{T{}}.index();
    }

    template<typename L, typename R>
    bool has(const R& t) {
        return std::is_same_v<L, R>;
    }

    OPT_FAST inline Tagged* set(Tagged& container, const BufferRef& key, const Tagged& val) {
        // PROFILER;
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

    OPT_FAST inline Tagged* set(Tagged& container, const Tagged& key, const Tagged& val) {
        return set(container, toString(key), val);
    }

    OPT_FAST inline Tagged* getTaggedPtr(Object* objptr, const BufferRef& key, bool recursive = false, bool force = false) {
        // PROFILER;
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

        Bucket* candidate = nullptr;
        uint32_t pos = h % bucketCount;
        for (uint32_t i = 0; i < bucketCount; ++i) {
            auto& bucket = buckets[pos++];
            if (pos == bucketCount)
                pos = 0;
            if (!bucket.key) {
                candidate = &bucket;
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
                    if (auto found = getTaggedPtr(*objptr, key, true)) {
                        return found;
                    }
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

        if (force && candidate) {
            candidate->key = key;
            candidate->value = {};
            return &candidate->value;
        }

        return nullptr;
    }

    inline Local strCharCodeAt(Local& args, bool);
    inline Local arrIndexOf(Local& args, bool);

    static inline Tagged undef;

    OPT_FAST inline Tagged& get(const BufferRef& str, const BufferRef& key) {
        switch (key->hash) {
        case hash("length"):
            undef = (int32_t) strlen(reinterpret_cast<const char*>(str.data()));
            break;
        case hash("charCodeAt"):
            undef = strCharCodeAt;
            break;
        default:
            undef = {};
            break;
        }
        return undef;
    }

    OPT_FAST inline Tagged& get(const Tagged& container, const BufferRef& key) {
        // PROFILER;
        auto object = std::get_if<Object*>(&container);
        const BufferRef* str = nullptr;
        if (object && *object) {
            if ((*object)->isString()) {
                auto ptr = getTaggedPtr(*object, V_buffer);
                str = std::get_if<BufferRef>(&*ptr);
            } else {
                if ((*object)->isArray()) {
                    switch (key->hash) {
                    case hash("indexOf"):
                        undef = arrIndexOf;
                        return undef;
                    default:
                        break;
                    }
                }
                if (auto ptr = getTaggedPtr(*object, key, true)) {
                    return {*ptr};
                }
            }
        } else {
            str = std::get_if<BufferRef>(&container);
        }
        if (str && *str) {
            return get(*str, key);
        } else {
            undef = {};
        }
        return undef;
    }

    OPT_FAST inline Tagged& get(const Tagged& container, uint32_t key) {
        static Tagged undef;
        undef = {};

        auto object = std::get_if<Object*>(&container);
        if (!object || !*object)
            return undef;

        auto objptr = *object;
        auto buckets = objptr->buckets();
        auto bucketCount = objptr->bucketCount();

        if (!bucketCount)
            return undef;

        uint32_t pos = key;

        if (pos >= bucketCount)
            pos %= bucketCount;

        for (uint32_t i = 0; i < bucketCount; ++i) {
            auto& bucket = buckets[pos++];
            if (pos == bucketCount)
                pos = 0;
            if (!bucket.key) {
                break;
            }
            if (bucket.key->hash == key) {
                // PRINT("Found ");
                // PRINT(key.data());
                // PRINTLN();
                return bucket.value;
            }
        }

        return undef;
    }

    OPT_FAST inline Tagged& get(const Tagged& container, const Tagged& key) {
        Tagged* ret = nullptr;
        Overload {
            [&](Undefined){ret = &(get(container, V_undefined));},
            [&](Object*){ret = &(get(container, V_91Object93));},
            [&](ResourceRef*){ret = &(get(container, V_91Resource93));},
            [&](const RawFunction&){ret = &(get(container, V_91Function93));},
            [&](const BufferRef& ref){ret = &(get(container, ref));},
            [&](const int32_t& a){ret = &(a > 0 ? get(container, a) : get(container, toString(a)));},
            [&](const uint32_t& a){ret = &(get(container, a));},
            [&](const bool& a){ret = &(get(container, a ? V_true : V_false));},
            [&](const Float& v){ret = &(Float(int32_t(v)) == v && v > 0 ? get(container, int32_t(v)) : get(container, toString(v)));}
        }(key);
        return *ret;
    }

    OPT_FAST inline void gc() {
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

    OPT_FAST inline Local alloc(uint32_t propCount, const Tagged& proto = {}) {
        PROFILER;
        auto hasProto = proto.index() != Tagged{}.index();
        if (hasProto)
            propCount++;

        auto list = Object::getObjectList(propCount);

        Local ret;

        for (auto it = Object::iterator{objectList[list]}; it; ++it) {
            if (it->canRecycle()) {
                ret = &*it;
                it->clear();
                recycleCount++;
                goto found;
            }
        }

        {
            auto size = Object::getTotalSize(propCount);
            auto adjustedCount = (size - sizeof(Object)) / sizeof(Bucket);
            heapSize += size;
            if (heapSize > maxHeapSize)
                gc();
            auto raw = aligned_malloc(size);
            ret = new (raw) Object(adjustedCount);
        }

        found:;
        if (hasProto) {
            set(ret, V___proto__, proto);
        }
        return ret;
    }

    OPT_FAST inline BufferRef allocBuffer(uint32_t size) {
        PROFILER;
        static int forceGC = 20;
        if (!--forceGC) {
            forceGC = 20;
            gc();
        }

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

    OPT_FAST inline BufferRef intToBufferRef(const int32_t& v) {
        auto val = v;
        switch (val) {
        case 0: return V_0;
        case 1: return V_1;
        case 2: return V_2;
        case 3: return V_3;
        case 4: return V_4;
        case 5: return V_5;
        case 6: return V_6;
        case 7: return V_7;
        case 8: return V_8;
        case 9: return V_9;
        case 10: return V_10;
        case 11: return V_11;
        case 12: return V_12;
        case 13: return V_13;
        case 14: return V_14;
        case 15: return V_15;
        case 16: return V_16;
        case 17: return V_17;
        case 18: return V_18;
        case 19: return V_19;
        case 20: return V_20;
        case 21: return V_21;
        case 22: return V_22;
        case 23: return V_23;
        case 24: return V_24;
        case 25: return V_25;
        case 26: return V_26;
        case 27: return V_27;
        case 28: return V_28;
        case 29: return V_29;
        case 30: return V_30;
        case 31: return V_31;
        case 32: return V_32;
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
                if (int32_t(v) > 0) {
                    return intToBufferRef(v);
                }
                auto val = v;
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
                return intToBufferRef(v);
            },

            [] (const bool& v) -> BufferRef {
                auto val = v;
                return val ? V_true : V_false;
            },

            [] (const Float& v) -> BufferRef {
                if (Float(int32_t(v)) == v) {
                    return intToBufferRef(int32_t(v));
                }
                auto val = v;
                uint32_t p = 16;
                int ival = int32_t(val * 1000);
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
        static uint32_t prevlen = ~uint32_t{};
        static Local prev{};
        if (auto obj = prev.object(); obj && obj->rootRefCount == 1 && prevlen == len) {
            // if (prevlen > len)
            //     set(prev, V_length, len);
            return prev;
        } else {
            auto args = alloc(len + 2);
            prev = args;
            prevlen = len;
            if (!prev.object()) {
                PRINT("NULL ALLOC");
                PRINTLN();
            }
            args.object()->setFlagArray();
            set(args, V_length, len);
            return args;
        }
    }

    // inline Local string(const BufferRef& ref) {
    //     PROFILER;
    //     // static Local strProto = ([]{
    //     //         auto ret = alloc(1);
    //     //         return ret;
    //     //     })();
    //     auto str = alloc(2);
    //     str.object()->setFlagString();
    //     set(str, V_buffer, ref);
    //     auto data = reinterpret_cast<const char*>(ref.data());
    //     set(str, V_length, (int) (data ? strlen(data) : 0));
    //     return str;
    // }

    // inline Local string(uint32_t index) {
    //     return string(BufferRef{stringTable[index]});
    // }

    template <typename Type>
    Type to(const Tagged& l) { static_assert(std::holds_alternative<Type>(l)); return {};}

    template <typename Type>
    Type to(const Local& l) { return to<Type>(l.data); }

    template <typename Type>
    Type to(const Type& t) { return t; }

    template <>
    inline Object* to<Object*>(const Tagged& l) {
        auto ptr = std::get_if<Object*>(&l);
        return ptr ? *ptr : nullptr;
    }

    template <>
    inline Float to<Float>(const Tagged& l) {
        return Overload {
            [](Undefined) -> Float {return 0.0f;},
            [](Object*) -> Float {return 0.0f;},
            [](ResourceRef*) -> Float {return 0.0f;},
            [](const RawFunction&) -> Float {return 0.0f;},
            [](const BufferRef&) -> Float {return 0.0f;},
            [](const int32_t& a) -> Float {return a;},
            [](const uint32_t& a) -> Float {return a;},
            [](const bool& a) -> Float {return a ? 1.0f : 0.0f;},
            [](const Float& v) -> Float {return v;}
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
            [](const Float& v) -> int32_t {return int32_t(v);}
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
            [](const Float& v) -> uint32_t {return int32_t(v);}
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
            [](const Float& v){return v != 0.0f;}
        }(l);
    }

    template <>
    inline BufferRef to<BufferRef>(const Tagged& l) {
        return toString(l);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_mul(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) * to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_mod(OUT& out, const LEFT& left, const RIGHT& right) {
        if constexpr (std::is_integral_v<OUT>) {
            out = to<OUT>(left) % to<OUT>(right);
        } else {
            auto a = to<Float>(left);
            auto n = to<Float>(right);
            out = a - floor((float) (a / n)) * n;
        }
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

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_div(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) / to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_sub(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) - to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_or(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) | to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_and(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) & to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_xor(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) ^ to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_shl(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) << to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_shr(OUT& out, const LEFT& left, const RIGHT& right) {
        out = to<OUT>(left) >> to<OUT>(right);
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_sru(OUT& out, const LEFT& left, const RIGHT& right) {
        out = uint32_t(to<OUT>(left)) >> uint32_t(to<OUT>(right));
    }

    template<typename OUT>
    inline void op_leq(OUT& out, const Tagged& left, const Tagged& right) {
        if (has<Float>(left) || has<Float>(right)) {
            out = to<Float>(left) <= to<Float>(right);
        } else {
            out = to<int32_t>(left) <= to<int32_t>(right);
        }
    }

    template<typename OUT>
    inline void op_lt(OUT& out, const Tagged& left, const Tagged& right) {
        if (has<Float>(left) || has<Float>(right)) {
            out = to<Float>(left) < to<Float>(right);
        } else {
            out = to<int32_t>(left) < to<int32_t>(right);
        }
    }

    template<typename OUT>
    inline void op_geq(OUT& out, const Tagged& left, const Tagged& right) {
        if (has<Float>(left) || has<Float>(right)) {
            out = to<Float>(left) >= to<Float>(right);
        } else {
            out = to<int32_t>(left) >= to<int32_t>(right);
        }
    }

    template<typename OUT>
    inline void op_gt(OUT& out, const Tagged& left, const Tagged& right) {
        if (has<Float>(left) || has<Float>(right)) {
            out = to<Float>(left) > to<Float>(right);
        } else {
            out = to<int32_t>(left) > to<int32_t>(right);
        }
    }

    template<typename OUT>
    inline void op_seq(OUT& out, const Tagged& left, const Tagged& right) {
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
            [&](const Float& left){return left == std::get<Float>(right);}
        }(left);
    }

    template<typename OUT>
    inline void op_sneq(OUT& out, const Tagged& left, const Tagged& right) {
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
            [&](const Float& left){return left != std::get<Float>(right);}
        }(left);
    }

    template<typename OUT>
    inline void op_eq(OUT& out, const Tagged& left, const Tagged& right) {
        if (left.index() == right.index()) {
            op_seq(out, left, right);
            return;
        }

        if (has<Undefined>(left)) {
            auto obj = std::get_if<Object*>(&right);
            out = (obj && *obj);
            return;
        }
        if (has<Undefined>(right)) {
            auto obj = std::get_if<Object*>(&left);
            out = (obj && *obj);
            return;
        }

        if (has<Object*>(left) ||
            has<Object*>(right) ||
            has<BufferRef>(left) ||
            has<BufferRef>(right)) {
            auto l = to<BufferRef>(left);
            auto r = to<BufferRef>(right);
            out = l == r;
        } else if (has<Float>(left) ||
                   has<Float>(right)) {
            out = to<Float>(left) == to<Float>(right);
        } else {
            out = to<int32_t>(left) == to<int32_t>(right);
        }
    }

    template<typename OUT>
    inline void op_neq(OUT& out, const Tagged& left, const Tagged& right) {
        if (left.index() == right.index()) {
            op_sneq(out, left, right);
            return;
        }

        if (has<Undefined>(left)) {
            auto obj = std::get_if<Object*>(&right);
            out = !(obj && *obj);
            return;
        }
        if (has<Undefined>(right)) {
            auto obj = std::get_if<Object*>(&left);
            out = !(obj && *obj);
            return;
        }

        if (has<Object*>(left) ||
            has<Object*>(right) ||
            has<BufferRef>(left) ||
            has<BufferRef>(right)) {
            auto l = to<BufferRef>(left);
            auto r = to<BufferRef>(right);
            out = l != r;
        } else if (has<Float>(left) ||
                   has<Float>(right)) {
            out = to<Float>(left) != to<Float>(right);
        } else {
            out = to<int32_t>(left) != to<int32_t>(right);
        }
    }

    template<typename OUT, typename LEFT, typename RIGHT>
    inline void op_add(OUT& out, const LEFT& left, const RIGHT& right) {
        if constexpr (std::is_same_v<OUT, Local> || std::is_same_v<OUT, BufferRef>) {
            if (has<Object*>(left) ||
                has<Object*>(right) ||
                has<BufferRef>(left) ||
                has<BufferRef>(right)) {
                auto l = to<BufferRef>(left);
                auto r = to<BufferRef>(right);
                auto o = allocBuffer(l->size + r->size - 1);
                strcpy((char*)o.data(), (const char*)l.data());
                strcat((char*)o.data(), (const char*)r.data());
                out = o; // string(o);
            } else if constexpr (!std::is_same_v<OUT, BufferRef>) {
                if (has<Float>(left) ||
                    has<Float>(right)) {
                    out = to<Float>(left) + to<Float>(right);
                } else {
                    out = to<int32_t>(left) + to<int32_t>(right);
                }
            }
        } else {
            out = to<OUT>(left) + to<OUT>(right);
        }
    }

    template<typename OUT, typename SRC>
    inline void op_inc(OUT& out, SRC& src) {
        out = to<OUT>(src);
        src = out + 1;
    }

    template<typename OUT, typename SRC>
    inline void op_preinc(OUT& out, SRC& src) {
        out = to<OUT>(src) + 1;
        src = out;
    }

    template<typename OUT, typename SRC>
    inline void op_dec(OUT& out, SRC& src) {
        out = to<OUT>(src);
        src = out - 1;
    }

    template<typename OUT, typename SRC>
    inline void op_predec(OUT& out, SRC& src) {
        out = to<OUT>(src) - 1;
        src = out;
    }

    template<typename OUT, typename IN>
    inline void op_neg(OUT& out, const IN& src) {
        out = -to<OUT>(src);
    }

    template<typename OUT, typename IN>
    inline void op_bitnot(OUT& out, const IN& src) {
        out = ~to<OUT>(src);
    }

    template<typename OUT, typename IN>
    inline void op_pos(OUT& out, const IN& src) {
        out = to<OUT>(src);
    }

    template<typename OUT, typename IN>
    inline void op_not(OUT& out, const IN& src) {
        out = !to<bool>(src);
    }

    inline Local arrIndexOf(Local& args, bool) {
        PROFILER;
        auto that = get(args, V_this);
        auto obj = std::get_if<Object*>(&that);
        if (obj && *obj) {
            auto objptr = *obj;
            auto needle = get(args, 0);
            auto buckets = objptr->buckets();
            auto bucketCount = objptr->bucketCount();

            if (!bucketCount)
                return {-1};

            Local eq;
            for (uint32_t i = 0; i < bucketCount; ++i) {
                auto& bucket = buckets[i];
                if (bucket.key) {
                    op_seq(eq, bucket.value, needle);
                    if (std::get<bool>(eq.data) == true) {
                        return {int32_t(bucket.key->hash)};
                    }
                }
            }
        }
        return {-1};
    }

    inline Local strCharCodeAt(Local& args, bool) {
        PROFILER;
        auto that = get(args, V_this);
        auto str = std::get_if<BufferRef>(&that);

        if (!str) {
            if (auto object = std::get_if<Object*>(&that); object && *object) {
                auto ptr = getTaggedPtr(*object, V_buffer);
                str = std::get_if<BufferRef>(&*ptr);
            }
        }

        if (str && *str) {
            if (auto index = to<uint32_t>(get(args, 0)); index < uint32_t((*str)->size - 1)) {
                return {int32_t((*str).data()[index])};
            }
        }

        return {};
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
    PROFILER;
    return js::arguments(js::to<uint32_t>(get(args, V_0)));
}

inline js::Local rand(js::Local& args, bool) {
    PROFILER;
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = int32_t(rand());
    switch (len) {
    case 0:
        return {ret / js::Float(RAND_MAX)};

    case 1:
        return {ret % js::to<int32_t>(get(args, V_0))};

    case 2: {
        auto a = js::to<js::Float>(get(args, V_0));
        auto b = js::to<js::Float>(get(args, V_1));
        js::Float min, range;
        if (a < b) {
            min = a;
            range = b - a;
        } else if (a > b) {
            min = b;
            range = a - b;
        } else {
            return {a};
        }
        return {min + ret / js::Float(RAND_MAX) * range};
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
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    if (auto sval = std::get_if<int32_t>(&arg0)) {
        if (*sval < 0)
            return {-*sval};
        return {*sval};
    }else{
        return {-js::to<js::Float>(arg0)};
    }
}

inline js::Local floor(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {js::to<int32_t>(arg0)};
}

inline js::Local round(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(int32_t)(float(js::to<js::Float>(arg0) + 0.5f))};
}

inline js::Local ceil(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(int32_t) ceilf(float(js::to<js::Float>(arg0)))};
}

inline js::Local cos(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(js::Float) cosf(float(js::to<js::Float>(arg0)))};
}

inline js::Local sin(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(js::Float) sinf(float(js::to<js::Float>(arg0)))};
}

inline js::Local atan2(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    auto& arg1 = js::get(args, V_1);
    return {(js::Float) atan2f(float(js::to<js::Float>(arg0)), float(js::to<js::Float>(arg1)))};
}

inline js::Local tan(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(js::Float) tanf(float(js::to<js::Float>(arg0)))};
}

inline js::Local sqrt(js::Local& args, bool) {
    PROFILER;
    auto& arg0 = js::get(args, V_0);
    return {(js::Float) sqrtf(float(js::to<js::Float>(arg0)))};
}

inline js::Local min(js::Local& args, bool) {
    PROFILER;
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<js::Float>(get(args, V_0));
    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            if (auto v = js::to<js::Float>(get(args, js::toString(i))); v < ret)
                ret = v;
        }
        [[fallthrough]];
    case 4:
        if (auto v = js::to<js::Float>(get(args, V_3)); v < ret)
            ret = v;
        [[fallthrough]];
    case 3:
        if (auto v = js::to<js::Float>(get(args, V_2)); v < ret)
            ret = v;
        [[fallthrough]];
    case 2:
        if (auto v = js::to<js::Float>(get(args, V_1)); v < ret)
            ret = v;
        [[fallthrough]];
    case 1:
    case 0: break;
    }
    return {ret};
}

inline js::Local max(js::Local& args, bool) {
    PROFILER;
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<js::Float>(get(args, V_0));
    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            if (auto v = js::to<js::Float>(get(args, js::toString(i))); v > ret)
                ret = v;
        }
        [[fallthrough]];
    case 4:
        if (auto v = js::to<js::Float>(get(args, V_3)); v > ret)
            ret = v;
        [[fallthrough]];
    case 3:
        if (auto v = js::to<js::Float>(get(args, V_2)); v > ret)
            ret = v;
        [[fallthrough]];
    case 2:
        if (auto v = js::to<js::Float>(get(args, V_1)); v > ret)
            ret = v;
        [[fallthrough]];
    case 1:
    case 0: break;
    }
    return {ret};
}

inline js::Local angleDifference(js::Local& args, bool) {
    PROFILER;
    auto mod = [](js::Float a, js::Float n) {return a - floor((float) (a / n)) * n;};

    auto x = js::to<js::Float>(get(args, V_0));
    auto y = js::to<js::Float>(get(args, V_1));
    auto c = js::to<js::Float>(get(args, V_2));

    if (c == 0)
        c = 3.1415926535897932384626433f;

    auto TAU = c * 2;
    auto a = mod(x - y, TAU);
    auto b = mod(y - x, TAU);

    return {a < b ? -a : b};
}

inline js::Local vectorLength(js::Local& args, bool) {
    PROFILER;
    auto len = js::to<uint32_t>(get(args, V_length));
    auto ret = js::to<js::Float>(get(args, V_0));

    if (len <= 1)
        return {ret};

    ret *= ret;

    switch (len) {
    case 5:
        for (uint32_t i = 4; i < len; ++i) {
            auto v = js::to<js::Float>(get(args, js::toString(i)));
            ret += v * v;
        }
    case 4:
    {
        auto v = js::to<js::Float>(get(args, V_3));
        ret += v * v;
    }
    case 3:
    {
        auto v = js::to<js::Float>(get(args, V_2));
        ret += v * v;
    }
    case 2:
    {
        auto v = js::to<js::Float>(get(args, V_1));
        ret += v * v;
    }
    case 1:
    case 0: break;
    }

    return {(js::Float) sqrt((float)ret)};
}
