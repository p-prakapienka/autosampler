#pragma once

#include <juce_core/juce_core.h>
#include "../AutosamplerState.h"
#include <map>
#include <vector>
#include <cstring>

/**
 * SF2 (SoundFont 2.04) Exporter
 *
 * Writes a minimal but spec-compliant SF2 file containing:
 *   - INFO sub-chunk (ifil, isng, INAM)
 *   - sdta sub-chunk (smpl - 16-bit PCM samples with 46 zero padding)
 *   - pdta sub-chunk (phdr, pbag, pmod, pgen, inst, ibag, imod, igen, shdr)
 *
 * Reference: SoundFont 2.04 Technical Specification (E.mu / Creative Labs)
 */
class SF2Exporter
{
public:
    static bool exportSamples(const std::map<int, SampleData>& samples,
                              const juce::String& packName,
                              const juce::File& outputDirectory)
    {
        if (samples.empty()) {
            return false;
        }

        if (!outputDirectory.exists()) {
            outputDirectory.createDirectory();
        }

        auto sf2File = outputDirectory.getChildFile(packName + ".sf2");
        auto outputStream = sf2File.createOutputStream();
        if (outputStream == nullptr) {
            return false;
        }

        // --- Build all sub-chunks in memory ---

        // 1. INFO sub-chunk
        auto infoChunk = buildInfoChunk(packName);

        // 2. sdta sub-chunk (16-bit PCM samples)
        std::vector<uint32_t> sampleStartOffsets;  // in sample frames
        std::vector<uint32_t> sampleEndOffsets;
        auto smplData = buildSmplData(samples, sampleStartOffsets, sampleEndOffsets);

        // 3. pdta sub-chunk
        auto pdtaChunk = buildPdtaChunk(samples, packName, sampleStartOffsets, sampleEndOffsets);

        // sdta chunk = LIST('sdta', smpl sub-chunk)
        juce::MemoryBlock sdtaChunk;
        {
            juce::MemoryOutputStream sdtaStream(sdtaChunk, false);
            sdtaStream.write("sdta", 4);
            writeChunkHeader(sdtaStream, "smpl", (uint32_t) smplData.getSize());
            sdtaStream.write(smplData.getData(), smplData.getSize());
        }

        // --- Write RIFF/sfbk ---
        uint32_t listInfoSize = 4 + (uint32_t) infoChunk.getSize();
        uint32_t listSdtaSize = 4 + 8 + (uint32_t) smplData.getSize(); // 'sdta' + chunk header + data
        uint32_t listPdtaSize = 4 + (uint32_t) pdtaChunk.getSize();

        uint32_t riffSize = 4 // 'sfbk'
            + 8 + listInfoSize  // LIST + size + INFO content
            + 8 + listSdtaSize  // LIST + size + sdta content
            + 8 + listPdtaSize; // LIST + size + pdta content

        outputStream->write("RIFF", 4);
        writeUint32LE(*outputStream, riffSize);
        outputStream->write("sfbk", 4);

        // LIST INFO
        outputStream->write("LIST", 4);
        writeUint32LE(*outputStream, listInfoSize);
        outputStream->write("INFO", 4);
        outputStream->write(infoChunk.getData(), infoChunk.getSize());

        // LIST sdta
        outputStream->write("LIST", 4);
        writeUint32LE(*outputStream, listSdtaSize);
        outputStream->write("sdta", 4);
        writeChunkHeader(*outputStream, "smpl", (uint32_t) smplData.getSize());
        outputStream->write(smplData.getData(), smplData.getSize());

        // LIST pdta
        outputStream->write("LIST", 4);
        writeUint32LE(*outputStream, listPdtaSize);
        outputStream->write("pdta", 4);
        outputStream->write(pdtaChunk.getData(), pdtaChunk.getSize());

        outputStream->flush();
        return true;
    }

private:
    static constexpr int SF2_SAMPLE_PADDING = 46; // spec requires 46 zero samples between

    // --- Helper: write little-endian uint32 ---
    static void writeUint32LE(juce::OutputStream& out, uint32_t val)
    {
        out.writeByte((char) (val & 0xFF));
        out.writeByte((char) ((val >> 8) & 0xFF));
        out.writeByte((char) ((val >> 16) & 0xFF));
        out.writeByte((char) ((val >> 24) & 0xFF));
    }

    static void writeUint16LE(juce::OutputStream& out, uint16_t val)
    {
        out.writeByte((char) (val & 0xFF));
        out.writeByte((char) ((val >> 8) & 0xFF));
    }

    static void writeInt16LE(juce::OutputStream& out, int16_t val)
    {
        writeUint16LE(out, static_cast<uint16_t>(val));
    }

    static void writeInt8(juce::OutputStream& out, int8_t val)
    {
        out.writeByte(val);
    }

    static void writeChunkHeader(juce::OutputStream& out, const char* id, uint32_t size)
    {
        out.write(id, 4);
        writeUint32LE(out, size);
    }

