export module ClaFi.Core.DomEngine :Value;

import :Core;
import :Section;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    template <IsScalarValue ValueT>
    class Value<ValueT> : public ScalarValueBase
    {
    public:
        Value(DomNodeBase* parent, const ValueT& defaultValue)
            :
            ScalarValueBase{ parent },
            m_value{ defaultValue }
        {
        }

        [[nodiscard]] ValueT get() const
        {
            return m_value;
        }

        void getTo(ValueT& out) const
        {
            out = m_value;
        }

        void set(const ValueT& value)
        {
            if constexpr (requires { m_value == value; })
            {
                if (m_value == value)
                {
                    return;
                }
            }
            Transaction t = Transaction{ *this };
            m_value = value;
            emitChange();
        }

        void set(ValueT&& value)
        {
            if constexpr (requires { m_value == value; })
            {
                if (m_value == value)
                {
                    return;
                }
            }
            Transaction t = Transaction{ *this };
            m_value = std::move(value);
            emitChange();
        }

        template <typename U>
        void set(U&& value) requires (!std::is_same_v<std::decay_t<U>, ValueT>&& std::is_constructible_v<ValueT, U>)
        {
            set(ValueT{ std::forward<U>(value) });
        }

        void setFromRaw(std::wstring_view raw) override
        {
            ValueT parsed = m_value;
            ScalarSerializer<ValueT>::fromWString(raw, parsed);
            set(std::move(parsed));
        }

        [[nodiscard]] std::wstring getAsRaw() const override
        {
            return ScalarSerializer<ValueT>::toWString(m_value);
        }

        [[nodiscard]] std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const override
        {
            return std::make_unique<Value<ValueT>>(parent, m_value);
        }

    private:
        ValueT m_value;
    };

    template <IsCompositeValue ValueT>
    class Value<ValueT> : public Section
    {
    public:
        Value(DomNodeBase* parent, const ValueT& defaultValue)
            :
            Section{ parent }
        {
            CompositeSerializer<ValueT>::defineSchema(*this, defaultValue);
        }

        [[nodiscard]] DomNodeType type() const noexcept override
        {
            return DomNodeType::CompositeValue;
        }

        [[nodiscard]] ValueT get() const
        {
            ValueT val{};
            CompositeSerializer<ValueT>::readTo(*this, val);
            return val;
        }

        void getTo(ValueT& out) const
        {
            CompositeSerializer<ValueT>::readTo(*this, out);
        }

        void set(const ValueT& value)
        {
            if constexpr (requires { value == value; })
            {
                if (get() == value)
                {
                    return;
                }
            }
            Transaction t = Transaction{ *this };
            CompositeSerializer<ValueT>::write(*this, value);
            emitChange();
        }

        void set(ValueT&& value)
        {
            if constexpr (requires { value == value; })
            {
                if (get() == value)
                {
                    return;
                }
            }
            Transaction t = Transaction{ *this };
            CompositeSerializer<ValueT>::write(*this, std::move(value));
            emitChange();
        }

        template <typename U>
        void set(U&& value) requires (!std::is_same_v<std::decay_t<U>, ValueT>&& std::is_constructible_v<ValueT, U>)
        {
            set(ValueT{ std::forward<U>(value) });
        }

        [[nodiscard]] std::unique_ptr<DomNodeBase> clone(DomNodeBase* parent) const override
        {
            auto copy = std::make_unique<Value<ValueT>>(parent, get());
            return copy;
        }
    };
}
