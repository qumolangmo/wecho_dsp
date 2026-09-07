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

BassResonatorEffect::BassResonatorEffect(bool enabled, float gain, float high_gain, float center_freq, float q)
    : Effect(enabled)
    , gain(gain)
    , high_gain(high_gain)
    , center_freq(center_freq)
    , q(q) {

    float lp_fc = center_freq / std::sqrt(1.0f - 1.0f / (3.0f * q * q));
    float hp_fc = 0.5f * center_freq * (std::sqrt(4.0f + 1.0f / hp_q * hp_q) - 1.0f / hp_q);
    high_shelf[0].setHighShelf({1000, 1, high_gain});
    high_shelf[1].setHighShelf({1000, 1, high_gain});

    hp[0].setHighPass({hp_fc, hp_q});
    hp[1].setHighPass({hp_fc, hp_q});
    lp[0].setLowPass({lp_fc, q});
    lp[1].setLowPass({lp_fc, q});

    reset();
}

BassResonatorEffect::~BassResonatorEffect() {}

void BassResonatorEffect::reset() {
    hp[0].reset();
    hp[1].reset();
    lp[0].reset();
    lp[1].reset();
    high_shelf[0].reset();
    high_shelf[1].reset();
}

Priority BassResonatorEffect::priority() const {
    return Priority::BASS_RESONATOR_EFFECT;
}

void BassResonatorEffect::setGain(float gain) {
    this->gain.store(gain, std::memory_order_release);

}

void BassResonatorEffect::setHighGain(float high_gain) {
    this->high_gain.store(high_gain, std::memory_order_release);

    high_shelf[0].setHighShelf({1000, 1, high_gain});
    high_shelf[1].setHighShelf({1000, 1, high_gain});
}

void BassResonatorEffect::setCenterFreq(float center_freq) {
    this->center_freq.store(center_freq, std::memory_order_release);

    float q = this->q.load(std::memory_order_acquire);

    float lp_fc = center_freq / std::sqrt(1.0f - 1.0f / (3.0f * q * q));
    float hp_fc = 0.5f * center_freq * (std::sqrt(4.0f + 1.0f / hp_q * hp_q) - 1.0f / hp_q);
    hp[0].setHighPass({hp_fc, hp_q});
    hp[1].setHighPass({hp_fc, hp_q});
    lp[0].setLowPass({lp_fc, q});
    lp[1].setLowPass({lp_fc, q});
}

void BassResonatorEffect::setQ(float q) {
    this->q.store(q, std::memory_order_release);

    float center_freq = this->center_freq.load(std::memory_order_acquire);

    float lp_fc = center_freq / std::sqrt(1.0f - 1.0f / (3.0f * q * q));
    lp[0].setLowPass({lp_fc, q});
    lp[1].setLowPass({lp_fc, q});
}

void BassResonatorEffect::copyParamsFrom(const BassResonatorEffect& other) {
    reset();

    setQ(other.q.load(std::memory_order_acquire));
    setCenterFreq(other.center_freq.load(std::memory_order_acquire));
    setGain(other.gain.load(std::memory_order_acquire));
    setHighGain(other.high_gain.load(std::memory_order_acquire));

    setEnabled(other.acquireReadEnabled());
}

void BassResonatorEffect::run(std::span<float> audio) {
    static_assert(bufferType() == BufferType::INTERLEAVED, "BassResonatorEffect run with non-interleaved buffer type");

    float gain = this->gain.load(std::memory_order_relaxed);

    for (int i = 0; i < audio.size(); i += 2) {
        float l = audio[i];
        float r = audio[i + 1];

        l = lp[0].process(l);
        r = lp[1].process(r);
        l *= 6.8f * gain;
        r *= 6.8f * gain;
        l = hp[0].process(l);
        r = hp[1].process(r);

        audio[i] += l;
        audio[i + 1] += r;

        audio[i] = high_shelf[0].process(audio[i]);
        audio[i + 1] = high_shelf[1].process(audio[i + 1]);
    }
}