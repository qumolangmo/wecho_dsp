/*
 * Copyright (C) 2026 qumolangmo
 *
 * This file is part of Wecho.
 *
 * Wecho is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wecho is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wecho.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_PROCESSOR_HPP__
#define __AUDIO_PROCESSOR_HPP__
#include "effects/effect.hpp"
#include "utils/audioStream.hpp"
#include "utils/crossFader.hpp"

#include <any>
#include <atomic>
#include <cstring>
#include <unordered_map>
#include <functional>
#include "enum.h"
#include "utils/debug.hpp"

class ParamSetter {
private:
    std::function<void(std::any, bool)> setter;
    static inline constexpr std::array<const char*, MAX_EFFECT_PARAM + 1> params_name = {{
        #define X(name, type, enum_type) #name,
        EFFECT_PARAMS
        #undef X
    }};
public:
    explicit ParamSetter() : setter([](std::any, bool){}) {}

    template<typename T>
    ParamSetter(ParamID param_id, std::function<void(T, bool)> func) {
        setter = [func, param_id](std::any value, bool initialize){
            T data = std::any_cast<T>(value);

            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool>) {
                LOG_D("update %s: %d", params_name[param_id], data);
            } else if constexpr (std::is_same_v<T, float>) {
                LOG_D("update %s: %.2f", params_name[param_id], data);
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (param_id == SCRIPT_EFFECT_CODE) {
                    LOG_D("update %s, config ignored print", params_name[param_id]);
                } else {
                    LOG_D("update %s: %.*s", params_name[param_id], 1024, data.c_str());
                }
            }

            func(data, initialize);
        };
    }

    void operator()(ParamID, std::any value, bool initialize = false) {
        setter(value, initialize);
    }
};

class AudioProcessor {
private:
    CrossFader<BassEffect> EBass;
    CrossFader<ClarityEffect> EClarity;
    GainEffect EGain;
    ChannelBalanceEffect EChannelBalance;
    CrossFader<EvenHarmonicEffect> EEvenHarmonic;
    CompressorEffect ECompressor;
    CrossFader<ConvolveEffect> EConvolve;
    CrossFader<VirtualBassEffect> EVirtualBass;
    CrossFader<LookAheadSoftLimitEffect> ELookAheadSoftLimiter;
    CrossFader<LowCatEffect> ELowCat;
    CrossFader<IIREqualizerEffect> EIIREQualizer;
    CrossFader<ReverbEffect> EReverb;
    // CrossFader<ScriptEffect> EScript;
    ScriptEffect EScript;
    CrossFader<DiffSurroundingEffect> EDiffSurrounding;

    std::unordered_map<ParamID, ParamSetter> param_map;
    AudioStream audio_stream;

    std::atomic<bool> master_enabled{true};

private:
    AudioProcessor()
        : audio_stream()
        , EBass(50.0, false, 0, 1.48f, 60.0f)
        , EClarity(50.0, false, 0)
        , EGain(false, 0)
        , EChannelBalance(false, 0)
        , EEvenHarmonic(80.0, false, 0.0f, 0.0f, 0.0f)
        , EConvolve(100, false, 0.1f)
        , ECompressor(false)
        , ELookAheadSoftLimiter(100, false)
        , ELowCat(30, false, 120)
        , EIIREQualizer(30, false)
        , EVirtualBass(100, false)
        , EReverb(300, false)
        , EScript(false)
        , EDiffSurrounding(30, false, 3) {

        param_map = {
            {MASTER_EFFECT_ENABLED,
                ParamSetter(MASTER_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    master_enabled.store(enabled, std::memory_order_relaxed);
                }))},
            {GAIN_EFFECT_GAIN,
                ParamSetter(GAIN_EFFECT_GAIN, std::function<void(float, bool)>([this](float gain, bool) {
                    EGain.setGain(gain);
                }))},
            {BALANCE_EFFECT_BALANCE,
                ParamSetter(BALANCE_EFFECT_BALANCE, std::function<void(float, bool)>([this](float balance, bool initialize) {
                    EChannelBalance.setBalance(balance);
                }))},
            {BASS_EFFECT_ENABLED,
                ParamSetter(BASS_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EBass.update([enabled](BassEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {BASS_EFFECT_GAIN,
                ParamSetter(BASS_EFFECT_GAIN, std::function<void(int, bool)>([this](int gain, bool initialize) {
                    EBass.update([gain](BassEffect& effect) {
                        effect.setGain(gain);
                    }, initialize);
                }))},
            {BASS_EFFECT_CENTER_FREQ,
                ParamSetter(BASS_EFFECT_CENTER_FREQ, std::function<void(int, bool)>([this](int center_freq, bool initialize) {
                    EBass.update([center_freq](BassEffect& effect) {
                        effect.setCenterFreq(center_freq);
                    }, initialize);
                }))},
            {BASS_EFFECT_Q,
                ParamSetter(BASS_EFFECT_Q, std::function<void(float, bool)>([this](float Q, bool initialize) {
                    EBass.update([Q](BassEffect& effect) {
                        effect.setQ(Q);
                    }, initialize);
                }))},
            {CLARITY_EFFECT_ENABLED,
                ParamSetter(CLARITY_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EClarity.update([enabled](ClarityEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {CLARITY_EFFECT_GAIN,
                ParamSetter(CLARITY_EFFECT_GAIN, std::function<void(int, bool)>([this](int gain, bool initialize) {
                    EClarity.update([gain](ClarityEffect& effect) {
                        effect.setGain(gain);
                    }, initialize);
                }))},
            {EVEN_HARMONIC_EFFECT_ENABLED,
                ParamSetter(EVEN_HARMONIC_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EEvenHarmonic.update([enabled](EvenHarmonicEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {EVEN_HARMONIC_EFFECT_BASE,
                ParamSetter(EVEN_HARMONIC_EFFECT_BASE, std::function<void(float, bool)>([this](float base, bool initialize) {
                    EEvenHarmonic.update([base](EvenHarmonicEffect& effect) {
                        effect.setBase(base);
                    }, initialize);
                }))},
            {EVEN_HARMONIC_EFFECT_WARM,
                ParamSetter(EVEN_HARMONIC_EFFECT_WARM, std::function<void(float, bool)>([this](float warm, bool initialize) {
                    EEvenHarmonic.update([warm](EvenHarmonicEffect& effect) {
                        effect.setWarm(warm);
                    }, initialize);
                }))},
            {EVEN_HARMONIC_EFFECT_SUGAR,
                ParamSetter(EVEN_HARMONIC_EFFECT_SUGAR, std::function<void(float, bool)>([this](float sugar, bool initialize) {
                    EEvenHarmonic.update([sugar](EvenHarmonicEffect& effect) {
                        effect.setSugar(sugar);
                    }, initialize);
                }))},
            {CONVOLVE_EFFECT_ENABLED,
                ParamSetter(CONVOLVE_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EConvolve.update([enabled](ConvolveEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {CONVOLVE_EFFECT_MIX,
                ParamSetter(CONVOLVE_EFFECT_MIX, std::function<void(float, bool)>([this](float mix, bool initialize) {
                    EConvolve.update([mix](ConvolveEffect& effect) {
                        effect.setMix(mix);
                    }, initialize);
                }))},
            {CONVOLVE_EFFECT_IR_PATH,
                ParamSetter(CONVOLVE_EFFECT_IR_PATH, std::function<void(std::string, bool)>([this](std::string ir_path, bool initialize) {
                    EConvolve.update([ir_path](ConvolveEffect& effect) {
                        effect.setIr(ir_path);
                    }, initialize);
                }))},
            {CONVOLVE_EFFECT_IR_DATA,
                ParamSetter(CONVOLVE_EFFECT_IR_DATA, std::function<void(std::vector<std::vector<float>>&, bool)>([this](std::vector<std::vector<float>>& ir_data, bool initialize) {
                    EConvolve.update([ir_data](ConvolveEffect& effect) {
                        effect.setIr(ir_data);
                    }, initialize);
                }))},
            {COMPRESSOR_EFFECT_ENABLED,
                ParamSetter(COMPRESSOR_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool) {
                    ECompressor.setEnabled(enabled);
                }))},
            {COMPRESSOR_EFFECT_THRESHOLD,
                ParamSetter(COMPRESSOR_EFFECT_THRESHOLD, std::function<void(int, bool)>([this](int threshold, bool) {
                    ECompressor.setThreshold(threshold);
                }))},
            {COMPRESSOR_EFFECT_RATIO,
                ParamSetter(COMPRESSOR_EFFECT_RATIO, std::function<void(int, bool)>([this](int ratio, bool) {
                    ECompressor.setRatio(ratio);
                }))},
            {COMPRESSOR_EFFECT_MAKEUP_GAIN,
                ParamSetter(COMPRESSOR_EFFECT_MAKEUP_GAIN, std::function<void(int, bool)>([this](int makeup_gain, bool) {
                    ECompressor.setMakeupGain(makeup_gain);
                }))},
            {COMPRESSOR_EFFECT_ATTACK,
                ParamSetter(COMPRESSOR_EFFECT_ATTACK, std::function<void(int, bool)>([this](int attack, bool) {
                    ECompressor.setAttack(attack);
                }))},
            {COMPRESSOR_EFFECT_RELEASE,
                ParamSetter(COMPRESSOR_EFFECT_RELEASE, std::function<void(int, bool)>([this](int release, bool) {
                    ECompressor.setRelease(release);
                }))},
            {LOOK_AHEAD_SOFT_LIMIT_EFFECT_ENABLED,
                ParamSetter(LOOK_AHEAD_SOFT_LIMIT_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    ELookAheadSoftLimiter.update([enabled](LookAheadSoftLimitEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {LOW_CAT_EFFECT_ENABLED,
                ParamSetter(LOW_CAT_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    ELowCat.update([enabled](LowCatEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {LOW_CAT_EFFECT_CUTOFF_FREQ,
                ParamSetter(LOW_CAT_EFFECT_CUTOFF_FREQ, std::function<void(int, bool)>([this](int cutoff_freq, bool initialize) {
                    ELowCat.update([cutoff_freq](LowCatEffect& effect) {
                        effect.setCutoffFreq(cutoff_freq);
                    }, initialize);
                }))},
            {IIR_EQUALIZER_EFFECT_ENABLED,
                ParamSetter(IIR_EQUALIZER_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EIIREQualizer.update([enabled](IIREqualizerEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {IIR_EQUALIZER_EFFECT_CONFIG,
                ParamSetter(IIR_EQUALIZER_EFFECT_CONFIG, std::function<void(std::string, bool)>([this](std::string coeffs, bool initialize) {
                    EIIREQualizer.update([coeffs](IIREqualizerEffect& effect) {
                        effect.setCoeffs(coeffs);
                    }, initialize);
                }))},
            {VIRTUALBASS_EFFECT_ENABLED,
                ParamSetter(VIRTUALBASS_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EVirtualBass.update([enabled](VirtualBassEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {VIRTUALBASS_EFFECT_ENVELOPE_RATE,
                ParamSetter(VIRTUALBASS_EFFECT_ENVELOPE_RATE, std::function<void(int, bool)>([this](int envelope_rate, bool initialize) {
                    EVirtualBass.update([envelope_rate](VirtualBassEffect& effect) {
                        effect.setEnvelopeRate(envelope_rate);
                    }, initialize);
                }))},
            {VIRTUALBASS_EFFECT_MID_GAIN,
                ParamSetter(VIRTUALBASS_EFFECT_MID_GAIN, std::function<void(float, bool)>([this](float mid_gain, bool initialize) {
                    EVirtualBass.update([mid_gain](VirtualBassEffect& effect) {
                        effect.setMidGain(mid_gain);
                    }, initialize);
                }))},
            {VIRTUALBASS_EFFECT_HIGH_GAIN,
                ParamSetter(VIRTUALBASS_EFFECT_HIGH_GAIN, std::function<void(float, bool)>([this](float high_gain, bool initialize) {
                    EVirtualBass.update([high_gain](VirtualBassEffect& effect) {
                        effect.setHighGain(high_gain);
                    }, initialize);
                }))},
            {VIRTUALBASS_EFFECT_HARMONIC_GAIN,
                ParamSetter(VIRTUALBASS_EFFECT_HARMONIC_GAIN, std::function<void(float, bool)>([this](float harmonic_gain, bool initialize) {
                    EVirtualBass.update([harmonic_gain](VirtualBassEffect& effect) {
                        effect.setHarmonicGain(harmonic_gain);
                    }, initialize);
                }))},
            {REVERB_EFFECT_ENABLED,
                ParamSetter(REVERB_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EReverb.update([enabled](ReverbEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {REVERB_EFFECT_ROOM_SIZE,
                ParamSetter(REVERB_EFFECT_ROOM_SIZE, std::function<void(float, bool)>([this](float room_size, bool initialize) {
                    EReverb.update([room_size](ReverbEffect& effect) {
                        effect.setRoomSize(room_size);
                    }, initialize);
                }))},
            {REVERB_EFFECT_DAMPING,
                ParamSetter(REVERB_EFFECT_DAMPING, std::function<void(float, bool)>([this](float damping, bool initialize) {
                    EReverb.update([damping](ReverbEffect& effect) {
                        effect.setDamping(damping);
                    }, initialize);
                }))},
            {REVERB_EFFECT_MIX,
                ParamSetter(REVERB_EFFECT_MIX, std::function<void(float, bool)>([this](float mix, bool initialize) {
                    EReverb.update([mix](ReverbEffect& effect) {
                        effect.setMix(mix);
                    }, initialize);
                }))},
            {REVERB_EFFECT_STEREO_WIDTH,
                ParamSetter(REVERB_EFFECT_STEREO_WIDTH, std::function<void(float, bool)>([this](float stereo_width, bool initialize) {
                    EReverb.update([stereo_width](ReverbEffect& effect) {
                        effect.setStereoWidth(stereo_width);
                    }, initialize);
                }))},
            {REVERB_EFFECT_MOD_DEPTH,
                ParamSetter(REVERB_EFFECT_MOD_DEPTH, std::function<void(float, bool)>([this](float mod_depth, bool initialize) {
                    EReverb.update([mod_depth](ReverbEffect& effect) {
                        effect.setModDepth(mod_depth);
                    }, initialize);
                }))},
            {REVERB_EFFECT_MOD_FREQ,
                ParamSetter(REVERB_EFFECT_MOD_FREQ, std::function<void(float, bool)>([this](float mod_freq, bool initialize) {
                    EReverb.update([mod_freq](ReverbEffect& effect) {
                        effect.setModFreq(mod_freq);
                    }, initialize);
                }))},
            {REVERB_EFFECT_PRE_DELAY,
                ParamSetter(REVERB_EFFECT_PRE_DELAY, std::function<void(int, bool)>([this](int pre_delay, bool initialize) {
                    EReverb.update([pre_delay](ReverbEffect& effect) {
                        effect.setPreDelay(pre_delay);
                    }, initialize);
                }))},
            {REVERB_EFFECT_MATRIX_TYPE,
                ParamSetter(REVERB_EFFECT_MATRIX_TYPE, std::function<void(int, bool)>([this](int matrix_type, bool initialize) {
                    EReverb.update([matrix_type](ReverbEffect& effect) {
                        effect.setMatrixType(matrix_type);
                    }, initialize);
                }))},
            {DIFF_SURROUNDING_EFFECT_ENABLED,
                ParamSetter(DIFF_SURROUNDING_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool initialize) {
                    EDiffSurrounding.update([enabled](DiffSurroundingEffect& effect) {
                        effect.setEnabled(enabled);
                    }, initialize);
                }))},
            {DIFF_SURROUNDING_EFFECT_DELAY_MS,
                ParamSetter(DIFF_SURROUNDING_EFFECT_DELAY_MS, std::function<void(int, bool)>([this](int delay_ms, bool initialize) {
                    EDiffSurrounding.update([delay_ms](DiffSurroundingEffect& effect) {
                        effect.setDelayMs(delay_ms);
                    }, initialize);
                }))},
            {SCRIPT_EFFECT_ENABLED,
                ParamSetter(SCRIPT_EFFECT_ENABLED, std::function<void(bool, bool)>([this](bool enabled, bool) {
                    EScript.setEnabled(enabled);
                }))},
            {SCRIPT_EFFECT_CODE,
                ParamSetter(SCRIPT_EFFECT_CODE, std::function<void(std::string, bool)>([this](std::string code, bool) {
                    EScript.setCode(code);
                }))},
            {SCRIPT_EFFECT_PARAMS,
                ParamSetter(SCRIPT_EFFECT_PARAMS, std::function<void(ScriptParams*, bool)>([this](ScriptParams* params, bool) {
                    EScript.setParams(params);
                }))},
        };
    }

public:
    static AudioProcessor& getInstance() {
        static AudioProcessor instance;
        return instance;
    }

    static void init(std::string_view files_dir, int sample_rate, int samples_per_channel, int channels) {
        // FFTWFPlan::initWisdom(std::string(files_dir) + "/fftw_wisdom");
        ScriptEffect::setCacheDir(files_dir);
        Utils::setChannels(channels);
        Utils::setSampleRate(sample_rate);
        Utils::setSamplesPerChannel(samples_per_channel);
    }

    // TODO: check out KFR/DSP
    void process(float *input, float *output, int length) noexcept {
        if (!master_enabled.load(std::memory_order_relaxed)) {
            std::memcpy(output, input, length * sizeof(float));
            return;
        }

        audio_stream << input;

        /* Ensure that all effects requiring planar buffers are placed consecutively in the processing chain */
        audio_stream >> EChannelBalance >> EDiffSurrounding
                     >> EIIREQualizer >> EEvenHarmonic >> EBass >> EClarity 
                     >> EVirtualBass >> EReverb 
                     /* planar buffer start */
                     >> EConvolve >> EScript
                     /* planar buffer end */
                     >> ECompressor >> ELowCat >> EGain
                     >> ELookAheadSoftLimiter >> output;
    }

    void setEffectParam(ParamID param, std::any value, bool initialize = false) {
        param_map[param](param, value, initialize);
    }

    void reset() {
        EBass.reset();
        EClarity.reset();
        EGain.reset();
        EChannelBalance.reset();
        EEvenHarmonic.reset();
        EBass.reset();
        ECompressor.reset();
        EConvolve.reset();
        ELookAheadSoftLimiter.reset();
        ELowCat.reset();
        EVirtualBass.reset();
    }
};
#endif