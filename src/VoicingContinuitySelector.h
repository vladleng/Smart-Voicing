#pragma once

#include "VoicingStrategy.h"
#include <array>
#include <climits>

namespace smartvoicing::harmony
{
// A small stack-only shortlist, avoiding a second expensive search for UST /
// Quartal / Cluster. The Stage 5 vertical always remains the fallback.
//
// This does NOT reinterpret chord functions, infer tensions or alter V1.
// A candidate may change only within a style-specific static-score margin,
// and may not drop guide tones / explicit colours represented by the baseline.
enum class ContinuityStyle : unsigned char { spread, quartal, ust, cluster };

class StrategyContinuitySelector
{
public:
    StrategyContinuitySelector(const NormalizedChord& chord,
                               const VoiceLeadingState* history,
                               ContinuityStyle style) noexcept
        : chord_(chord), history_(history), style_(style) {}

    void consider(const std::array<int, kVoiceCount>& notes, int verticalCost) noexcept
    {
        if (verticalCost < staticCost_)
        {
            staticCost_ = verticalCost;
            staticNotes_ = notes;
        }
        if (history_ == nullptr || ! history_->hasPrevious()
            || verticalCost > staticCost_ + maximumLoss())
            return;

        const int motion = motionRank(notes);
        if (count_ < kCapacity)
        {
            shortlist_[count_++] = {notes, verticalCost, motion};
            return;
        }

        // Fixed maximum of eight items, no heap, no dynamic allocation.
        // Keep a spread of the best static+movement combinations.
        int worstIndex = 0;
        for (int i = 1; i < kCapacity; ++i)
            if (combined(shortlist_[i]) > combined(shortlist_[worstIndex]))
                worstIndex = i;
        const Item current { notes, verticalCost, motion };
        if (combined(current) < combined(shortlist_[worstIndex]))
            shortlist_[worstIndex] = current;
    }

    bool hasCandidate() const noexcept { return staticCost_ != INT_MAX; }

    std::array<int, kVoiceCount> selected() const noexcept
    {
        auto chosen = staticNotes_;
        if (! hasCandidate() || history_ == nullptr || ! history_->hasPrevious())
            return chosen;

        // Baseline always admissible, preserving legacy static behavior
        // where there is no convincingly better legal alternative.
        int chosenScore = staticCost_ * 2 + motionRank(staticNotes_);
        for (int i = 0; i < count_; ++i)
        {
            const auto& item = shortlist_[i];
            if (item.verticalCost > staticCost_ + maximumLoss()
                || ! preservesStaticChordIdentity(item.notes))
                continue;
            const int candidateScore = combined(item);
            if (candidateScore < chosenScore)
            {
                chosenScore = candidateScore;
                chosen = item.notes;
            }
        }
        return chosen;
    }

private:
    struct Item
    {
        std::array<int, kVoiceCount> notes {};
        int verticalCost = 0;
        int motion = 0;
    };
    static constexpr int kCapacity = 8;

    int maximumLoss() const noexcept
    {
        switch (style_)
        {
            case ContinuityStyle::spread: return 8;
            case ContinuityStyle::quartal: return 6;
            case ContinuityStyle::ust: return 5;
            case ContinuityStyle::cluster: return 6;
        }
        return 6;
    }

    int divisor() const noexcept
    {
        switch (style_)
        {
            case ContinuityStyle::spread: return 10;
            case ContinuityStyle::quartal: return 12;
            case ContinuityStyle::ust: return 14;
            case ContinuityStyle::cluster: return 12;
        }
        return 12;
    }

    int motionRank(const std::array<int, kVoiceCount>& notes) const noexcept
    {
        if (history_ == nullptr || ! history_->hasPrevious())
            return 0;
        VoiceOutput output;
        for (int voice = 0; voice < kVoiceCount; ++voice)
            output.voices[static_cast<std::size_t>(voice)] = {
                true, notes[static_cast<std::size_t>(voice)]
            };
        const auto motion = evaluateVoiceLeading(output, *history_);
        if (! motion.valid)
            return INT_MAX / 8;
        return (motion.cost + divisor() / 2) / divisor();
    }

    static int combined(const Item& item) noexcept
    {
        return item.verticalCost * 2 + item.motion;
    }

    static int pitchClass(int note) noexcept
    {
        return ((note % 12) + 12) % 12;
    }

    bool containsPitchClass(const std::array<int, kVoiceCount>& notes,
                            int relative) const noexcept
    {
        for (const int note : notes)
            if ((pitchClass(note) - chord_.rootPitchClass + 12) % 12 == relative)
                return true;
        return false;
    }

    bool protectsRole(int relative) const noexcept
    {
        if (! chord_.hasTone(relative))
            return false;
        const int degree = chord_.degrees[static_cast<std::size_t>(relative)];
        if (degree == 3 || degree == 7 || degree == 9 || degree == 11 || degree == 13)
            return true;
        // Generic-used pitch classes from ARA need harmonic identity too.
        if ((chord_.quality == ChordQuality::dominant
             || chord_.quality == ChordQuality::major)
            && (relative == 4 || relative == 10 || relative == 11))
            return true;
        if ((chord_.quality == ChordQuality::minor
             || chord_.quality == ChordQuality::halfDiminished)
            && (relative == 3 || relative == 10))
            return true;
        if (relative == 1 && chord_.quality == ChordQuality::dominant)
            return true; // explicit dominant b9, even if ARA has generic degree
        if (relative == 6
            && (chord_.quality == ChordQuality::halfDiminished
                || chord_.hasAlteration(ChordAlteration::flatFifth)
                || chord_.hasAlteration(ChordAlteration::sharpEleventh)))
            return true;
        if (relative == 8
            && (chord_.quality == ChordQuality::augmented
                || chord_.hasAlteration(ChordAlteration::sharpFifth)
                || chord_.hasAlteration(ChordAlteration::flatThirteenth)))
            return true;
        return (relative == 2 && chord_.quality == ChordQuality::suspended2)
            || (relative == 5 && chord_.quality == ChordQuality::suspended4);
    }

    bool preservesStaticChordIdentity(const std::array<int, kVoiceCount>& notes) const noexcept
    {
        for (int relative = 0; relative < 12; ++relative)
        {
            if (protectsRole(relative)
                && containsPitchClass(staticNotes_, relative)
                && ! containsPitchClass(notes, relative))
                return false;
        }
        if (chord_.slashBass
            && pitchClass(notes[3]) != chord_.bassPitchClass)
            return false;

        // Preserve the explicit root in a major-seventh slash inversion where
        // the static reference includes it (avoids ambiguity with minor triad).
        if (chord_.slashBass && chord_.quality == ChordQuality::major
            && chord_.hasExtension(ChordExtension::majorSeventh)
            && containsPitchClass(staticNotes_, 0)
            && ! containsPitchClass(notes, 0))
            return false;

        return true;
    }

    const NormalizedChord& chord_;
    const VoiceLeadingState* history_ = nullptr;
    ContinuityStyle style_;
    int staticCost_ = INT_MAX;
    std::array<int, kVoiceCount> staticNotes_ {};
    std::array<Item, kCapacity> shortlist_ {};
    int count_ = 0;
};
}
