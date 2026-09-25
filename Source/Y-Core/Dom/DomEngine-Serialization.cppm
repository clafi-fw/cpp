export module ClaFi.Core.DomEngine :Serialization;

import :Core;
import :Section;
import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    namespace StructSerializerUtils
    {
        template <typename Field>
        consteval bool isDirectMemberField()
        {
            if constexpr (requires { std::declval<Field>().member; })
            {
                if constexpr (std::is_member_object_pointer_v<decltype(std::declval<Field>().member)>)
                {
                    if constexpr (requires { std::declval<Field>().setter; })
                    {
                        return std::is_same_v<decltype(std::declval<Field>().setter), std::nullptr_t>;
                    }
                    return true;
                }
            }
            return false;
        }

        template <typename T, typename Field>
        [[nodiscard]] constexpr decltype(auto) getFieldValue(const T& obj, const Field& field)
        {
            static_assert(requires { field.member; }, "SerializedField must contain a 'member' pointer.");
            return std::invoke(field.member, obj);
        }

        template <typename T, typename Field, typename ValueType>
        constexpr void setFieldValue(T& obj, const Field& field, ValueType&& value)
        {
            if constexpr (requires { field.setter; } && !std::is_same_v<decltype(field.setter), std::nullptr_t>) {
                std::invoke(field.setter, obj, std::forward<ValueType>(value));
            }
            else if constexpr (requires { field.member; }) {
                std::invoke(field.member, obj) = std::forward<ValueType>(value);
            }
        }

        template <typename T>
        constexpr auto getSerializedFields()
        {
            if constexpr (requires { T::k_serializedFields; })
            {
                return T::k_serializedFields;
            }
            else if constexpr (requires { T::serializedFields; })
            {
                return T::serializedFields;
            }
            else if constexpr (requires { serializedFields(T{}); })
            {
                return serializedFields(T{});
            }
            else
            {
                static_assert(sizeof(T) == 0,
                    "Type must define k_serializedFields, serializedFields member, or an ADL serializedFields() function.");
                return std::tuple<>{};
            }
        }

        template <IsCompositeValue T, typename Field>
        void initField(Section& section, const T& defaultValue, const Field& field)
        {
            using MemberType = std::decay_t<decltype(getFieldValue(defaultValue, field))>;

            if constexpr (IsSequenceValue<MemberType>)
            {
                // addValue()/Value<ValueT> only has specializations for
                // scalar and composite ValueT — a sequence-typed field (e.g.
                // a std::vector<T> member) needs addSequence() instead. It
                // expects a single item as its default value, not the whole
                // container. Same "use the first element, or a
                // default-constructed one" pattern Sequence<T>::makeChildNode
                // already uses for the equivalent case.
                using ItemT = std::ranges::range_value_t<dom_storage_t<MemberType>>;
                const auto& container = getFieldValue(defaultValue, field);

                ItemT defaultItemValue{};
                auto it = std::begin(container);
                if (it != std::end(container))
                {
                    defaultItemValue = *it;
                }

                // addSequence() only establishes the item schema (needed so
                // a later .add() knows what type to create) — it does not
                // populate the node. Without this .set(), a field like
                // std::array<Hsl, 3> would create a correctly-typed but
                // permanently empty sequence, saving as e.g. "Palette=[ ]"
                // instead of its actual 3 entries.
                auto& seq = section.addSequence(field.name, defaultItemValue);
                seq.set(container);
            }
            else
            {
                section.addValue(field.name, getFieldValue(defaultValue, field));
            }
        }

        template <typename T, typename Field>
        void readField(const Section& section, T& val, const Field& field)
        {
            using MemberType = std::decay_t<decltype(getFieldValue(std::declval<const T&>(), std::declval<const Field&>()))>;
            using StorageType = dom_storage_t<MemberType>;

            // If field.name isn't present in section, operator/ throws
            // std::runtime_error (after emitting a SchemaErrorEvent) — a missing
            // field is an error, not a default. Deserializing an older document
            // that predates a field this struct expects lands here.
            if constexpr (isDirectMemberField<Field>() &&
                std::is_same_v<MemberType, StorageType> &&
                (IsCompositeValue<MemberType> || IsSequenceValue<MemberType>))
            {
                (section / field.name).getTo(val.*(field.member));
            }
            else
            {
                setFieldValue(val, field, (section / field.name).template get<MemberType>());
            }
        }

        template <typename T, typename... Fields>
        void readTo(const Section& section, T& val, const std::tuple<Fields...>& fields)
        {
            std::apply([&](const auto&... f) { (..., readField(section, val, f)); }, fields);
        }

        template <typename T, typename Field>
        void writeField(Section& node, const T& val, const Field& field)
        {
            (node / field.name).set(getFieldValue(val, field));
        }

        template <typename T, typename... Fields>
        void defineSchema(Section& section, const T& defaultValue, const std::tuple<Fields...>& fields)
        {
            std::apply([&](const auto&... f) { (..., initField(section, defaultValue, f)); }, fields);
        }

        template <typename T, typename... Fields>
        T read(const Section& section, const std::tuple<Fields...>& fields)
        {
            T val = T{};
            readTo(section, val, fields);
            return val;
        }

        template <typename T, typename... Fields>
        void write(Section& node, const T& val, const std::tuple<Fields...>& fields)
        {
            std::apply([&](const auto&... f) { (..., writeField(node, val, f)); }, fields);
        }
    }

    export template <typename ValueT>
        struct CompositeSerializer
    {
        static void defineSchema(Section& section, const ValueT& defaultValue);
        static ValueT read(const Section& section);
        static void readTo(const Section& section, ValueT& out);
        static void write(Section& node, const ValueT& value);
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Dom
{
    // --- CompositeSerializer ---

    template <typename ValueT>
    void CompositeSerializer<ValueT>::defineSchema(Section& section, const ValueT& defaultValue)
    {
        StructSerializerUtils::defineSchema(section, defaultValue, StructSerializerUtils::getSerializedFields<ValueT>());
    }

    template <typename ValueT>
    ValueT CompositeSerializer<ValueT>::read(const Section& section)
    {
        return StructSerializerUtils::read<ValueT>(section, StructSerializerUtils::getSerializedFields<ValueT>());
    }

    template <typename ValueT>
    void CompositeSerializer<ValueT>::readTo(const Section& section, ValueT& out)
    {
        StructSerializerUtils::readTo(section, out, StructSerializerUtils::getSerializedFields<ValueT>());
    }

    template <typename ValueT>
    void CompositeSerializer<ValueT>::write(Section& node, const ValueT& value)
    {
        StructSerializerUtils::write(node, value, StructSerializerUtils::getSerializedFields<ValueT>());
    }
}
