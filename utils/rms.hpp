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
#ifndef __RMS_HPP__
#define __RMS_HPP__

#include <vector>
#include "utils.h"

class RMS: public Utils {
private:
    int sample_rate = getSampleRate();
    std::vector<float> buffer;
    float attack_ms, release_ms, window_ms;
    float attack_coeff;
    float release_coeff;
    float rms_sum;
    float makeup_gain;
    float envelope;
    int rms_idx;

public:
    RMS()
        : buffer(0)
        , attack_ms(0)
        , release_ms(0)
        , window_ms(0)
        , rms_sum(0.0f)
        , makeup_gain(0.0f)
        , attack_coeff(0.0f)
        , release_coeff(0.0f)
        , rms_idx(0)
        , envelope(0.0f) {
        
        /* 100ms */
        buffer.reserve(sample_rate / 10);
    }

    ~RMS() = default;

    void setWindowMs(int ms) {
        window_ms = ms;
        buffer.resize(sample_rate / 1000 * ms);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        rms_idx = 0;
    }

    void setAttackMs(int ms) {
        attack_ms = ms;
        attack_coeff = 1.0f - std::exp(-1.0f / (sample_rate / 1000.0f * ms));
    }

    void setReleaseMs(int ms) {
        release_ms = ms;
        release_coeff = 1.0f - std::exp(-1.0f / (sample_rate / 1000.0f * ms));
    }

    void setMakeupGain(float gain) {
        makeup_gain = gain;
    }

    void reset() {
        rms_sum = 0.0f;
        envelope = 0.0f;
        rms_idx = 0;

        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }

    float process(float sample) {
        rms_sum -= buffer[rms_idx] * buffer[rms_idx];

        buffer[rms_idx++] = sample;
        rms_idx = rms_idx % buffer.size();

        rms_sum += sample * sample;

        float rms = std::sqrt(std::max(0.0f, rms_sum / buffer.size()));
        rms *= makeup_gain;

        if (rms > envelope) {
            envelope += attack_coeff * (rms - envelope);
        } else {
            envelope += release_coeff * (rms - envelope);
        }

        return envelope;
    }

};

#endif
