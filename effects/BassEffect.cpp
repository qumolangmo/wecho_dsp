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
#include "effect.hpp"

/*********************************************BassEffect*****************************************************/
BassEffect::BassEffect(bool _enabled, int _gain, float _deep, float _punch)
    : Effect(_enabled) {

    setGain(_gain);
    setDeep(_deep);
    setPunch(_punch);

    hp_20_m.setHighPass({20, 0.7071f});
    hp_20_s.setHighPass(120);

    low_gain1.setLowShelf({60, 0.7f, 6.0f * _gain / 10});
    low_gain2.setPeak({45, 1.2f, 4.0f * _gain / 10});
    lp_env.setLowPass({120, 0.7071f});
    sub_lp.setLowPass({80, 0.7071f});
    sub_lp2.setLowPass({40, 0.7071f});

    attack_coeff = 1.0f - std::expf(-1.0f / (getSampleRate() * 0.03f));
    release_coeff = 1.0f - std::expf(-1.0f / (getSampleRate() * 0.2f));
    env_smooth = 0.0f;
    prev_low = 0.0f;

    comp_attack_coeff = 1.0f - std::expf(-1.0f / (getSampleRate() * 0.05f));
    comp_release_coeff = 1.0f - std::expf(-1.0f / (getSampleRate() * 0.4f));
    comp_env = 0.0f;
    sub_flip = 1.0f;
    prev_sub = 0.0f;

    reset();
}

BassEffect::~BassEffect() {}

Priority BassEffect::priority() const {
    return BASS_EFFECT;
}

void BassEffect::reset() {
    hp_20_m.reset();
    hp_20_s.reset();
    low_gain1.reset();
    low_gain2.reset();
    lp_env.reset();
    sub_lp.reset();
    sub_lp2.reset();

    env_smooth = 0.0f;
    prev_low = 0.0f;
    sub_flip = 1.0f;
    prev_sub = 0.0f;
    comp_env = 0.0f;
}

void BassEffect::setGain(int gain) {
    this->gain.store(gain, std::memory_order_release);

    low_gain1.setLowShelf({60, 0.7f, 6.0f * gain / 10});
    low_gain2.setPeak({45, 1.2f, 4.0f * gain / 10});
}

void BassEffect::setDeep(float deep) {
    this->deep.store(deep, std::memory_order_release);
}

void BassEffect::setPunch(float punch) {
    this->punch.store(punch, std::memory_order_release);
}

void BassEffect::copyParamsFrom(const BassEffect& other) {
    reset();

    setDeep(other.deep.load(std::memory_order_acquire));
    setPunch(other.punch.load(std::memory_order_acquire));
    setGain(other.gain.load(std::memory_order_acquire));

    setEnabled(other.acquireReadEnabled());
}

void BassEffect::run(std::span<float> audio) {
    static_assert((bufferType() == BufferType::INTERLEAVED), "BassEffect run with non-interleaved buffer type");

    float _deep = deep.load(std::memory_order_relaxed);
    float _gain = gain.load(std::memory_order_relaxed) / 10.0f;
    float _punch = punch.load(std::memory_order_relaxed);

    for (int i = 0; i < audio.size(); i += 2) {
        int l_idx = i;
        int r_idx = i + 1;

        float m = (audio[l_idx] + audio[r_idx]) * 0.5f;
        float s = (audio[l_idx] - audio[r_idx]) * 0.5f;
        float m_hp = hp_20_m.process(m);
        float m_low_origin = lp_env.process(m_hp);
        
        float env_abs = std::fabs(m_low_origin);
        if (env_abs > env_smooth) {
            env_smooth += (env_abs - env_smooth) * attack_coeff;
        } else {
            env_smooth += (env_abs - env_smooth) * release_coeff;
        }

        float dynamic_gain = 1.0f + _deep * _gain * (1.0f / (1.0f + env_smooth * 15.0f));

        float diff = m_low_origin - prev_low;
        prev_low = m_low_origin;
        float punch = diff * _punch * 100.0f;

        float m_low = m_low_origin;
        m_low = low_gain2.process(low_gain1.process(m_low));
        m_low *= dynamic_gain;
        m_low += punch;

        /* subharmonic: flip-flop divider flips polarity at downward zero crossings, 0.5x frequency */
        float sub_in = sub_lp.process(m_low_origin);
        if (prev_sub > 0.0f && sub_in <= 0.0f) {
            sub_flip = -sub_flip;
        }
        prev_sub = sub_in;
        m_low += sub_lp2.process(sub_in * sub_flip) * _deep * _gain * 1.5f;

        /* slow compression evens out low band dynamics, keeps sustained chest pressure */
        // float low_abs = std::fabs(m_low);
        // if (low_abs > comp_env) {
        //     comp_env += (low_abs - comp_env) * comp_attack_coeff;
        // } else {
        //     comp_env += (low_abs - comp_env) * comp_release_coeff;
        // }
        // float comp_gain = 1.0f;
        // float over = comp_env - 0.5f;
        // if (over > 0.0f) {
        //     comp_gain = (0.2f + over / 3.0f) / comp_env;
        // }

        float delta = (m_low - m_low_origin); // * comp_gain;
        float m_out = m_hp + delta;

        float s_hp = hp_20_s.process(s);
        
        audio[l_idx] = m_out + s_hp;
        audio[r_idx] = m_out - s_hp;
    }
}
