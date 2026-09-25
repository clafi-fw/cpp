// =========================================================================
// Benchmark: Save/Load performance across ClaFi, Json, and Xml
// =========================================================================
//
// NOT a module itself - a plain translation unit with main(), importing the
// library modules it needs. ClaFi.StdLib re-exports the whole std module, so
// the clock, the stream formatting and the filesystem paths used below all
// arrive with it.

import ClaFi.Dom.Formats.ClaFi;
import ClaFi.Dom.Formats.Json;
import ClaFi.Dom.Formats.Xml;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;

using namespace ClaFi::Dom;

namespace
{
    // -----------------------------------------------------------
    // Schema: deliberately not just "one big flat list" - a nested
    // composite (Bounds) and a small nested sequence (tags) per record,
    // so the benchmark exercises more than raw sequence throughput.
    // -----------------------------------------------------------

    struct Bounds
    {
        int x{};
        int y{};
        int width{};
        int height{};

        static constexpr auto serializedFields = std::make_tuple(
            SerializedField{ L"X", &Bounds::x },
            SerializedField{ L"Y", &Bounds::y },
            SerializedField{ L"Width", &Bounds::width },
            SerializedField{ L"Height", &Bounds::height }
        );

        bool operator==(const Bounds&) const = default;
    };

    struct Record
    {
        std::wstring id;
        std::wstring name;
        float score{};
        bool active{};
        int count{};
        Bounds bounds{};
        std::vector<std::wstring> tags;

        static constexpr auto serializedFields = std::make_tuple(
            SerializedField{ L"Id", &Record::id },
            SerializedField{ L"Name", &Record::name },
            SerializedField{ L"Score", &Record::score },
            SerializedField{ L"Active", &Record::active },
            SerializedField{ L"Count", &Record::count },
            SerializedField{ L"Bounds", &Record::bounds },
            SerializedField{ L"Tags", &Record::tags }
        );

        bool operator==(const Record&) const = default;
    };

    // Tune these to make the run bigger/smaller.
    constexpr std::size_t k_recordCount = 150000;
    constexpr int k_repetitions = 4;

    std::vector<Record> makeRecords(std::size_t count)
    {
        std::vector<Record> records;
        records.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            Record r;
            r.id = L"REC-" + std::to_wstring(i);
            r.name = L"Record Name " + std::to_wstring(i);
            r.score = static_cast<float>(i % 1000) / 10.0f;
            r.active = (i % 2 == 0);
            r.count = static_cast<int>(i);
            r.bounds = Bounds{
                .x = static_cast<int>(i % 100),
                .y = static_cast<int>(i % 200),
                .width = 100 + static_cast<int>(i % 50),
                .height = 50 + static_cast<int>(i % 25)
            };
            r.tags = std::vector<std::wstring>{
                L"tag" + std::to_wstring(i % 7),
                L"group" + std::to_wstring(i % 13)
            };
            records.push_back(std::move(r));
        }

