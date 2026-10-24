export module ClaFi.Core.System.Csv;

import ClaFi.StdLib;

namespace ClaFi
{
    // Code generated with the assistance of Google AI.

    export class CsvBuilder
    {
    public:
        CsvBuilder() = default;
        void Reset();
        std::wstring str() const { return m_stream.str(); }
        template <typename T>
        CsvBuilder& operator<<(const T& value)
        {
            if (!m_empty)
                m_stream << L",";
            m_stream << value;
            m_empty = false;
            return *this;
        }
    private:
        std::wstringstream m_stream{};
        bool m_empty{ true };
    };

    export class CsvParser
    {
    public:
        template <typename T>
        static std::vector<T> parse(const std::wstring& wstr) {
            std::vector<T> results{};
            if (wstr.empty())
                return results;

            std::size_t comma_count = std::count(wstr.begin(), wstr.end(), L',');
            results.reserve(comma_count + 1);

            std::wstringstream wss(wstr);
            std::wstring segment;

            while (std::getline(wss, segment, L',')) {
                if constexpr (std::is_same_v<T, std::wstring>) {
                    results.push_back(segment);
                }
                else if constexpr (std::is_arithmetic_v<T>) {
                    try {
                        if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
                            results.push_back(static_cast<T>(std::stoull(segment)));
                        }
                        else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
                            results.push_back(static_cast<T>(std::stoll(segment)));
                        }
                        else if constexpr (std::is_floating_point_v<T>) {
                            results.push_back(static_cast<T>(std::stod(segment)));
                        }
                    }
                    catch (const std::exception& e) {
                        std::wcerr << L"ERROR: Invalid data '" << segment << L"'. " << e.what() << std::endl;
                    }
                }
                else {
                    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, std::wstring>,
                        "Unsupported type T provided to CsvParser::Parse. Must be a number or std::wstring.");
                }
            }
            return results;
        }
    };


    //-------------------------------------------------------------------------


    void CsvBuilder::Reset()
    {
        m_stream.str(L"");
        m_stream.clear();
        m_empty = true;
    }
}
