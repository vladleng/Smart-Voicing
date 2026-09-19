#pragma once

#include <JuceHeader.h>
#include <cstdint>

#if JUCE_WINDOWS
 #include <windows.h>
#endif

struct SharedHarmonicContextSnapshot
{
    bool connected = false;
    std::uint64_t revision = 0;

    bool hostContentAccessAvailable = false;
    int musicalContextCount = 0;

    bool keySignaturesAvailable = false;
    int keySignatureEventCount = 0;

    bool sheetChordsAvailable = false;
    int sheetChordEventCount = 0;

    bool tempoEntriesAvailable = false;
    int tempoEntryEventCount = 0;

    bool barSignaturesAvailable = false;
    int barSignatureEventCount = 0;
};

// Very small bridge between Smart Voicing ARA.vst3 and Smart Voicing.vst3.
//
// Two different VST3 bundles are two different DLL modules, therefore ordinary
// C++ statics are not shared between them. On Windows 0.0c uses a named memory
// mapping so the ARA/Event FX can publish a tiny immutable-style snapshot and
// the Instrument can read it without polling the host or doing file I/O.
class SharedHarmonicContextBridge final
{
public:
    static SharedHarmonicContextBridge& instance()
    {
        static SharedHarmonicContextBridge bridge;
        return bridge;
    }

    ~SharedHarmonicContextBridge()
    {
#if JUCE_WINDOWS
        if (writer && block != nullptr)
        {
            SharedHarmonicContextSnapshot empty;
            publish(empty);
        }

        if (block != nullptr)
            UnmapViewOfFile(block);

        if (mapping != nullptr)
            CloseHandle(mapping);

        if (writeMutex != nullptr)
            CloseHandle(writeMutex);
#endif
    }

    void publish(const SharedHarmonicContextSnapshot& snapshot)
    {
#if JUCE_WINDOWS
        if (! ensureWriter())
            return;

        if (writeMutex != nullptr)
            WaitForSingleObject(writeMutex, INFINITE);

        // Odd sequence means "writer in progress", even means stable snapshot.
        InterlockedIncrement64(&block->sequence);
        MemoryBarrier();

        block->magic = magicValue;
        block->abiVersion = abiVersion;
        block->connected = snapshot.connected ? 1u : 0u;
        block->hostContentAccessAvailable = snapshot.hostContentAccessAvailable ? 1u : 0u;
        block->musicalContextCount = snapshot.musicalContextCount;
        block->keySignaturesAvailable = snapshot.keySignaturesAvailable ? 1u : 0u;
        block->keySignatureEventCount = snapshot.keySignatureEventCount;
        block->sheetChordsAvailable = snapshot.sheetChordsAvailable ? 1u : 0u;
        block->sheetChordEventCount = snapshot.sheetChordEventCount;
        block->tempoEntriesAvailable = snapshot.tempoEntriesAvailable ? 1u : 0u;
        block->tempoEntryEventCount = snapshot.tempoEntryEventCount;
        block->barSignaturesAvailable = snapshot.barSignaturesAvailable ? 1u : 0u;
        block->barSignatureEventCount = snapshot.barSignatureEventCount;

        MemoryBarrier();
        InterlockedIncrement64(&block->sequence);

        if (writeMutex != nullptr)
            ReleaseMutex(writeMutex);
#else
        localSnapshot = snapshot;
        localSnapshot.revision++;
#endif
    }

