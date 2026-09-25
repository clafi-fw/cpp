export module ClaFi.Diagnostic.Benchmark;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.RingBuffer;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Diagnostic
{
    export using Clock = std::chrono::steady_clock;
    export using TimePoint = Clock::time_point;

    // BenchmarkValue

    // A running aggregate of one measured quantity - last, extremes, mean. See Diagnostic
    export class BenchmarkValue
    {
    public:
        void reset();
        void aggregate(float value);
        [[nodiscard]] int timesNum() const { return m_timesNum; }
        [[nodiscard]] float lastValue() const { return m_lastValue; }
        [[nodiscard]] float minValue() const { return m_minValue; }
        [[nodiscard]] float maxValue() const { return m_maxValue; }
        [[nodiscard]] float avgValue() const { return m_avgValue; }
    private:
        int m_timesNum{ 0 };
        float m_lastValue{ k_maxFloat };
        float m_minValue{ k_maxFloat };
        float m_maxValue{ -1.0f };
        float m_avgValue{ 0.0f };
    };

    // CpuUsage

    // The share of one core this process is taking, sampled once a second at most. See Diagnostic
    export class CpuUsage
    {
    public:
        /// @brief Takes the baseline the first sample is measured against. Everything sampled
        /// before is forgotten.
        void start();
        /// @brief Takes a sample if the interval has passed, and answers whether it did.
        bool tick(TimePoint now);
        [[nodiscard]] const BenchmarkValue& cpu() const { return m_cpu; }
        /// @brief The mean of the last k_recentSamples samples - the last ten seconds of the
        /// share, against the aggregate's whole run. None until the first sample.
        [[nodiscard]] std::optional<float> recentAverage() const;
    private:
        using Samples = RingBuffer<float>;
    private:
        static constexpr std::size_t k_recentSamples{ 10 };
        // A sample taken over a shorter span than this says little: a platform accounts a
        // process's CPU time in steps of its own, and a span near the width of one of those steps
        // reads as a whole core or as none.
        static constexpr std::chrono::nanoseconds k_minSampleInterval{ std::chrono::seconds{ 1 } };
    private:
        std::chrono::nanoseconds m_prevCpuTime{ 0 };
        TimePoint m_prevSampleTime{};
        BenchmarkValue m_cpu{};
        Samples m_recent{ k_recentSamples };
    };

    // FrameExtent

    // How much of what is measured a frame covered. See Diagnostic
    export enum class FrameExtent
    {
        Partial,
        Whole
    };

    // FpsBucket

    // The frames measured inside one slice of the history, held as their totals. See Diagnostic
    export struct FpsBucket
    {
        explicit FpsBucket(TimePoint start);

        /// @brief What one whole frame in this slice cost to DRAW. Zero while it holds none.
        [[nodiscard]] float avgDurationInMilliSeconds() const;
        /// @brief How long one frame in this slice took END TO END - the drawing and everything
        /// the frame then waited on.
        [[nodiscard]] float avgFrameIntervalMs() const;

        TimePoint startedAt{};
        // Of the whole frames alone - see FrameExtent.
        float totalPaintTimeMs{ 0.0f };
        float totalFrameIntervalMs{ 0.0f };
        int frameCount{ 0 };
        int wholeFrameCount{ 0 };
    };

    // FpsBuckets

    export using FpsBuckets = RingBuffer<FpsBucket>;

    // FpsBenchmark

    // Times a piece of work, keeping a running aggregate and the last two seconds. See Diagnostic
    export class FpsBenchmark
    {
    public:
        /// @brief How far back the slices reach: what a chart of them spans, and what a reading
        /// "over the history" is taken over.
        static constexpr std::chrono::milliseconds k_historySpan{ 2000 };
        struct Recent
        {
            int frames{ 0 };
            int wholeFrames{ 0 };
            float wholePaintMs{ 0.0f };
        };
    public:
        FpsBenchmark();
    public:
        /// @brief The size of the scene being measured. A new size resets the aggregates: a frame
        /// time taken at one size says nothing about another.
        void setDimensions(ScaledDimensions value);
        [[nodiscard]] ScaledDimensions dimensions() const { return m_dimensions; }
        /// @brief What the measured work cost. THIS IS NOT A FRAME RATE - see frameInterval.
        [[nodiscard]] const BenchmarkValue& paintTime() const { return m_paintTime; }
        /// @brief From one frame's start to the next one's, which is the rate anything is
        /// actually seen at.
        ///
        /// @note The difference between this and paintTime is everything a frame waits on that
        /// the measured work is not: the rest of the form's paint, the copy to the screen, and
        /// whatever pace the display server keeps. A drawing time far under the interval says the
        /// drawing is not what the rate is limited by.
        [[nodiscard]] const BenchmarkValue& frameInterval() const { return m_frameInterval; }
        [[nodiscard]] const BenchmarkValue& cpuUsage() const { return m_cpuUsage.cpu(); }
        /// @brief The CPU share over its last ten samples - see CpuUsage::recentAverage.
        [[nodiscard]] std::optional<float> recentCpuUsage() const { return m_cpuUsage.recentAverage(); }
        /// @brief Forgets the CPU share measured so far and takes a fresh baseline. Neither
        /// reset nor restart touches it: the share is the process's, not the work's.
        void resetCpuUsage() { m_cpuUsage.start(); }
        [[nodiscard]] const FpsBuckets& buckets() const { return m_buckets; }
        /// @brief Drops the aggregates. The slices and the clock stand - see the definition.
        void reset();
        /// @brief Forgets everything measured: the aggregates, the slices and the clock. For a
        /// change of what is being measured, which a reset is not.
        void restart();
        /// @brief Ends the run of frames. The readings stand; the next frame is a first one and
        /// states no interval, so a gap in which no frame was asked for is not read as one that
        /// took that long.
        void interrupt();
        /// @brief Runs the work and measures how long it took, as a whole frame.
        void run(const std::function<void()>& measuredWork);
        /// @brief Takes a frame timed by somebody else, as run takes the one it timed itself.
        /// The extent says whether its duration is a paint time - see FrameExtent.
        void record(TimePoint startedAt, TimePoint finishedAt, FrameExtent = FrameExtent::Whole);
        /// @brief What the last `span` of the history holds as of `now`: the frames that arrived
        /// in it, the whole ones among them, and what those cost on average - zero with none.
        [[nodiscard]] Recent recent(std::chrono::milliseconds span, TimePoint now) const;
    private:
        // The slice `startedAt` falls in: the newest one while that is younger than a slice, and
        // a new one after it otherwise.
        [[nodiscard]] FpsBucket& bucketFor(TimePoint startedAt);
    private:
        // Slices of 10 milliseconds, as many as the span takes: the same history whatever the
        // frame rate.
        static constexpr std::chrono::milliseconds k_bucketDuration{ 10 };
        static constexpr std::size_t k_bucketCount{ static_cast<std::size_t>(k_historySpan / k_bucketDuration) };
    private:
        FpsBuckets m_buckets{ k_bucketCount };
        ScaledDimensions m_dimensions{ 0.0f, 0.0f };
        BenchmarkValue m_paintTime{};
        BenchmarkValue m_frameInterval{};
        // When the previous frame started, and whether there has been one. An interval is between
        // two frames, so the first one establishes the clock and states nothing.
        TimePoint m_prevRunStart{};
        bool m_hasPrevRun{ false };
        CpuUsage m_cpuUsage{};
    };


//-----------------------------------------------------------------------------


    // BenchmarkValue

    // Every default stands at its declaration, so a reset is a fresh value rather than a second
    // statement of the same numbers.
    void BenchmarkValue::reset()
    {
        *this = BenchmarkValue{};
    }

    void BenchmarkValue::aggregate(const float value)
    {
        m_lastValue = value;
        ++m_timesNum;
        m_avgValue += (m_lastValue - m_avgValue) / m_timesNum;
        m_minValue = std::min(m_minValue, m_lastValue);
        m_maxValue = std::max(m_maxValue, m_lastValue);
    }

    // CpuUsage

    void CpuUsage::start()
    {
        m_prevSampleTime = Clock::now();
        m_prevCpuTime = Platform::processCpuTime();
        m_cpu.reset();
        m_recent.clear();
    }

    bool CpuUsage::tick(TimePoint now)
    {
        const std::chrono::nanoseconds sinceSample = now - m_prevSampleTime;
        if (sinceSample < k_minSampleInterval)
            return false;

        const std::chrono::nanoseconds cpuTime = Platform::processCpuTime();
        const std::chrono::nanoseconds cpuStep = cpuTime - m_prevCpuTime;
        m_prevSampleTime = now;
        m_prevCpuTime = cpuTime;

        const float share = static_cast<float>(cpuStep.count())
            / static_cast<float>(sinceSample.count());
        m_cpu.aggregate(share * 100.0f);
        m_recent.push_back(share * 100.0f);
        return true;
    }

    std::optional<float> CpuUsage::recentAverage() const
    {
        if (m_recent.empty())
            return std::nullopt;

        float total = 0.0f;
        for (const float sample : m_recent)
            total += sample;
        return total / static_cast<float>(m_recent.size());
    }

    // FpsBucket

    FpsBucket::FpsBucket(TimePoint start)
        :
        startedAt{ start }
    {
    }

    float FpsBucket::avgDurationInMilliSeconds() const
    {
        if (wholeFrameCount == 0)
            return 0.0f;

        return totalPaintTimeMs / wholeFrameCount;
    }

    float FpsBucket::avgFrameIntervalMs() const
    {
        if (frameCount == 0)
            return 0.0f;

        return totalFrameIntervalMs / frameCount;
    }

    // FpsBenchmark

    FpsBenchmark::FpsBenchmark()
    {
        m_cpuUsage.start();
    }

    void FpsBenchmark::setDimensions(ScaledDimensions value)
    {
        if (m_dimensions == value)
            return;

        m_dimensions = value;
        reset();
    }

    // The slices stand. They are the last two seconds of the rate, which is what a chart of them
    // shows, and the rate itself did not change because the aggregates were dropped.
    //
    // m_prevRunStart stands too: the clock did not stop, and the next frame's interval is still
    // measured from the last one that ran.
    void FpsBenchmark::reset()
    {
        m_paintTime.reset();
        m_frameInterval.reset();
    }

    void FpsBenchmark::restart()
    {
        reset();
        m_buckets.clear();
        m_hasPrevRun = false;
    }

    void FpsBenchmark::interrupt()
    {
        m_hasPrevRun = false;
    }

    void FpsBenchmark::run(const std::function<void()>& measuredWork)
    {
        const TimePoint startedAt = Clock::now();
        measuredWork();
        record(startedAt, Clock::now(), FrameExtent::Whole);
    }

    void FpsBenchmark::record(TimePoint startedAt, TimePoint finishedAt, FrameExtent extent)
    {
        using namespace std::chrono;

        m_cpuUsage.tick(finishedAt);

        const TimePoint prevRunStart = m_prevRunStart;
        const bool hadPrevRun = m_hasPrevRun;
        m_prevRunStart = startedAt;
        m_hasPrevRun = true;

        // THE FIRST FRAME STATES NOTHING. An interval is between two frames, and taking the
        // drawing time for it would report a rate this has never run at.
        if (!hadPrevRun)
            return;

        const float durationMs =
            static_cast<float>(duration_cast<microseconds>(finishedAt - startedAt).count())
            / 1000.0f;
        // MEASURED START TO START, not end to start. A frame is the whole turn of the loop, and
        // an interval taken from where the drawing finished would leave the drawing out of it.
        const float intervalMs =
            static_cast<float>(duration_cast<microseconds>(startedAt - prevRunStart).count())
            / 1000.0f;

        const bool whole = extent == FrameExtent::Whole;
        if (whole)
            m_paintTime.aggregate(durationMs);
        m_frameInterval.aggregate(intervalMs);

        FpsBucket& bucket = bucketFor(startedAt);
        bucket.totalFrameIntervalMs += intervalMs;
        ++bucket.frameCount;
        if (whole)
        {
            bucket.totalPaintTimeMs += durationMs;
            ++bucket.wholeFrameCount;
        }
    }

    FpsBenchmark::Recent FpsBenchmark::recent(std::chrono::milliseconds span, TimePoint now) const
    {
        Recent result{};
        const TimePoint since = now - span;
        for (const FpsBucket& bucket : m_buckets)
        {
            if (bucket.startedAt < since)
                continue;
            result.frames += bucket.frameCount;
            result.wholeFrames += bucket.wholeFrameCount;
            result.wholePaintMs += bucket.totalPaintTimeMs;
        }
        if (result.wholeFrames > 0)
            result.wholePaintMs /= static_cast<float>(result.wholeFrames);
        return result;
    }

    FpsBucket& FpsBenchmark::bucketFor(TimePoint startedAt)
    {
        using namespace std::chrono;

        if (!m_buckets.empty())
        {
            FpsBucket& lastBucket = m_buckets.back();
            const milliseconds sinceSliceStart =
                duration_cast<milliseconds>(startedAt - lastBucket.startedAt);
            if (sinceSliceStart < k_bucketDuration)
                return lastBucket;
        }

        m_buckets.emplace_back(startedAt);
        return m_buckets.back();
    }
}
