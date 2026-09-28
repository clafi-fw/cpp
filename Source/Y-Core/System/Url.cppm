export module ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace ClaFi
{
    // A place a link names: the path of a page, and an anchor inside it. See UI-Types
    export class Url
    {
    public:
        Url() = default;
        explicit Url(std::wstring_view path, std::wstring_view anchor = {});
    public:
        [[nodiscard]] static Url parse(std::wstring_view spelling); // reads what str() writes
        [[nodiscard]] const std::wstring& path() const { return m_path; }
        [[nodiscard]] const std::wstring& anchor() const { return m_anchor; } // empty for none
        [[nodiscard]] Url withAnchor(std::wstring_view anchor) const;
        [[nodiscard]] std::wstring str() const; // path#anchor, with a # inside either part escaped
        [[nodiscard]] bool operator==(const Url&) const = default;
    private:
        std::wstring m_path{};
        std::wstring m_anchor{};
    };
}