        return records;
    }

    Dt::Section makeLayout()
    {
        return Dt::Section{
            Dt::Value{ L"AppName", std::wstring{ L"BenchmarkApp" } },
            Dt::Value{ L"Version", 1 },
            Dt::Sequence{ L"Records", Record{} }
        };
    }

    // Builds a schema-applied Section and populates it directly via
    // .set(records) - pure in-memory construction, no format, no file I/O,
    // no text parsing anywhere in this call.
    Section buildPopulatedSection(const std::vector<Record>& records)
    {
        Section section{ nullptr };
        makeLayout().apply(section);
        (section / L"Records").set(records);
        return section;
    }

    // Isolates tree-construction/allocation cost from anything
    // format-specific: assign() deep-copies an already-populated Section
    // into a fresh one, entirely in memory. No file is read or written, no
    // text is parsed or produced - whatever this costs is a floor that
    // every format's load() pays in addition to, never instead of.
    void measureAssignTime(const std::vector<Record>& records)
    {
        Section templateSection = buildPopulatedSection(records);

        // Warm-up, same reasoning as runBenchmark's.
        {
            Section fresh{ nullptr };
            makeLayout().apply(fresh);
            fresh.assign(templateSection);
        }

        double totalMs = 0.0;
        for (int rep = 0; rep < k_repetitions; ++rep)
        {
            Section fresh{ nullptr };
            makeLayout().apply(fresh);

            auto t0 = std::chrono::steady_clock::now();
            fresh.assign(templateSection);
            auto t1 = std::chrono::steady_clock::now();

            totalMs += std::chrono::duration<double, std::milli>(t1 - t0).count();
        }

        double avgMs = totalMs / k_repetitions;
        std::wcout << L"Section::assign() from an already-populated in-memory Section\n"
            << L"(pure tree copy - no file I/O, no text parsing at all): "
            << std::fixed << std::setprecision(3) << avgMs << L" ms\n\n";
    }

    struct BenchResult
    {
        std::wstring formatName;
        double schemaMsAvg{};
        double saveMsAvg{};
        double loadMsAvg{};
        std::uintmax_t fileSizeBytes{};
        bool verified{};
    };

    // Spot-checks the loaded data rather than trusting a fast load blindly:
    // size, plus a few sampled records compared field-by-field against
    // what was written.
    bool verify(const Section& loaded, const std::vector<Record>& expected)
    {
        auto loadedRecords = (loaded / L"Records").get<std::vector<Record>>();

        if (loadedRecords.size() != expected.size())
        {
            std::wcout << L"    verify: size mismatch, expected " << expected.size()
                << L" got " << loadedRecords.size() << L"\n";
            return false;
        }

        std::vector<std::size_t> sampleIndices = { 0, expected.size() / 2, expected.size() - 1 };
        for (std::size_t idx : sampleIndices)
        {
            const Record& exp = expected[idx];
            const Record& got = loadedRecords[idx];

            if (exp != got)
            {
                std::wcout << L"    verify: mismatch at record " << idx << L"\n";
                return false;
            }
        }

        return true;
    }

    template <typename Format>
    BenchResult runBenchmark(std::wstring_view formatName, const std::filesystem::path& path, const std::vector<Record>& records)
    {
        BenchResult result;
        result.formatName = std::wstring{ formatName };

        // Untimed warm-up: one full construct/populate/save/load cycle
        // before any timed repetition, so first-call effects (iostream
        // locale initialization on the first stream ever constructed,
        // allocator arena growth, a cold OS page cache for this path)
        // don't skew the first timed repetition's numbers.
        {
            Document<Format> warmupDoc{ path, AutoSave::No, makeLayout() };
            (warmupDoc / L"Records").set(records);
            warmupDoc.save();

            Document<Format> warmupLoaded{ path, AutoSave::No, makeLayout() };
            warmupLoaded.load();
        }

        double totalSchemaMs = 0.0;
        double totalSaveMs = 0.0;
        double totalLoadMs = 0.0;
        int schemaBuildCount = 0;

        std::wcout << L"Measuring " << formatName << L", 4 runs: ";
        for (int rep = 0; rep < k_repetitions; ++rep)
        {
            std::wcout << L" " << rep + 1 << L"...";
            auto tSchema0 = std::chrono::steady_clock::now();
            Document<Format> doc{ path, AutoSave::No, makeLayout() };
            auto tSchema1 = std::chrono::steady_clock::now();
            totalSchemaMs += std::chrono::duration<double, std::milli>(tSchema1 - tSchema0).count();
            ++schemaBuildCount;

            (doc / L"Records").set(records);

            auto t0 = std::chrono::steady_clock::now();
            doc.save();
            auto t1 = std::chrono::steady_clock::now();

            totalSaveMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            auto tSchema2 = std::chrono::steady_clock::now();
            Document<Format> loaded{ path, AutoSave::No, makeLayout() };
            auto tSchema3 = std::chrono::steady_clock::now();
            totalSchemaMs += std::chrono::duration<double, std::milli>(tSchema3 - tSchema2).count();
            ++schemaBuildCount;

            auto t2 = std::chrono::steady_clock::now();
            loaded.load();
            auto t3 = std::chrono::steady_clock::now();

            totalLoadMs += std::chrono::duration<double, std::milli>(t3 - t2).count();

            if (rep == k_repetitions - 1)
            {
                result.verified = verify(loaded, records);
                result.fileSizeBytes = std::filesystem::file_size(path);
            }
        }
        std::wcout << L" done.\n";

        result.schemaMsAvg = totalSchemaMs / schemaBuildCount;
        result.saveMsAvg = totalSaveMs / k_repetitions;
        result.loadMsAvg = totalLoadMs / k_repetitions;
        return result;
    }

    void printResults(const std::vector<BenchResult>& results)
    {
        std::wcout << L"\n";
        std::wcout << std::left << std::setw(10) << L"Format"
            << std::right << std::setw(14) << L"Schema (ms)"
            << std::setw(14) << L"Save (ms)"
            << std::setw(14) << L"Load (ms)"
            << std::setw(16) << L"File Size (KB)"
            << std::setw(12) << L"Verified"
            << L"\n";
        std::wcout << std::wstring(80, L'-') << L"\n";

        for (const auto& r : results)
        {
            std::wcout << std::left << std::setw(10) << r.formatName
                << std::right << std::setw(14) << std::fixed << std::setprecision(3) << r.schemaMsAvg
                << std::setw(14) << std::fixed << std::setprecision(2) << r.saveMsAvg
                << std::setw(14) << std::fixed << std::setprecision(2) << r.loadMsAvg
                << std::setw(16) << std::fixed << std::setprecision(1) << (static_cast<double>(r.fileSizeBytes) / 1024.0)
                << std::setw(12) << (r.verified ? L"OK" : L"FAILED")
                << L"\n";
        }
        std::wcout << L"\n";
    }
}

int main()
{
    std::wcout << L"Generating " << k_recordCount << L" records...\n";
    std::vector<Record> records = makeRecords(k_recordCount);

    measureAssignTime(records);

    std::filesystem::path tempDir = std::filesystem::temp_directory_path();

    std::vector<BenchResult> results;
    results.push_back(runBenchmark<FileFormat::ClaFi>(L"ClaFi", tempDir / L"benchmark.clafi", records));
    results.push_back(runBenchmark<FileFormat::Json>(L"Json", tempDir / L"benchmark.json", records));
    results.push_back(runBenchmark<FileFormat::Xml>(L"Xml", tempDir / L"benchmark.xml", records));

    std::wcout << L"Each figure is an average over " << k_repetitions << L" runs.\n";
    printResults(results);

    return 0;
}
