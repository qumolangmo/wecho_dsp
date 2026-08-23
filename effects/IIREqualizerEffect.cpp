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

IIREqualizerEffect::IIREqualizerEffect(bool enabled) 
    : Effect(enabled) {

    biquads[0].reserve(20);
    biquads[1].reserve(20);
}

IIREqualizerEffect::~IIREqualizerEffect() {}

void IIREqualizerEffect::run(std::span<float> audio) {
    static_assert((bufferType() == BufferType::INTERLEAVED), "IREqualizerEffect run with non-interleaved buffer type");

    float origin_l, origin_r;

    float inner_preamp = this->preamp.load(std::memory_order_relaxed);
    inner_preamp = std::pow(10, inner_preamp / 20.0f);

    for (auto& sample: audio) {
        sample *= inner_preamp;
    }

    for (int i = 0; i < audio.size(); i += 2) {
        origin_l = audio[i];
        origin_r = audio[i + 1];

        for (int j = 0; j < biquads[0].size(); j++) {
            origin_l = biquads[0][j].process(origin_l);
            origin_r = biquads[1][j].process(origin_r);
        }
        
        audio[i] = origin_l;
        audio[i + 1] = origin_r;
    }
}

Priority IIREqualizerEffect::priority() const {
    return Priority::IIR_EQUALIZER_EFFECT;
}

void IIREqualizerEffect::reset() {
    biquads[0].clear();
    biquads[1].clear();
}

void IIREqualizerEffect::setCoeffs(std::string coeffs) {
    this->coeffs = coeffs;
    
    const auto& filter_group = parser.parse(coeffs);
    this->preamp.store(parser.getPreamp(), std::memory_order_relaxed);
    

    biquads[0].clear();
    biquads[1].clear();

    for (int i = 0; i < filter_group.size(); i++) {
        biquads[0].emplace_back(Biquad<1>());
        biquads[1].emplace_back(Biquad<1>());

        auto& left = biquads[0].back();
        auto& right = biquads[1].back();
        auto& param = filter_group[i];

        switch (filter_group[i].type) {
            case ParameterEqParser::FilterType::PK: {
                left.setPeak({{param.fc, param.q, param. gain}});
                right.setPeak({{param.fc, param.q, param.gain}});
                break;
            }
            case ParameterEqParser::FilterType::LSC: {
                left.setLowShelf({{param.fc, param.q, param.gain}});
                right.setLowShelf({{param.fc, param.q, param.gain}});
                break;
            }
            case ParameterEqParser::FilterType::HSC: {
                left.setHighShelf({{param.fc, param.q, param.gain}});
                right.setHighShelf({{param.fc, param.q, param.gain}});
                break;
            }
        }
    }
}

void IIREqualizerEffect::copyParamsFrom(const IIREqualizerEffect& other) {
    reset();

    this->coeffs = other.coeffs;
    this->setCoeffs(other.coeffs);
    this->setEnabled(other.isEnabled());
}
