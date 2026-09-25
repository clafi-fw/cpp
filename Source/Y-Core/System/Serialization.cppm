export module ClaFi.Core.System.Serialization;

import ClaFi.StdLib;

// WHY THIS SITS BELOW UiTypes. UiTypes.Rect states its own fields with SerializedField, and
// UiTypes is what nearly everything else is built on - so the descriptor has to be reachable
// from under UiTypes rather than from the Dom layer that consumes it. Nothing here may import
// UiTypes, or the two close a loop.

namespace ClaFi
{
    // A structure that maps a key string to a member pointer, preserving the exact type
    export template <typename Class, typename Type, typename Getter = Type Class::*, typename Setter = std::nullptr_t>
        struct SerializedField
    {
        using ClassType = Class;
        using ValueType = Type;
        using GetterType = Getter;
        using SetterType = Setter;

        std::wstring_view name{};
        Getter member{};
        Setter setter{};
    };

    // CTAD Deduction Guide: Direct member object pointer
    template <typename Class, typename Type>
    SerializedField(std::wstring_view, Type Class::*)
        -> SerializedField<Class, Type, Type Class::*, std::nullptr_t>;

    // CTAD Deduction Guide: By-value getter (const) + Setter
    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type(Class::*)() const, Setter)
        -> SerializedField<Class, Type, Type(Class::*)() const, Setter>;

    // CTAD Deduction Guide: By-value getter (non-const) + Setter
    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type(Class::*)(), Setter)
        -> SerializedField<Class, Type, Type(Class::*)(), Setter>;

    // CTAD Deduction Guide: Const-reference getter + Setter
    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, const Type& (Class::*)() const, Setter)
        -> SerializedField<Class, Type, const Type& (Class::*)() const, Setter>;

    // CTAD Deduction Guide: Non-const reference getter + Setter
    template <typename Class, typename Type, typename Setter>
    SerializedField(std::wstring_view, Type& (Class::*)(), Setter)
        -> SerializedField<Class, Type, Type& (Class::*)(), Setter>;
}
