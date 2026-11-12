#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>

#include "SoundManager.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {
    const int   SAMPLE_RATE    = 44100;
    const int   BUFFER_COUNT   = 4;
    const int   BUFFER_SAMPLES = 512;
    const int   MAX_VOICES     = 16;
    const float PI             = 3.14159265358979323846f;

    struct SoundBuffer {
        std::vector<float> samples;
    };

    struct Voice {
        bool  active = false;
        int   sfxIndex = 0;
        size_t cursor = 0;
        float volume = 1.0f;
    };

    // Audio engine state
    bool             gInitialized = false;
    volatile bool    gRunning     = false;
    HWAVEOUT         gWaveOut     = nullptr;
    HANDLE           gAudioEvent  = nullptr;
    HANDLE           gThread      = nullptr;
    CRITICAL_SECTION gLock;

    WAVEHDR gHeaders[BUFFER_COUNT];
    int16_t gOutBuffers[BUFFER_COUNT][BUFFER_SAMPLES];

    std::vector<SoundBuffer> gSounds;
    Voice                    gVoices[MAX_VOICES];

    // Fast deterministic PRNG for procedural noise synthesis
    struct NoiseGen {
        uint32_t state = 0x12345678u;
        float next() {
            state = state * 1664525u + 1013904223u;
            return ((state >> 8) & 0x00FFFFFF) / 8388607.5f - 1.0f;
        }
    };

    struct BandpassSVF {
        float low  = 0.0f;
        float band = 0.0f;

        float process(float input, float freqHz, float q) {
            float clampedFreq = std::max(40.0f, std::min(freqHz, 7500.0f));
            float f = 2.0f * std::sin(PI * clampedFreq / (float)SAMPLE_RATE);
            float damp = 1.0f / std::max(0.5f, q);
            low  += f * band;
            float high = input - low - damp * band;
            band += f * high;
            return band;
        }
    };

    struct LowpassOnePole {
        float prev = 0.0f;

        float process(float input, float cutoffHz) {
            float rc = 1.0f / (2.0f * PI * std::max(20.0f, cutoffHz));
            float dt = 1.0f / (float)SAMPLE_RATE;
            float alpha = dt / (rc + dt);
            prev += alpha * (input - prev);
            return prev;
        }
    };

    float attackDecayEnv(float t, float attackSec, float durationSec, float curve = 1.8f) {
        if (t < 0.0f || t >= durationSec) return 0.0f;
        float att = (attackSec > 0.0001f && t < attackSec) ? (t / attackSec) : 1.0f;
        float decT = std::max(0.0f, (t - attackSec) / std::max(0.0001f, durationSec - attackSec));
        float dec = std::pow(1.0f - std::min(1.0f, decT), curve);
        return att * dec;
    }

    // Adds a frequency-swept sine/triangle tone starting at startTimeSec
    void addSweptTone(std::vector<float>& buf, float startTimeSec, float durationSec,
                      float freqStart, float freqEnd, float amp,
                      float attackSec, float decayCurve, bool addHarmonic = false) {
        int startIdx = std::max(0, (int)(startTimeSec * SAMPLE_RATE));
        int count    = (int)(durationSec * SAMPLE_RATE);
        int endIdx   = std::min((int)buf.size(), startIdx + count);

        double phase = 0.0;
        for (int i = startIdx; i < endIdx; ++i) {
            float t = (float)(i - startIdx) / (float)SAMPLE_RATE;
            float u = t / durationSec;
            float freq = freqStart + (freqEnd - freqStart) * (1.0f - std::pow(1.0f - u, 1.5f));
            phase += 2.0 * PI * freq / SAMPLE_RATE;
            float sig = (float)std::sin(phase);
            if (addHarmonic) {
                sig += 0.35f * (float)std::sin(phase * 2.0) + 0.15f * (float)std::sin(phase * 3.0);
            }
            buf[i] += sig * amp * attackDecayEnv(t, attackSec, durationSec, decayCurve);
        }
    }

    // Adds a bandpass-filtered noise sweep (ideal for martial-arts whooshes and air slices)
    void addNoiseSweep(std::vector<float>& buf, NoiseGen& rng, float startTimeSec, float durationSec,
                       float freqStart, float freqEnd, float q, float amp,
                       float attackSec, float decayCurve) {
        int startIdx = std::max(0, (int)(startTimeSec * SAMPLE_RATE));
        int count    = (int)(durationSec * SAMPLE_RATE);
        int endIdx   = std::min((int)buf.size(), startIdx + count);

        BandpassSVF filter;
        for (int i = startIdx; i < endIdx; ++i) {
            float t = (float)(i - startIdx) / (float)SAMPLE_RATE;
            float u = t / durationSec;
            float freq = freqStart + (freqEnd - freqStart) * u;
            float filtered = filter.process(rng.next(), freq, q);
            buf[i] += filtered * amp * attackDecayEnv(t, attackSec, durationSec, decayCurve);
        }
    }

    // Adds an impact noise crunch with decaying lowpass cutoff
    void addImpactCrunch(std::vector<float>& buf, NoiseGen& rng, float startTimeSec, float durationSec,
                         float cutoffStart, float cutoffEnd, float amp, float decayCurve) {
        int startIdx = std::max(0, (int)(startTimeSec * SAMPLE_RATE));
        int count    = (int)(durationSec * SAMPLE_RATE);
        int endIdx   = std::min((int)buf.size(), startIdx + count);

        LowpassOnePole lp;
        for (int i = startIdx; i < endIdx; ++i) {
            float t = (float)(i - startIdx) / (float)SAMPLE_RATE;
            float u = t / durationSec;
            float cutoff = cutoffStart + (cutoffEnd - cutoffStart) * u;
            float filtered = lp.process(rng.next(), cutoff);
            buf[i] += filtered * amp * attackDecayEnv(t, 0.002f, durationSec, decayCurve);
        }
    }

    // Adds inharmonic partials for metallic clang / bronze gong / temple bell
    void addMetallicPartials(std::vector<float>& buf, float startTimeSec, float durationSec,
                             const float* freqs, const float* amps, int numPartials,
                             float ampScale, float decayRate) {
        int startIdx = std::max(0, (int)(startTimeSec * SAMPLE_RATE));
        int count    = (int)(durationSec * SAMPLE_RATE);
        int endIdx   = std::min((int)buf.size(), startIdx + count);

        for (int i = startIdx; i < endIdx; ++i) {
            float t = (float)(i - startIdx) / (float)SAMPLE_RATE;
            float attack = (t < 0.004f) ? (t / 0.004f) : 1.0f;
            float sum = 0.0f;
            for (int p = 0; p < numPartials; ++p) {
                float pDecay = std::exp(-decayRate * (1.0f + p * 0.45f) * t);
                sum += amps[p] * pDecay * std::sin(2.0f * PI * freqs[p] * t);
            }
            buf[i] += sum * attack * ampScale;
        }
    }

    void normalizeAndShape(std::vector<float>& buf, float targetPeak) {
        float peak = 0.0f;
        for (float s : buf) {
            float a = std::fabs(s);
            if (a > peak) peak = a;
        }
        if (peak < 0.0001f) return;
        float scale = targetPeak / peak;
        for (float& s : buf) {
            s *= scale;
        }
    }

    SoundBuffer synthesizeSound(SoundEffect sfx) {
        NoiseGen rng;
        rng.state ^= (uint32_t)sfx * 2654435761u;

        std::vector<float> buf;

        switch (sfx) {
            case SoundEffect::PUNCH_WHOOSH: {
                buf.assign((size_t)(0.14f * SAMPLE_RATE), 0.0f);
                addNoiseSweep(buf, rng, 0.0f, 0.13f, 1500.0f, 420.0f, 1.8f, 1.0f, 0.012f, 2.0f);
                addSweptTone(buf, 0.0f, 0.08f, 240.0f, 110.0f, 0.18f, 0.005f, 2.2f);
                normalizeAndShape(buf, 0.48f);
                break;
            }

            case SoundEffect::KICK_WHOOSH: {
                buf.assign((size_t)(0.22f * SAMPLE_RATE), 0.0f);
                addNoiseSweep(buf, rng, 0.0f, 0.20f, 950.0f, 220.0f, 1.6f, 1.0f, 0.020f, 1.8f);
                addSweptTone(buf, 0.0f, 0.14f, 185.0f, 75.0f, 0.28f, 0.010f, 2.0f);
                normalizeAndShape(buf, 0.58f);
                break;
            }

            case SoundEffect::SPECIAL_CHARGE: {
                buf.assign((size_t)(0.25f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.24f, 240.0f, 640.0f, 0.55f, 0.03f, 1.3f, true);
                addNoiseSweep(buf, rng, 0.0f, 0.24f, 400.0f, 1900.0f, 2.5f, 0.45f, 0.04f, 1.4f);
                normalizeAndShape(buf, 0.45f);
                break;
            }

            case SoundEffect::SPECIAL_WHOOSH: {
                buf.assign((size_t)(0.30f * SAMPLE_RATE), 0.0f);
                addNoiseSweep(buf, rng, 0.0f, 0.28f, 1900.0f, 180.0f, 1.5f, 1.0f, 0.015f, 1.6f);
                addSweptTone(buf, 0.0f, 0.22f, 340.0f, 85.0f, 0.45f, 0.010f, 1.8f, true);
                normalizeAndShape(buf, 0.68f);
                break;
            }

            case SoundEffect::HIT_PUNCH: {
                buf.assign((size_t)(0.18f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.11f, 175.0f, 48.0f, 1.0f, 0.002f, 2.4f);
                addImpactCrunch(buf, rng, 0.0f, 0.09f, 2400.0f, 420.0f, 0.95f, 3.0f);
                normalizeAndShape(buf, 0.82f);
                break;
            }

            case SoundEffect::HIT_KICK: {
                buf.assign((size_t)(0.26f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.16f, 145.0f, 36.0f, 1.15f, 0.002f, 2.0f);
                addImpactCrunch(buf, rng, 0.0f, 0.16f, 2000.0f, 280.0f, 1.0f, 2.3f);
                addNoiseSweep(buf, rng, 0.0f, 0.07f, 1100.0f, 350.0f, 1.2f, 0.45f, 0.001f, 2.5f);
                normalizeAndShape(buf, 0.90f);
                break;
            }

            case SoundEffect::HIT_SPECIAL: {
                buf.assign((size_t)(0.42f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.26f, 165.0f, 30.0f, 1.25f, 0.002f, 1.7f, true);
                addImpactCrunch(buf, rng, 0.0f, 0.32f, 3200.0f, 240.0f, 1.1f, 1.9f);
                const float freqs[] = { 320.0f, 510.0f, 790.0f };
                const float amps[]  = { 0.4f, 0.25f, 0.15f };
                addMetallicPartials(buf, 0.0f, 0.35f, freqs, amps, 3, 0.35f, 9.0f);
                normalizeAndShape(buf, 0.95f);
                break;
            }

            case SoundEffect::HIT_BLOCK: {
                buf.assign((size_t)(0.24f * SAMPLE_RATE), 0.0f);
                const float freqs[] = { 720.0f, 1140.0f, 1790.0f, 2560.0f };
                const float amps[]  = { 0.55f, 0.45f, 0.30f, 0.18f };
                addMetallicPartials(buf, 0.0f, 0.22f, freqs, amps, 4, 0.9f, 14.0f);
                addImpactCrunch(buf, rng, 0.0f, 0.05f, 3500.0f, 900.0f, 0.55f, 3.2f);
                normalizeAndShape(buf, 0.72f);
                break;
            }

            case SoundEffect::KO: {
                buf.assign((size_t)(0.98f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.34f, 135.0f, 28.0f, 1.3f, 0.002f, 1.5f);
                addImpactCrunch(buf, rng, 0.0f, 0.38f, 2600.0f, 180.0f, 1.0f, 1.8f);
                const float freqs[] = { 110.0f, 165.0f, 238.0f, 316.0f, 445.0f };
                const float amps[]  = { 0.75f, 0.50f, 0.35f, 0.22f, 0.14f };
                addMetallicPartials(buf, 0.02f, 0.95f, freqs, amps, 5, 0.85f, 2.8f);
                normalizeAndShape(buf, 0.96f);
                break;
            }

            case SoundEffect::JUMP: {
                buf.assign((size_t)(0.15f * SAMPLE_RATE), 0.0f);
                addNoiseSweep(buf, rng, 0.0f, 0.14f, 320.0f, 1050.0f, 1.7f, 0.85f, 0.015f, 2.0f);
                addSweptTone(buf, 0.0f, 0.10f, 180.0f, 340.0f, 0.22f, 0.008f, 2.0f);
                normalizeAndShape(buf, 0.42f);
                break;
            }

            case SoundEffect::LAND: {
                buf.assign((size_t)(0.11f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.07f, 95.0f, 42.0f, 0.85f, 0.002f, 2.5f);
                addImpactCrunch(buf, rng, 0.0f, 0.08f, 420.0f, 140.0f, 0.65f, 2.8f);
                normalizeAndShape(buf, 0.40f);
                break;
            }

            case SoundEffect::BLOCK_GUARD: {
                buf.assign((size_t)(0.09f * SAMPLE_RATE), 0.0f);
                addNoiseSweep(buf, rng, 0.0f, 0.08f, 650.0f, 1100.0f, 2.0f, 0.8f, 0.006f, 2.4f);
                addSweptTone(buf, 0.0f, 0.05f, 260.0f, 320.0f, 0.20f, 0.004f, 2.5f);
                normalizeAndShape(buf, 0.30f);
                break;
            }

            case SoundEffect::ROUND_START: {
                buf.assign((size_t)(0.90f * SAMPLE_RATE), 0.0f);
                // Taiko drum strike + temple gong resonance
                addSweptTone(buf, 0.0f, 0.28f, 105.0f, 52.0f, 0.95f, 0.003f, 2.0f);
                const float freqs[] = { 220.0f, 330.0f, 488.0f, 656.0f };
                const float amps[]  = { 0.65f, 0.42f, 0.24f, 0.14f };
                addMetallicPartials(buf, 0.01f, 0.88f, freqs, amps, 4, 0.75f, 3.2f);
                normalizeAndShape(buf, 0.72f);
                break;
            }

            case SoundEffect::FIGHT_START: {
                buf.assign((size_t)(0.45f * SAMPLE_RATE), 0.0f);
                // Crisp Japanese wooden hyoshigi double-clack + bright gong accent
                const float woodFreqs[] = { 1180.0f, 1760.0f, 2620.0f };
                const float woodAmps[]  = { 0.65f, 0.45f, 0.25f };
                addMetallicPartials(buf, 0.00f, 0.09f, woodFreqs, woodAmps, 3, 0.9f, 32.0f);
                addMetallicPartials(buf, 0.09f, 0.14f, woodFreqs, woodAmps, 3, 1.0f, 26.0f);
                const float gongFreqs[] = { 330.0f, 495.0f, 660.0f };
                const float gongAmps[]  = { 0.50f, 0.30f, 0.18f };
                addMetallicPartials(buf, 0.09f, 0.35f, gongFreqs, gongAmps, 3, 0.55f, 6.5f);
                normalizeAndShape(buf, 0.75f);
                break;
            }

            case SoundEffect::VICTORY: {
                buf.assign((size_t)(1.15f * SAMPLE_RATE), 0.0f);
                // Ascending Japanese Yo pentatonic chime (D4, E4, G4, A4, D5)
                const float notes[] = { 293.66f, 329.63f, 392.00f, 440.00f, 587.33f };
                const float times[] = { 0.00f, 0.11f, 0.22f, 0.33f, 0.46f };
                for (int n = 0; n < 5; ++n) {
                    float f = notes[n];
                    float freqs[] = { f, f * 2.0f, f * 3.0f };
                    float amps[]  = { 0.70f, 0.28f, 0.10f };
                    float dur = (n == 4) ? 0.68f : 0.35f;
                    float decay = (n == 4) ? 3.5f : 6.0f;
                    addMetallicPartials(buf, times[n], dur, freqs, amps, 3, 0.65f, decay);
                }
                normalizeAndShape(buf, 0.70f);
                break;
            }

            case SoundEffect::DEFEAT: {
                buf.assign((size_t)(1.15f * SAMPLE_RATE), 0.0f);
                // Somber descending tone + low gong
                const float notes[] = { 220.00f, 174.61f, 146.83f };
                const float times[] = { 0.00f, 0.22f, 0.46f };
                for (int n = 0; n < 3; ++n) {
                    float f = notes[n];
                    float freqs[] = { f, f * 1.5f, f * 2.0f };
                    float amps[]  = { 0.70f, 0.25f, 0.12f };
                    float dur = (n == 2) ? 0.68f : 0.40f;
                    float decay = (n == 2) ? 3.2f : 5.0f;
                    addMetallicPartials(buf, times[n], dur, freqs, amps, 3, 0.65f, decay);
                }
                normalizeAndShape(buf, 0.68f);
                break;
            }

            case SoundEffect::UI_CLICK: {
                buf.assign((size_t)(0.055f * SAMPLE_RATE), 0.0f);
                const float freqs[] = { 960.0f, 1440.0f };
                const float amps[]  = { 0.70f, 0.35f };
                addMetallicPartials(buf, 0.0f, 0.050f, freqs, amps, 2, 0.8f, 45.0f);
                addImpactCrunch(buf, rng, 0.0f, 0.015f, 2200.0f, 800.0f, 0.25f, 3.0f);
                normalizeAndShape(buf, 0.42f);
                break;
            }

            case SoundEffect::UI_BACK: {
                buf.assign((size_t)(0.065f * SAMPLE_RATE), 0.0f);
                const float freqs[] = { 620.0f, 930.0f };
                const float amps[]  = { 0.70f, 0.30f };
                addMetallicPartials(buf, 0.0f, 0.060f, freqs, amps, 2, 0.8f, 38.0f);
                normalizeAndShape(buf, 0.40f);
                break;
            }

            case SoundEffect::UI_START: {
                buf.assign((size_t)(0.38f * SAMPLE_RATE), 0.0f);
                addSweptTone(buf, 0.0f, 0.22f, 130.0f, 58.0f, 0.85f, 0.002f, 2.2f);
                const float freqs[] = { 587.33f, 880.00f, 1174.66f };
                const float amps[]  = { 0.65f, 0.35f, 0.18f };
                addMetallicPartials(buf, 0.0f, 0.36f, freqs, amps, 3, 0.70f, 7.5f);
                normalizeAndShape(buf, 0.62f);
                break;
            }
        }

        SoundBuffer sb;
        sb.samples = std::move(buf);
        return sb;
    }

    void mixIntoBuffer(int16_t* out) {
        float mix[BUFFER_SAMPLES] = { 0.0f };

        EnterCriticalSection(&gLock);
        for (int v = 0; v < MAX_VOICES; ++v) {
            Voice& voice = gVoices[v];
            if (!voice.active) continue;

            const std::vector<float>& src = gSounds[voice.sfxIndex].samples;
            size_t avail = (voice.cursor < src.size()) ? (src.size() - voice.cursor) : 0;
            size_t toMix = std::min((size_t)BUFFER_SAMPLES, avail);

            for (size_t i = 0; i < toMix; ++i) {
                mix[i] += src[voice.cursor + i] * voice.volume;
            }
            voice.cursor += toMix;
            if (voice.cursor >= src.size()) {
                voice.active = false;
            }
        }
        LeaveCriticalSection(&gLock);

        for (int i = 0; i < BUFFER_SAMPLES; ++i) {
            float s = std::tanh(mix[i] * 0.90f);
            int val = (int)(s * 32000.0f);
            if (val > 32767) val = 32767;
            if (val < -32768) val = -32768;
            out[i] = (int16_t)val;
        }
    }

    DWORD WINAPI audioThreadProc(LPVOID) {
        while (gRunning) {
            WaitForSingleObject(gAudioEvent, 50);
            if (!gRunning) break;

            for (int i = 0; i < BUFFER_COUNT; ++i) {
                if (gHeaders[i].dwFlags & WHDR_DONE) {
                    mixIntoBuffer(gOutBuffers[i]);
                    waveOutWrite(gWaveOut, &gHeaders[i], sizeof(WAVEHDR));
                }
            }
        }
        return 0;
    }
} // namespace

namespace SoundManager {

void init() {
    if (gInitialized) return;

    InitializeCriticalSection(&gLock);

    // Pre-synthesize all sound effects once into memory
    const int totalEffects = (int)SoundEffect::UI_START + 1;
    gSounds.resize(totalEffects);
    for (int i = 0; i < totalEffects; ++i) {
        gSounds[i] = synthesizeSound((SoundEffect)i);
    }

    for (int v = 0; v < MAX_VOICES; ++v) {
        gVoices[v].active = false;
    }

    gAudioEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!gAudioEvent) {
        DeleteCriticalSection(&gLock);
        return;
    }

    WAVEFORMATEX wfx = {};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 1;
    wfx.nSamplesPerSec  = SAMPLE_RATE;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize          = 0;

    if (waveOutOpen(&gWaveOut, WAVE_MAPPER, &wfx,
                    (DWORD_PTR)gAudioEvent, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
        CloseHandle(gAudioEvent);
        gAudioEvent = nullptr;
        DeleteCriticalSection(&gLock);
        return;
    }

    gRunning = true;

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        for (int s = 0; s < BUFFER_SAMPLES; ++s) {
            gOutBuffers[i][s] = 0;
        }
        gHeaders[i] = {};
        gHeaders[i].lpData         = (LPSTR)gOutBuffers[i];
        gHeaders[i].dwBufferLength = BUFFER_SAMPLES * sizeof(int16_t);
        waveOutPrepareHeader(gWaveOut, &gHeaders[i], sizeof(WAVEHDR));
        waveOutWrite(gWaveOut, &gHeaders[i], sizeof(WAVEHDR));
    }

    gThread = CreateThread(nullptr, 0, audioThreadProc, nullptr, 0, nullptr);
    if (gThread) {
        SetThreadPriority(gThread, THREAD_PRIORITY_ABOVE_NORMAL);
    }

    gInitialized = true;
    std::atexit(shutdown);
}

void shutdown() {
    if (!gInitialized) return;
    gInitialized = false;

    gRunning = false;
    if (gAudioEvent) {
        SetEvent(gAudioEvent);
    }

    if (gThread) {
        WaitForSingleObject(gThread, 1000);
        CloseHandle(gThread);
        gThread = nullptr;
    }

    if (gWaveOut) {
        waveOutReset(gWaveOut);
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            waveOutUnprepareHeader(gWaveOut, &gHeaders[i], sizeof(WAVEHDR));
        }
        waveOutClose(gWaveOut);
        gWaveOut = nullptr;
    }

    if (gAudioEvent) {
        CloseHandle(gAudioEvent);
        gAudioEvent = nullptr;
    }

    DeleteCriticalSection(&gLock);
    gSounds.clear();
}

void play(SoundEffect sfx, float volume) {
    if (!gInitialized) return;
    int idx = (int)sfx;
    if (idx < 0 || idx >= (int)gSounds.size()) return;

    EnterCriticalSection(&gLock);
    int chosen = -1;
    size_t maxCursor = 0;

    for (int v = 0; v < MAX_VOICES; ++v) {
        if (!gVoices[v].active) {
            chosen = v;
            break;
        }
        if (gVoices[v].cursor >= maxCursor) {
            maxCursor = gVoices[v].cursor;
            chosen = v;
        }
    }

    if (chosen >= 0) {
        gVoices[chosen].active   = true;
        gVoices[chosen].sfxIndex = idx;
        gVoices[chosen].cursor   = 0;
        gVoices[chosen].volume   = std::max(0.0f, std::min(volume, 2.0f));
    }
    LeaveCriticalSection(&gLock);
}

} // namespace SoundManager
