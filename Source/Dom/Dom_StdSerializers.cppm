export module ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

// 1. Export concepts and non-specialization interface items
export namespace ClaFi::Dom
{
    template <typename T>
    concept IsStringLike = std::is_convertible_v<std::decay_t<T>, std::wstring_view> &&
        !std::is_same_v<std::decay_t<T>, std::wstring>;

    template <typename T>
    concept IsSerializableEnum = requires(T t)
    {
        { enumNames(t) };
    };
}

// 2. Define specializations in a non-exported block (they inherit export from the primary template)
namespace ClaFi::Dom
{
    // Specialization for std::wstring must be declared first to serve as a valid base class
    template <>
    struct ScalarSerializer<std::wstring>
    {
        [[nodiscard]] static std::wstring toWString(const std::wstring& v);
        [[nodiscard]] static std::wstring toWString(std::wstring&& v);
        static void fromWString(std::wstring_view str, std::wstring& val);
    };

    // Specialization for string-like types inherits from std::wstring
    template <IsStringLike T>
    struct ScalarSerializer<T> : ScalarSerializer<std::wstring>
    {
        using StorageType = std::wstring;
    };

    // Specialization for bool
    template <>
    struct ScalarSerializer<bool>
    {
        [[nodiscard]] static std::wstring toWString(const bool& v);
        static void fromWString(std::wstring_view str, bool& val);
    };

    // Specialization for int
    template <>
    struct ScalarSerializer<int>
    {
        [[nodiscard]] static std::wstring toWString(const int& v);
        static void fromWString(std::wstring_view str, int& val);
    };

    // Specialization for std::size_t, which is what the framework counts with
    template <>
    struct ScalarSerializer<std::size_t>
    {
        [[nodiscard]] static std::wstring toWString(const std::size_t& v);
        static void fromWString(std::wstring_view str, std::size_t& val);
    };

    // Specialization for float
    template <>
    struct ScalarSerializer<float>
    {
        [[nodiscard]] static std::wstring toWString(const float& v);
        static void fromWString(std::wstring_view str, float& val);
    };

    // Specialization for Enums
    template <IsSerializableEnum T>
    struct ScalarSerializer<T>
    {
        [[nodiscard]] static std::wstring toWString(const T& val)
        {
            std::size_t idx = static_cast<std::size_t>(val);
            if (idx < k_namesArray.size())
            {
                return std::wstring{ k_namesArray[idx] };
            }
            return L"";
        }

        static void fromWString(std::wstring_view str, T& val)
        {
            for (std::size_t i = 0; i < k_namesArray.size(); ++i)
            {
                if (k_namesArray[i] == str)
                {
                    val = static_cast<T>(i);
                    return;
                }
            }
        }

        static constexpr auto k_namesArray = enumNames(T{});

        [[nodiscard]] static constexpr bool verifyCount()
        {
            if constexpr (requires { T::Count; })
            {
                return k_namesArray.size() == static_cast<std::size_t>(T::Count);
            }
            return true;
        }

        static_assert(verifyCount(), "Enum name count mismatch!");
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    std::wstring ScalarSerializer<std::wstring>::toWString(const std::wstring& v)
    {
        return v;
    }

    std::wstring ScalarSerializer<std::wstring>::toWString(std::wstring&& v)
    {
        return std::move(v);
    }

    void ScalarSerializer<std::wstring>::fromWString(std::wstring_view str, std::wstring& val)
    {
        val = std::wstring{ str };
    }

    std::wstring ScalarSerializer<bool>::toWString(const bool& v)
    {
        return v ? L"true" : L"false";
    }

    std::wstring ScalarSerializer<int>::toWString(const int& v)
    {
        return std::to_wstring(v);
    }

    void ScalarSerializer<bool>::fromWString(std::wstring_view str, bool& val)
    {
        if (str == L"true" || str == L"1" || str == L"yes")
        {
            val = true;
        }
        else if (str == L"false" || str == L"0" || str == L"no")
        {
            val = false;
        }
    }

    void ScalarSerializer<int>::fromWString(std::wstring_view str, int& val)
    {
        try
        {
            val = std::stoi(std::wstring{ str });
        }
        catch (...)
        {
        }
    }

    std::wstring ScalarSerializer<std::size_t>::toWString(const std::size_t& v)
    {
        return std::to_wstring(v);
    }

    void ScalarSerializer<std::size_t>::fromWString(std::wstring_view str, std::size_t& val)
    {
        try
        {
            val = static_cast<std::size_t>(std::stoull(std::wstring{ str }));
        }
        catch (...)
        {
        }
    }

    std::wstring ScalarSerializer<float>::toWString(const float& v)
    {
        return std::to_wstring(v);
    }

    void ScalarSerializer<float>::fromWString(std::wstring_view str, float& val)
    {
        try
        {
            val = std::stof(std::wstring{ str });
        }
        catch (...)
        {
        }
    }
}
