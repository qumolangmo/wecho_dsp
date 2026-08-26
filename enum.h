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

#ifndef __ENUM_H__
#define __ENUM_H__

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include "scripting/wecho_dsp_c_api.h"

using FloatArray = float[65536 * 2];
using FileName = char[4096];
using ScriptCode = char[65536 * 2];

struct Coeffs {
    int32_t index;
    int32_t start_freq;
    int32_t end_freq;
    int32_t gain;
};

using ScriptParamsArray = ScriptParams[16];

enum ParamType {
    PARAM_TYPE_BOOL,
    PARAM_TYPE_INT,
    PARAM_TYPE_FLOAT,
    PARAM_TYPE_STRING,
    PARAM_TYPE_ARRAY,
    PARAM_TYPE_SCRIPT_PARAMS,
};

#define EFFECT_PARAMS \
    X(MASTER_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(GAIN_EFFECT_GAIN, float, PARAM_TYPE_FLOAT) \
    X(BALANCE_EFFECT_BALANCE, float, PARAM_TYPE_FLOAT) \
    X(BASS_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(BASS_EFFECT_GAIN, int, PARAM_TYPE_INT) \
    X(BASS_EFFECT_CENTER_FREQ, int, PARAM_TYPE_INT) \
    X(BASS_EFFECT_Q, float, PARAM_TYPE_FLOAT) \
    X(CLARITY_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(CLARITY_EFFECT_GAIN, int, PARAM_TYPE_INT) \
    X(EVEN_HARMONIC_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(EVEN_HARMONIC_EFFECT_BASE, float, PARAM_TYPE_FLOAT) \
    X(EVEN_HARMONIC_EFFECT_WARM, float, PARAM_TYPE_FLOAT) \
    X(EVEN_HARMONIC_EFFECT_SUGAR, float, PARAM_TYPE_FLOAT) \
    X(CONVOLVE_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(CONVOLVE_EFFECT_MIX, float, PARAM_TYPE_FLOAT) \
    X(CONVOLVE_EFFECT_IR_PATH, FileName, PARAM_TYPE_STRING) \
    X(CONVOLVE_EFFECT_IR_DATA, FloatArray, PARAM_TYPE_ARRAY) \
    X(COMPRESSOR_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(COMPRESSOR_EFFECT_THRESHOLD, int, PARAM_TYPE_INT) \
    X(COMPRESSOR_EFFECT_RATIO, int, PARAM_TYPE_INT) \
    X(COMPRESSOR_EFFECT_MAKEUP_GAIN, int, PARAM_TYPE_INT) \
    X(COMPRESSOR_EFFECT_ATTACK, int, PARAM_TYPE_INT) \
    X(COMPRESSOR_EFFECT_RELEASE, int, PARAM_TYPE_INT) \
    X(LOOK_AHEAD_SOFT_LIMIT_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(LOW_CAT_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(LOW_CAT_EFFECT_CUTOFF_FREQ, int, PARAM_TYPE_INT) \
    X(IIR_EQUALIZER_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(IIR_EQUALIZER_EFFECT_CONFIG, std::string, PARAM_TYPE_STRING) \
    X(VIRTUALBASS_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(VIRTUALBASS_EFFECT_ENVELOPE_RATE, int, PARAM_TYPE_INT) \
    X(VIRTUALBASS_EFFECT_MID_GAIN, float, PARAM_TYPE_FLOAT) \
    X(VIRTUALBASS_EFFECT_HIGH_GAIN, float, PARAM_TYPE_FLOAT) \
    X(VIRTUALBASS_EFFECT_HARMONIC_GAIN, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(REVERB_EFFECT_ROOM_SIZE, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_DAMPING, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_MIX, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_STEREO_WIDTH, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_MOD_DEPTH, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_MOD_FREQ, float, PARAM_TYPE_FLOAT) \
    X(REVERB_EFFECT_PRE_DELAY, int, PARAM_TYPE_INT) \
    X(REVERB_EFFECT_MATRIX_TYPE, int, PARAM_TYPE_INT) \
    X(SCRIPT_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(SCRIPT_EFFECT_PARAMS, ScriptParamsArray, PARAM_TYPE_SCRIPT_PARAMS) \
    X(SCRIPT_EFFECT_CODE, ScriptCode, PARAM_TYPE_STRING) \
    X(DIFF_SURROUNDING_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(DIFF_SURROUNDING_EFFECT_DELAY_MS, int, PARAM_TYPE_INT) \
    X(DEVICE_SIMULATION_EFFECT_ENABLED, bool, PARAM_TYPE_BOOL) \
    X(DEVICE_SIMULATION_EFFECT_CONFIG, std::string, PARAM_TYPE_STRING) \
    X(MAX_EFFECT_PARAM, int, PARAM_TYPE_INT)

enum ParamID {
#define X(name, type, enum_type) name,
    EFFECT_PARAMS
#undef X
};

enum Priority {
    GAIN_EFFECT,
    CHANNEL_BALANCE_EFFECT,
    DEVICE_SIMULATION_EFFECT,
    DIFF_SURROUNDING_EFFECT,
    VIRTUALBASS_EFFECT,
    CONVOLVE_EFFECT,
    BASS_EFFECT,
    CLARITY_EFFECT,
    IIR_EQUALIZER_EFFECT,
    EVEN_HARMONIC_EFFECT,
    COMPRESSOR_EFFECT,
    LOOK_AHEAD_SOFT_LIMIT_EFFECT,
    LOW_CAT_EFFECT,
    REVERB_EFFECT,
    SCRIPT_EFFECT,
    MAX_PRIORITY_EFFECT
};

enum FilterType {
    LOW_PASS,
    BAND_PASS,
    HIGH_PASS,
};

enum BufferType {
    INTERLEAVED,
    PLANAR
};

#endif