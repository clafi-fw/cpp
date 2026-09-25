export module ClaFi.Core.TextEngine;

import ClaFi.Diagnostic.Options;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    export class TextEngine
    {
    public:
        TextEngine() = default;
        // The anchor is not optional. A layout is built at the origin of a box of the given size,
        // and the anchor is the whole of what says where in the bounds that box was put - so a
        // call that leaves it out draws against a box the text does not occupy. See
        // anchoredOrigin, which is what a control holding a layout of its own measures through.
        DrawTextResult drawText(ControlPaintContext&, const FloatRect&, const Text&,
            TextAnchor, const EditProps* = nullptr, TextRenderMode = TextRenderMode::Static,
            bool wrap = true);
        // Editable belongs to the layout rather than to the caller's convenience: an editable text
        // keeps the empty paragraph a trailing newline opens, because the caret has to be able to
        // stand on it. Measured as not editable, that row is missing from the size the control is
        // given, and the paint then places it past the control's own bottom edge where the clip
        // removes it - along with any caret standing on it.
        CalculatedDimensions calculateText(const FormContext&, const Text&,
            MaxSize = { k_maxFloat, k_maxFloat }, bool editable = false, bool wrap = true);
        // Whether a box of that size cuts this text. calculatedDimensions is clamped to the box
        // it was measured against, so a measurement cannot answer this and the fit is asked for
        // outright - which is what a hint standing in for words a control has cut has to know
        // before it is shown. Keyed the way drawText keys it, so a box a paint has already drawn
        // this text in costs the lookup and no shaping.
        [[nodiscard]] bool isTextTrimmed(const FormContext&, const Text&, MaxSize,
            bool wrap = true);
        // Positions in a text, and no layout between them and the answer. A caret is measured on a
        // TextLayout instead, by whoever holds one: an edited text is a different text on every
        // key, so a caret query routed through the cache below stores a copy of it per press and
        // evicts the static layouts the rest of the form is drawn from.
        std::size_t nextWord(const Text& text, std::size_t pos);
        std::size_t prevWord(const Text& text, std::size_t pos);
        TextRange wordAt(const Text& text, std::size_t pos);
        TextRange paragraphAt(const Text& text, std::size_t pos);

        // Drops every retained layout. For anything that changes what a build would produce without
        // being part of the key - the font table above all.
        void invalidateLayouts();
        // What a layout held outside this cache tests to learn that a build made before it would
        // come out differently now. invalidateLayouts drops the cache and moves this on, and an
        // owner reading a number other than the one it built at invalidates its own layout.
        [[nodiscard]] std::uint64_t layoutGeneration() const { return m_layoutGeneration; }
    private:
        // Which Text, at what scale, in what state. Not the box: the width the lines are broken
        // at picks a layout inside the bucket this names rather than a bucket, and the height is
        // read off finished lines and shapes nothing. Colours are absent for a similar reason, but
        // a colour still makes one Text a different Text and textHash carries it. Floats are held
        // as their bits, because this is an identity question and not a numeric one.
        struct LayoutKey
        {
            std::size_t textHash;
            std::uint32_t scaleFactor;
            bool editable;
            // A text broken to its box and the same text broken to nothing are two shapings, and
            // acceptsWidth answers for a whole width where one of them is concerned.
            bool wrap;
            // Stated only by a text that carries a flex space - see select. Everything else is
            // shaped the same way for a measurement and for a paint.
            EventPhase phase;

            bool operator==(const LayoutKey&) const = default;
        };

        struct LayoutKeyHash
        {
            std::size_t operator()(const LayoutKey&) const;
        };

        struct CachedLayout
        {
            // Owned. Every caller's Text is a temporary built for one paint, while TextLayout
            // keeps a pointer to the one it was given - so the cache holds the copy that pointer
            // refers to, and an entry is held by pointer so that a growing bucket cannot move it.
            Text text;
            TextLayout layout;
            std::uint64_t lastUsed{ 0 };
        };

        using CachedLayoutPtr = std::unique_ptr<CachedLayout>;
        // One layout per width a text has been broken at. Two widths that break the same lines
        // share a build - which is what a measurement and the paint after it ask for - and two
        // that do not each keep their own, rather than one entry re-shaping past itself every
        // frame. See TextLayout::acceptsWidth.
        using LayoutBucket = std::vector<CachedLayoutPtr>;
    private:
        [[nodiscard]] TextLayout& select(const Text&, MaxSize, ScaleFactor, bool editable, bool wrap,
            EventPhase);
        void evict();
        static void highlightTextArea(ControlPaintContext&, const FloatRect& bounds,
            const FloatRect& textArea);
    private:
        static constexpr std::size_t k_maxCachedLayouts = 512;
        // How many widths one text keeps a layout for. A control drawing the same text at two
        // widths that break it differently is ordinary; a dozen is a text being animated through
        // them, and the oldest goes rather than the cache filling with one text.
        static constexpr std::size_t k_maxWidthsPerText = 4;

        std::unordered_map<LayoutKey, LayoutBucket, LayoutKeyHash> m_layouts;
        // Entries across every bucket, which is what k_maxCachedLayouts counts.
        std::size_t m_entryCount{ 0 };
        std::uint64_t m_useCounter{ 0 };
        std::uint64_t m_layoutGeneration{ 0 };
    };


    //-------------------------------------------------------------------------


    std::size_t TextEngine::LayoutKeyHash::operator()(const LayoutKey& key) const
    {
        std::size_t hash = 14695981039346656037ull;
        auto combine = [&hash](std::size_t value)
            {
                hash ^= value;
                hash *= 1099511628211ull;
            };

        combine(key.textHash);
        combine(key.scaleFactor);
        combine(key.editable ? 1u : 0u);
        combine(key.wrap ? 1u : 0u);
        combine(static_cast<std::size_t>(key.phase));
        return hash;
    }

    DrawTextResult TextEngine::drawText(ControlPaintContext& controlContext, const FloatRect& bounds,
        const Text& text, const TextAnchor anchor,
        const EditProps* editProps, TextRenderMode textRenderMode, bool wrap)
    {
        if (text.plainText().empty() && !editProps)
            return {};

        TextLayout& layout = select(text, bounds.dimensions(), controlContext.scaleFactor(),
            editProps != nullptr, wrap, EventPhase::Paint);
        const FloatPoint anchoredPos = anchoredOrigin(bounds, layout.calculatedDimensions(), anchor);

        const DrawTextResult result = layout.draw(controlContext, anchoredPos, editProps,
            textRenderMode);
        if constexpr (Diagnostic::Options::highlightTextAreas)
        {
            highlightTextArea(controlContext, bounds,
                FloatRect::fromDimensions(anchoredPos, layout.calculatedDimensions()));
        }
        return result;
    }

    CalculatedDimensions TextEngine::calculateText(const FormContext& formContext, const Text& text,
        MaxSize maxDimensions, bool editable, bool wrap)
    {
        return select(text, maxDimensions, formContext.scaleFactor(), editable, wrap,
            EventPhase::Calculate).calculatedDimensions();
    }

    bool TextEngine::isTextTrimmed(const FormContext& formContext, const Text& text,
        MaxSize maxDimensions, bool wrap)
    {
        // An empty text is cut by no box, and drawText declines the same way rather than keeping
        // a layout for one.
        if (text.plainText().empty())
            return false;
        // The paint's phase: the question is about the words on screen, and a text carrying flex
        // space is laid out differently in the two phases. A text without one names Calculate
        // whatever is asked for here, so both entry points share the entry.
        return select(text, maxDimensions, formContext.scaleFactor(), false, wrap,
            EventPhase::Paint).isTrimmed();
    }

    void TextEngine::invalidateLayouts()
    {
        m_layouts.clear();
        m_entryCount = 0;
        ++m_layoutGeneration;
    }

    // The one place a layout is found or made. Every entry point below goes through here, so they
    // all state their inputs the same way and none of them can forget one - which the old sequence
    // of setters allowed: calculateText never set editable, and silently inherited whatever the
    // previous caller had left behind.
    TextLayout& TextEngine::select(const Text& text, MaxSize bounds, ScaleFactor scaleFactor,
        bool editable, bool wrap, EventPhase phase)
    {
        LayoutKey key{
            .textHash = text.hash(),
            .scaleFactor = std::bit_cast<std::uint32_t>(static_cast<float>(scaleFactor)),
            .editable = editable,
            .wrap = wrap,
            // A text carrying no flex space shapes the same in both phases, so it names one of
            // them and the measurement and the paint share an entry.
            .phase = text.hasFlexSpace() ? phase : EventPhase::Calculate
        };

        // A key names a text, not a build of it: the bucket holds one layout per width the text
        // has been broken at, and the first that can answer about this one takes it. A hash is a
        // way to find them and not proof of anything, so each is asked what it was built from -
        // two texts landing on one key would otherwise draw the wrong words.
        //
        // ADDRESSED BY WHAT A TEXT SAYS, never by which text it is. That is what lets two controls
        // saying the same thing share one shaping, and what a caller whose answer is composed anew
        // for every paint needs - such a text is a different object each time and would match
        // nothing by identity. A ControlText's stamp is for a reader holding ONE text across time,
        // which is what a control does with its own and what this cache never does.
        auto found = m_layouts.find(key);
        if (found != m_layouts.end())
        {
            for (const CachedLayoutPtr& entry : found->second)
            {
                if (!entry->layout.acceptsWidth(bounds.x) || entry->text != text)
                    continue;

                entry->lastUsed = ++m_useCounter;
                // Neither the height nor a width the lines already fit is in the key, so the
                // entry is told the box this caller asked about. What that moved is dropped and
                // the lines are kept.
                entry->layout.setBoundsAndScale(bounds, scaleFactor);
                return entry->layout;
            }
        }

        // Before the bucket is taken by reference: this erases, and what it erases can be the
        // bucket this key names.
        evict();

        LayoutBucket& bucket = m_layouts[key];
        if (bucket.size() >= k_maxWidthsPerText)
        {
            auto oldest = std::min_element(bucket.begin(), bucket.end(),
                [](const CachedLayoutPtr& first, const CachedLayoutPtr& second){
                    return first->lastUsed < second->lastUsed;
                });
            bucket.erase(oldest);
            --m_entryCount;
        }

        bucket.push_back(std::make_unique<CachedLayout>());
        ++m_entryCount;

        CachedLayout& entry = *bucket.back();
        entry.text = text;
        entry.lastUsed = ++m_useCounter;
        entry.layout.setEventPhase(phase);
        entry.layout.setEditable(editable);
        entry.layout.setWrap(wrap);
        entry.layout.setText(entry.text);
        entry.layout.setBoundsAndScale(bounds, scaleFactor);
        return entry.layout;
    }

    void TextEngine::evict()
    {
        if (m_entryCount < k_maxCachedLayouts)
        {
            return;
        }

        // Swept rather than kept in order, because this runs once per cache full instead of once
        // per lookup, and an ordered list would charge every hit for a case that is rare.
        const std::uint64_t keep = k_maxCachedLayouts / 2;
        const std::uint64_t cutoff = m_useCounter > keep ? m_useCounter - keep : 0;
        for (auto it = m_layouts.begin(); it != m_layouts.end(); )
        {
            LayoutBucket& bucket = it->second;
            m_entryCount -= std::erase_if(bucket, [cutoff](const CachedLayoutPtr& entry){
                return entry->lastUsed < cutoff;
            });

            if (bucket.empty())
                it = m_layouts.erase(it);
            else
                ++it;
        }
    }

    // The diagnostic overlay behind Diagnostic::Options::highlightTextAreas: the box the text took
    // filled, the bounds it was given outlined. Painted after the text so both stay readable.
    void TextEngine::highlightTextArea(ControlPaintContext& controlContext, const FloatRect& bounds,
        const FloatRect& textArea)
    {
        constexpr Color k_textAreaFill{ 255, 0, 160, 56 };
        constexpr Color k_boundsOutline{ 255, 0, 160, 160 };
        Graphics::Canvas& canvas = controlContext.canvas();
        canvas.fillRectangle(textArea, k_textAreaFill);
        canvas.drawRectangle(bounds, k_boundsOutline);
    }

    enum class CharClass { Space, Alnum, Punct };

    static CharClass getCharClass(wchar_t c) {
        if (std::iswspace(c)) return CharClass::Space;
        if (std::iswalnum(c)) return CharClass::Alnum;
        return CharClass::Punct;
    }

    std::size_t TextEngine::nextWord(const Text& text, std::size_t pos)
    {
        const auto& raw = text.plainText();
        std::size_t len = raw.length();
        if (pos >= len) return len;

        CharClass startClass = getCharClass(raw[pos]);

        if (startClass != CharClass::Space) {
            while (pos < len && getCharClass(raw[pos]) == startClass) {
                pos++;
            }
        }

        while (pos < len && getCharClass(raw[pos]) == CharClass::Space) {
            pos++;
        }

        return pos;
    }

    std::size_t TextEngine::prevWord(const Text& text, std::size_t pos)
    {
        const auto& raw = text.plainText();
        if (pos == 0 || raw.empty()) return 0;
        if (pos > raw.length()) pos = raw.length();

        pos--;

        while (pos > 0 && getCharClass(raw[pos]) == CharClass::Space) {
            pos--;
        }

        CharClass targetClass = getCharClass(raw[pos]);
        if (targetClass != CharClass::Space) {
            while (pos > 0 && getCharClass(raw[pos - 1]) == targetClass) {
                pos--;
            }
        }

        return pos;
    }

    TextRange TextEngine::wordAt(const Text& text, std::size_t pos)
    {
        const auto& raw = text.plainText();
        if (raw.empty()) return { 0, 0 };

        if (pos >= raw.length()) pos = raw.length() - 1;

        CharClass targetClass = getCharClass(raw[pos]);

        std::size_t start = pos;
        std::size_t end = pos;

        while (start > 0 && getCharClass(raw[start - 1]) == targetClass) start--;
        while (end < raw.length() && getCharClass(raw[end]) == targetClass) end++;

        return { start, end - start };
    }

    // The paragraph a position stands in, from the start of the text or the newline before it, up
    // to and INCLUDING the newline that ends it. The newline is part of what a paragraph is here:
    // a selection that leaves it behind cuts the text and leaves an empty row where it was, and
    // pastes back as a fragment rather than as a paragraph.
    TextRange TextEngine::paragraphAt(const Text& text, std::size_t pos)
    {
        const std::wstring& raw = text.plainText();
        if (raw.empty())
            return { 0, 0 };
        if (pos > raw.length())
            pos = raw.length();

        // Searched from the character BEFORE the position: a position sits between characters, and
        // the newline the paragraph opens after is the one to the left of it.
        std::size_t start = pos ? raw.rfind(L'\n', pos - 1) : std::wstring::npos;
        start = start == std::wstring::npos ? 0 : start + 1;

        std::size_t end = raw.find(L'\n', pos);
        end = end == std::wstring::npos ? raw.length() : end + 1;

        return { start, end - start };
    }

}