    // Write a fixed-length string field, padded with zeros
    static void writeFixedString(juce::OutputStream& out, const juce::String& str, int maxLen)
    {
        auto utf8 = str.toUTF8();
        int len = juce::jmin((int) strlen(utf8), maxLen - 1);
        out.write(utf8, (size_t) len);
        for (int i = len; i < maxLen; i++) {
            out.writeByte(0);
        }
    }

    // ---- INFO chunk ----
    static juce::MemoryBlock buildInfoChunk(const juce::String& packName)
    {
        juce::MemoryBlock block;
        juce::MemoryOutputStream out(block, false);

        // ifil: SoundFont version 2.04
        writeChunkHeader(out, "ifil", 4);
        writeUint16LE(out, 2);  // wMajor
        writeUint16LE(out, 4);  // wMinor

        // isng: sound engine
        juce::String engineName = "EMU8000";
        uint32_t isngSize = (uint32_t) engineName.length() + 1;
        if (isngSize % 2 != 0) {
            isngSize++; // pad to even
        }
        writeChunkHeader(out, "isng", isngSize);
        writeFixedString(out, engineName, (int) isngSize);

        // INAM: bank name
        uint32_t inamSize = (uint32_t) packName.length() + 1;
        if (inamSize % 2 != 0) {
            inamSize++;
        }
        writeChunkHeader(out, "INAM", inamSize);
        writeFixedString(out, packName, (int) inamSize);

        return block;
    }

    // ---- smpl data: interleaved 16-bit mono samples with padding ----
    static juce::MemoryBlock buildSmplData(const std::map<int, SampleData>& samples,
                                            std::vector<uint32_t>& startOffsets,
                                            std::vector<uint32_t>& endOffsets)
    {
        juce::MemoryBlock block;
        juce::MemoryOutputStream out(block, false);

        uint32_t currentOffset = 0; // in sample frames

        for (auto& [midiNote, data] : samples) {
            int numSamples = data.audioBuffer.getNumSamples();
            startOffsets.push_back(currentOffset);

            // Write mono (use first channel only for SF2)
            const float* channelData = data.audioBuffer.getReadPointer(0);
            for (int i = 0; i < numSamples; i++) {
                float sample = juce::jlimit(-1.0f, 1.0f, channelData[i]);
                int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
                writeInt16LE(out, pcm);
            }

            currentOffset += (uint32_t) numSamples;
            endOffsets.push_back(currentOffset);

            // Write 46 zero padding samples (required by SF2 spec)
            for (int i = 0; i < SF2_SAMPLE_PADDING; i++) {
                writeInt16LE(out, 0);
            }

            currentOffset += SF2_SAMPLE_PADDING;
        }

        return block;
    }

