export module ClaFi.Core.System.Props;

import ClaFi.StdLib;

namespace ClaFi::Props
{
    export template <typename T, typename... Args>
        inline auto get(T default_val, Args&&... args)
    {
        T result = default_val;
        auto extract = [&](auto&& arg) {
            if constexpr (std::is_same_v<T, std::decay_t<decltype(arg)>>)
                result = arg;
            };
        (extract(std::forward<Args>(args)), ...);
        return result;
    }

    export template <typename T, typename... Args>
        inline auto find(Args&&... args) -> std::optional<T>
    {
        std::optional<T> result;
        auto extract = [&](auto&& arg) {
            if constexpr (std::is_same_v<T, std::decay_t<decltype(arg)>>)
                result.emplace(std::forward<decltype(arg)>(arg));
            };
        (extract(std::forward<Args>(args)), ...);
        return result;
    }

#if 0
    // usage example. A plain property type - an event handler does not go through here, it
    // arrives as OnEvent and EventComponent's constructor connects it.
    Props::ifThereIs<Padding>([&](const auto& prop) {
        // Here use the prop;
        }, std::forward<Args>(args)...);
#endif
    export template <typename T, typename... Args, typename Func>
        inline constexpr void ifThereIs(Func&& func, Args&&... args)
    {
        auto extract = [&](auto&& arg) {
            if constexpr (std::is_same_v<T, std::decay_t<decltype(arg)>>)
                func(arg);
            };
        (extract(std::forward<Args>(args)), ...);
    }

    template <typename T, typename... Args>
        concept contains_type = (std::is_same_v<T, std::decay_t<Args>> || ...);

    template <typename T, typename... Args, typename Func>
    inline void ifMissing(Func&& func, Args&&... args)
    {
        if constexpr (!contains_type<T, Args...>) {
            func();
        }
    }

#if 0

    // That harvest routine yet has to be implemented. I'm still not sure if it's actually needed.
    // To harvest props from an etalon source, reading the props in the constructors should be like this:
    // m_width = props::harvest(Width{100.0f}, this, &Button::getWidth, args...).v;
    template <AnyControl T>
    struct BasedOn {
        const T* source;
        explicit BasedOn(const T& s) : source(&s) {}
    };

    template <typename P, typename E, typename... Args>
    auto harvest(P default_val, const E* self_ptr, auto getter_func, Args&&... args) {
        // 1. Direct hit: Is the property P explicitly in the arguments?
        if (auto p = find<P>(std::forward<Args>(args)...))
            return *p;
        // 2. Prototype hit: Is there a 'BasedOn<E>' wrapper in the arguments?
        // We look for BasedOn<E> specifically.
        if (auto wrapper = find<BasedOn<E>>(std::forward<Args>(args)...))
            if (wrapper->source)
                // "Harvest" the value using the provided member-function pointer
                return P{ (wrapper->source->*getter_func)() };
        // 3. Fallback: Return the default
        return default_val;
    }
#endif


    export template<typename... Args>
        struct PropsRouter
    {
        std::tuple<Args...> data;
        PropsRouter(Args&&... p)
            :
            data(std::forward<Args>(p)...)
        {
        }
    };

}

namespace ClaFi
{
    // Props destined for the host control itself.
    export template<typename... Args>
        struct HostProps : public Props::PropsRouter<Args...>
    {
        using Props::PropsRouter<Args...>::PropsRouter;
    };
    export template<typename... Args>
        HostProps(Args&&...) -> HostProps<Args...>;

    // Props destined for the body the host creates.
    export template<typename... Args>
        struct BodyProps : public Props::PropsRouter<Args...>
    {
        using Props::PropsRouter<Args...>::PropsRouter;
    };
    export template<typename... Args>
        BodyProps(Args&&...) -> BodyProps<Args...>;
}
