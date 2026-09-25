module;
#include <immintrin.h>
export module ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // THE ONE THING UTILS USED TO CLIMB TO UiTypes FOR. It is declared here instead, so Utils
    // imports nothing but StdLib and UiTypes stands above it rather than beside it - setFlag
    // below is what wanted it, and the enums in UiTypes that spell it as an underlying type
    // reach it from up there.
    export using FlagByte = std::uint8_t;

    export template<typename T> concept ArithmeticType = std::is_arithmetic_v<T>;

    export constexpr float k_maxFloat = std::numeric_limits<float>::max();
    export constexpr int k_maxInt = std::numeric_limits<int>::max();
    export constexpr std::size_t k_maxSize = std::numeric_limits<std::size_t>::max();
    export constexpr float k_2Pi = std::numbers::pi_v<float> * 2.0f;

    export [[noreturn]] void unreachable() { std::terminate(); }
    export [[noreturn]] void unreachable(const std::string& errorMessage) { throw std::logic_error{ errorMessage }; }

    export template <typename T>
    struct ScopedPushPop
    {
        ScopedPushPop(T& target, T newValue)
            :
            m_targetVar{ target },
            m_initialValue{ target }
        {
            m_targetVar = newValue;
        }
        ~ScopedPushPop()
        {
            m_targetVar = m_initialValue;
        }
        ScopedPushPop(const ScopedPushPop&) = delete;
        ScopedPushPop& operator=(const ScopedPushPop&) = delete;
    private:
        T& m_targetVar;
        T m_initialValue;
    };

    // construct() below skips default construction rather than running it, so an element lands in
    // whatever the last user of that allocation left there. Skipping the zeroing IS the point -
    // Color has a default constructor that clears all four channels, and a bitmap's worth of
    // that is what this exists to avoid - so the line is not whether the type initializes
    // itself. It is that an element must be safe to destroy and to overwrite having never been
    // constructed, which is what trivially copyable states: a trivial destructor, and copy and
    // assignment that read no invariant. A string, a vector or a unique_ptr would be destroyed
    // through a garbage pointer.
    //
    // The caller still owes the other half: every element the buffer reports must be written
    // before it is read.
    export template <typename T>
        requires std::is_trivially_copyable_v<T>
    struct NoInitAllocator
    {
        using value_type = T;

        NoInitAllocator() noexcept = default;
        template <typename U> constexpr NoInitAllocator(const NoInitAllocator<U>&) noexcept {}

        T* allocate(std::size_t n)
        {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t) noexcept
        {
            ::operator delete(p);
        }

        // Default construction: do absolutely nothing!
        // This bypasses default value-initialization (zeroing) for trivial types.
        template <typename U>
        void construct(U*) noexcept(std::is_nothrow_default_constructible_v<U>)
        {
            // No-op
        }

        template <typename U, typename... Args>
        void construct(U* p, Args&&... args)
        {
            ::new(static_cast<void*>(p)) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U* p) noexcept(std::is_nothrow_destructible_v<U>)
        {
            p->~U();
        }
    };

    export template <typename T, typename U>
    bool operator==(const NoInitAllocator<T>&, const NoInitAllocator<U>&) noexcept { return true; }

    export using NoAllocFloatVector = std::vector<float, NoInitAllocator<float>>;

    export inline __m256 mm256_abs_ps(__m256 v) {
        // 0x7FFFFFFF mask clears the sign bit of a 32-bit float
        const __m256 mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7fffffff));
        return _mm256_and_ps(v, mask);
    }

    export template <typename T>
        void memzero(T& ref)
    {
        std::memset(&ref, 0, sizeof(T));
    }

    export template <typename T1, typename T2>
        bool inRange(T1 value, T2 low, T2 high)
    {
        return !(value < low || value > high);
    }

    // TODO: move sameFactors out of Utils.
    export bool sameFactors(float alpha, float b);
    bool sameFactors(float alpha, float b)
    {
        constexpr float eps = std::numeric_limits<float>::epsilon();
        return std::fabs(alpha - b) < eps;
    }

    export template<typename T>
        std::wstring toHex(T i, bool skipLeadingZeroes = true)
    {
        std::wstringstream stream;
        if (!skipLeadingZeroes)
            stream << std::setfill(L'0') << std::setw(sizeof(T) * 2);
        stream << std::hex << i;
        std::wstring s = stream.str();
        // let's uppercase a..z
        for (wchar_t& c : s)
            if ('a' <= c && c <= 'z')
                c ^= 0x20;
        return std::move(s);
    }

    export float strToFloatDef(std::wstring_view sv, float def) {
        if (sv.empty()) return def;
        wchar_t buf[64];
        const std::size_t len = std::min(sv.size(), std::size_t{ 63 });
        std::memcpy(buf, sv.data(), len * sizeof(wchar_t));
        buf[len] = L'\0';
        wchar_t* end;
        float val = std::wcstof(buf, &end);
        return (end == buf) ? def : val;
    }

    //export float strToFloatDef(std::wstring_view str, float defaultValue = 0.0f)
    //{
    //  if (str.empty())
    //      return defaultValue;
    //  float result;
    //  if (tryStrToFloat(str, result))
    //      return result;
    //  return defaultValue;
    //}


    export void writeZerosIntoBinaryStream(std::ofstream& outFile, std::size_t count);
    export void writeString(std::wostream& stream, std::wstring_view value);
    export void writeString(std::wostream& stream, wchar_t value);
    export void writeIndent(std::wostream& stream, wchar_t indentChar, std::size_t level);
    export void setFlag(FlagByte& flags, const FlagByte flag, const bool value);
    export bool tryStrToFloat(std::wstring_view str, float& result);
    export std::wstring floatToStr(float val);
    export void trimLeft(std::wstring& s);
    export void trimLeft(std::wstring_view& s);
    export void trimRight(std::wstring& s);
    export void trimRight(std::wstring_view& s);
    export void trim(std::wstring& s);
    export void trim(std::wstring_view& s);

    // The one translation between the framework's text and a byte stream. wchar_t is UTF-16 on
    // Windows and UTF-32 on Linux, and both spellings arrive here as the same UTF-8, so a file
    // written on either reads back the same on the other.
    //
    // Neither refuses malformed input. A lone surrogate on the way out, and a byte sequence
    // that is not UTF-8 on the way in, each become U+FFFD - one per malformed sequence, with
    // the byte that ended it read again rather than swallowed. A theme file damaged on disk
    // opens showing where the damage is, which is what the rest of the loading path already
    // assumes: a file that is not there is a quiet return, not a failure.
    export [[nodiscard]] std::string toUtf8(std::wstring_view text);
    export [[nodiscard]] std::wstring fromUtf8(std::string_view bytes);
}
