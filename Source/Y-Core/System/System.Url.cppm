export module ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace ClaFi
{
    export struct UrlParameter
    {
        std::wstring name;
        std::optional<std::wstring> value;

        [[nodiscard]] bool operator==(const UrlParameter&) const = default;
    };

    // A URL reference with decoded path, query parameters and fragment. See UI-Types
    export class Url
    {
    public:
        using QueryParameters = std::vector<UrlParameter>;
    public:
        Url() = default;
        explicit Url(std::wstring_view path, std::wstring_view fragment = {});
    public:
        [[nodiscard]] static std::optional<Url> Parse(std::wstring_view spelling);
        [[nodiscard]] static Url Build(std::wstring_view scheme, std::wstring_view path,
            std::wstring_view fragment = {}, const QueryParameters& query = {},
            std::optional<std::wstring_view> authority = {});
        [[nodiscard]] static std::wstring_view schemeOf(std::wstring_view spelling);
        [[nodiscard]] const std::wstring& scheme() const { return m_scheme; }
        [[nodiscard]] const std::optional<std::wstring>& authority() const { return m_authority; }
        [[nodiscard]] const std::wstring& path() const { return m_path; }
        [[nodiscard]] const QueryParameters& query() const { return m_query; }
        [[nodiscard]] const std::wstring& fragment() const { return m_fragment; }
        [[nodiscard]] const std::wstring& str() const { return m_spelling; }
        [[nodiscard]] bool operator==(const Url&) const = default;
    private:
        std::wstring m_scheme{};
        std::optional<std::wstring> m_authority{};
        std::wstring m_path{};
        QueryParameters m_query{};
        std::wstring m_fragment{};
        std::wstring m_spelling{};
    };
}