    // ---- pdta chunk: preset, instrument, sample header tables ----
    static juce::MemoryBlock buildPdtaChunk(const std::map<int, SampleData>& samples,
                                             const juce::String& packName,
                                             const std::vector<uint32_t>& sampleStarts,
                                             const std::vector<uint32_t>& sampleEnds)
    {
        juce::MemoryBlock block;
        juce::MemoryOutputStream out(block, false);

        int numSamples = (int) samples.size();

        // Calculate lokey/hikey per sample (same logic as SFZ exporter)
        std::vector<int> midiNotes;
        std::vector<double> sampleRates;
        for (auto& [note, data] : samples) {
            midiNotes.push_back(note);
            sampleRates.push_back(data.sampleRate);
        }

        std::vector<int> lokeys(numSamples), hikeys(numSamples);
        for (int i = 0; i < numSamples; i++) {
            if (i == 0) {
                lokeys[i] = midiNotes[i];
            } else {
                lokeys[i] = midiNotes[i - 1] + (midiNotes[i] - midiNotes[i - 1]) / 2 + 1;
            }

            if (i == numSamples - 1) {
                hikeys[i] = midiNotes[i];
            } else {
                hikeys[i] = midiNotes[i] + (midiNotes[i + 1] - midiNotes[i]) / 2;
            }
        }

        // ========== phdr (Preset Header) ==========
        // One preset + terminal record
        // Each phdr record is 38 bytes
        {
            uint32_t phdrSize = 38 * 2;
            writeChunkHeader(out, "phdr", phdrSize);

            // Preset 0
            writeFixedString(out, packName, 20);
            writeUint16LE(out, 0);   // wPreset (program number)
            writeUint16LE(out, 0);   // wBank
            writeUint16LE(out, 0);   // wPresetBagNdx
            writeUint32LE(out, 0);   // dwLibrary
            writeUint32LE(out, 0);   // dwGenre
            writeUint32LE(out, 0);   // dwMorphology

            // Terminal preset
            writeFixedString(out, "EOP", 20);
            writeUint16LE(out, 0);
            writeUint16LE(out, 0);
            writeUint16LE(out, 1);   // wPresetBagNdx = num bags
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
        }

        // ========== pbag (Preset Bag) ==========
        // One bag for the preset + terminal
        // Each pbag record is 4 bytes
        {
            uint32_t pbagSize = 4 * 2;
            writeChunkHeader(out, "pbag", pbagSize);

            // Bag 0: points to first pgen
            writeUint16LE(out, 0);   // wGenNdx
            writeUint16LE(out, 0);   // wModNdx

            // Terminal bag
            writeUint16LE(out, 1);   // wGenNdx (1 generator: instrument)
            writeUint16LE(out, 0);
        }

        // ========== pmod (Preset Modulator) ==========
        // Terminal only = 10 bytes
        {
            writeChunkHeader(out, "pmod", 10);
            for (int i = 0; i < 10; i++) {
                out.writeByte(0);
            }
        }

        // ========== pgen (Preset Generator) ==========
        // One generator (instrument reference) + terminal
        // Each pgen record is 4 bytes
        {
            uint32_t pgenSize = 4 * 2;
            writeChunkHeader(out, "pgen", pgenSize);

            // Generator: instrument (genOper=41)
            writeUint16LE(out, 41);  // instrument
            writeUint16LE(out, 0);   // instrument index 0

            // Terminal
            writeUint16LE(out, 0);
            writeUint16LE(out, 0);
        }

        // ========== inst (Instrument) ==========
        // One instrument + terminal
        // Each inst record is 22 bytes
        {
            uint32_t instSize = 22 * 2;
            writeChunkHeader(out, "inst", instSize);

            // Instrument 0
            writeFixedString(out, packName, 20);
            writeUint16LE(out, 0);   // wInstBagNdx

            // Terminal instrument
            writeFixedString(out, "EOI", 20);
            writeUint16LE(out, (uint16_t) numSamples); // wInstBagNdx
        }

        // ========== ibag (Instrument Bag) ==========
        // One bag per sample + terminal
        // Each ibag record is 4 bytes
        {
            uint32_t ibagSize = 4 * ((uint32_t) numSamples + 1);
            writeChunkHeader(out, "ibag", ibagSize);

            for (int i = 0; i < numSamples; i++) {
                // Each bag uses 4 generators: keyRange, overridingRootKey, sampleModes, sampleID
                writeUint16LE(out, (uint16_t) (i * 4)); // wInstGenNdx
                writeUint16LE(out, 0);                    // wInstModNdx
            }

            // Terminal bag
            writeUint16LE(out, (uint16_t) (numSamples * 4));
            writeUint16LE(out, 0);
        }

        // ========== imod (Instrument Modulator) ==========
        // Terminal only = 10 bytes
        {
            writeChunkHeader(out, "imod", 10);
            for (int i = 0; i < 10; i++) {
                out.writeByte(0);
            }
        }

        // ========== igen (Instrument Generator) ==========
        // 4 generators per sample + terminal
        // Each igen record is 4 bytes
        {
            uint32_t igenSize = 4 * ((uint32_t) numSamples * 4 + 1);
            writeChunkHeader(out, "igen", igenSize);

            for (int i = 0; i < numSamples; i++) {
                // keyRange (genOper=43)
                writeUint16LE(out, 43);
                writeInt8(out, (int8_t) lokeys[i]);   // lo
                writeInt8(out, (int8_t) hikeys[i]);    // hi

                // overridingRootKey (genOper=58)
                writeUint16LE(out, 58);
                writeInt16LE(out, (int16_t) midiNotes[i]);

                // sampleModes (genOper=54) - 0=no loop
                writeUint16LE(out, 54);
                writeInt16LE(out, 0);

                // sampleID (genOper=53)
                writeUint16LE(out, 53);
                writeInt16LE(out, (int16_t) i);
            }

            // Terminal generator
            writeUint16LE(out, 0);
            writeUint16LE(out, 0);
        }

        // ========== shdr (Sample Header) ==========
        // One per sample + terminal
        // Each shdr record is 46 bytes
        {
            uint32_t shdrSize = 46 * ((uint32_t) numSamples + 1);
            writeChunkHeader(out, "shdr", shdrSize);

            for (int i = 0; i < numSamples; i++) {
                juce::String sampleName = getMidiNoteName(midiNotes[i]);
                writeFixedString(out, sampleName, 20);

                writeUint32LE(out, sampleStarts[(size_t) i]);                           // dwStart
                writeUint32LE(out, sampleEnds[(size_t) i]);                             // dwEnd
                writeUint32LE(out, sampleStarts[(size_t) i]);                           // dwStartLoop
                writeUint32LE(out, sampleEnds[(size_t) i]);                             // dwEndLoop
                writeUint32LE(out, (uint32_t) sampleRates[(size_t) i]);                 // dwSampleRate
                writeInt8(out, (int8_t) midiNotes[i]);                                  // byOriginalKey
                writeInt8(out, 0);                                                       // chCorrection
                writeUint16LE(out, 0);                                                   // wSampleLink
                writeUint16LE(out, 1);                                                   // sfSampleType: monoSample
            }

            // Terminal sample header
            writeFixedString(out, "EOS", 20);
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
            writeUint32LE(out, 0);
            writeInt8(out, 0);
            writeInt8(out, 0);
            writeUint16LE(out, 0);
            writeUint16LE(out, 0);
        }

        return block;
    }
};