    SharedHarmonicContextSnapshot read()
    {
#if JUCE_WINDOWS
        SharedHarmonicContextSnapshot result;
        if (! ensureReader())
            return result;

        for (int attempt = 0; attempt < 8; ++attempt)
        {
            // InterlockedCompareExchange64 is an atomic read implemented as a
            // read-modify-write operation. Therefore the mapped view must be
            // writable even though the Instrument never changes the payload.
            const auto before = static_cast<std::uint64_t>(
                InterlockedCompareExchange64(&block->sequence, 0, 0));

            if ((before & 1u) != 0u)
                continue;

            MemoryBarrier();

            const auto magic = block->magic;
            const auto version = block->abiVersion;
            const auto connected = block->connected;
            const auto hostContentAccessAvailable = block->hostContentAccessAvailable;
            const auto musicalContextCount = block->musicalContextCount;
            const auto keySignaturesAvailable = block->keySignaturesAvailable;
            const auto keySignatureEventCount = block->keySignatureEventCount;
            const auto sheetChordsAvailable = block->sheetChordsAvailable;
            const auto sheetChordEventCount = block->sheetChordEventCount;
            const auto tempoEntriesAvailable = block->tempoEntriesAvailable;
            const auto tempoEntryEventCount = block->tempoEntryEventCount;
            const auto barSignaturesAvailable = block->barSignaturesAvailable;
            const auto barSignatureEventCount = block->barSignatureEventCount;

            MemoryBarrier();

            const auto after = static_cast<std::uint64_t>(
                InterlockedCompareExchange64(&block->sequence, 0, 0));

            if (before != after || (after & 1u) != 0u)
                continue;

            if (magic != magicValue || version != abiVersion)
                return result;

            result.connected = connected != 0;
            result.revision = after / 2u;
            result.hostContentAccessAvailable = hostContentAccessAvailable != 0;
            result.musicalContextCount = musicalContextCount;
            result.keySignaturesAvailable = keySignaturesAvailable != 0;
            result.keySignatureEventCount = keySignatureEventCount;
            result.sheetChordsAvailable = sheetChordsAvailable != 0;
            result.sheetChordEventCount = sheetChordEventCount;
            result.tempoEntriesAvailable = tempoEntriesAvailable != 0;
            result.tempoEntryEventCount = tempoEntryEventCount;
            result.barSignaturesAvailable = barSignaturesAvailable != 0;
            result.barSignatureEventCount = barSignatureEventCount;
            return result;
        }

        return result;
#else
        return localSnapshot;
#endif
    }

private:
    SharedHarmonicContextBridge() = default;

#if JUCE_WINDOWS
    struct SharedBlock
    {
        volatile LONG64 sequence = 0;
        std::uint32_t magic = 0;
        std::uint32_t abiVersion = 0;
        std::uint32_t connected = 0;
        std::uint32_t hostContentAccessAvailable = 0;
        std::int32_t musicalContextCount = 0;
        std::uint32_t keySignaturesAvailable = 0;
        std::int32_t keySignatureEventCount = 0;
        std::uint32_t sheetChordsAvailable = 0;
        std::int32_t sheetChordEventCount = 0;
        std::uint32_t tempoEntriesAvailable = 0;
        std::int32_t tempoEntryEventCount = 0;
        std::uint32_t barSignaturesAvailable = 0;
        std::int32_t barSignatureEventCount = 0;
    };

    static constexpr std::uint32_t magicValue = 0x53564D52; // "SVMR"
    static constexpr std::uint32_t abiVersion = 1;
    static constexpr const wchar_t* mappingName = L"Local\\MoonRiverStudio_SmartVoicing_HarmonicContext_v1";
    static constexpr const wchar_t* mutexName = L"Local\\MoonRiverStudio_SmartVoicing_HarmonicContext_Write_v1";

    bool ensureWriter()
    {
        if (block != nullptr && writer)
            return true;

        if (block != nullptr && ! writer)
        {
            UnmapViewOfFile(block);
            block = nullptr;
        }

        if (mapping != nullptr)
        {
            CloseHandle(mapping);
            mapping = nullptr;
        }

        mapping = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                     nullptr,
                                     PAGE_READWRITE,
                                     0,
                                     static_cast<DWORD>(sizeof(SharedBlock)),
                                     mappingName);
        if (mapping == nullptr)
            return false;

        block = static_cast<SharedBlock*>(
            MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));
        if (block == nullptr)
        {
            CloseHandle(mapping);
            mapping = nullptr;
            return false;
        }

        writeMutex = CreateMutexW(nullptr, FALSE, mutexName);
        writer = true;
        return true;
    }

    bool ensureReader()
    {
        if (block != nullptr)
            return true;

        // Important: InterlockedCompareExchange64 used by read() performs an
        // atomic read-modify-write, so FILE_MAP_READ is not sufficient and may
        // cause an access violation in the host. Map the tiny block writable.
        mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, mappingName);
        if (mapping == nullptr)
            return false;

        block = static_cast<SharedBlock*>(
            MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedBlock)));

        if (block == nullptr)
        {
            CloseHandle(mapping);
            mapping = nullptr;
            return false;
        }

        return true;
    }

    HANDLE mapping = nullptr;
    HANDLE writeMutex = nullptr;
    SharedBlock* block = nullptr;
    bool writer = false;
#else
    SharedHarmonicContextSnapshot localSnapshot;
#endif
};
