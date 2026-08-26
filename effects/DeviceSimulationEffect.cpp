#include "effect.hpp"
#include "../utils/cubicSpline.hpp"
#include <algorithm>
#include <cmath>

DeviceSimulationEffect::DeviceSimulationEffect(bool enabled) 
    : Effect(enabled),
    runable(false),
    cache(fft_size),
    forward_plan(fft_size, FFTW_FORWARD, cache, cache, FFTW_ESTIMATE),
    backward_plan(fft_size, FFTW_BACKWARD, cache, cache, FFTW_ESTIMATE) {}

DeviceSimulationEffect::~DeviceSimulationEffect() {}

void DeviceSimulationEffect::run(std::span<float> audio) {
    static_assert((bufferType() == BufferType::PLANAR), "DeviceSimulationEffect run with non-planar buffer type");

    if (!runable) {
        return;
    }

    convolver.convolve(audio, audio);
}

void DeviceSimulationEffect::reset() {
    convolver.reset();
}

void DeviceSimulationEffect::setFreqResponseFile(std::string self, std::string target) {
    freq_response_db.clear();
    self_fr_reader.load(self, 15.0f);
    const auto& self_equalization = self_fr_reader.get("equalization");
    const auto& freq_bins = self_fr_reader.get("frequency");

    if (freq_bins.empty() || self_equalization.empty()) {
        runable = false;
        return;
    }
    
    auto irs = generateIr(freq_bins, self_equalization);

    if (target != "") {
        target_fr_reader.load(target, 15.0f);
        const auto& target_error = target_fr_reader.get("error");

        if (target_error.empty()) {
            runable = false;
            return;
        }

        if (self_equalization.size() != target_error.size()) {
            LOG_D("equalization != error, file: %s, %s", self.c_str(), target.c_str());
            runable = false;
            return;
        }

        std::vector<float> mags = self_equalization;

        for (int i = 0; i < self_equalization.size(); i++) {
            mags[i] += target_error[i];
        }

        irs = generateIr(freq_bins, mags);
    }

    if (irs.empty()) {
        runable = false;
        return;
    }

    convolver.setIr(irs, 2);
    computeFreqResponse(irs[0]);

    runable = true;
}

Priority DeviceSimulationEffect::priority() const {
    return Priority::DEVICE_SIMULATION_EFFECT;
}

void DeviceSimulationEffect::setFreqResponseConfig(std::string config) {
    this->config = config;

    auto newline = config.find('\n');
    const std::string self = newline == std::string::npos ? config : config.substr(0, newline);
    const std::string target = newline == std::string::npos ? "" : config.substr(newline + 1);

    setFreqResponseFile(self, target);
}

auto DeviceSimulationEffect::generateIr(const std::vector<float>& freq_bins, const std::vector<float>& mags) -> std::vector<std::vector<float>> {
    std::vector<float> freq_extended;
    std::vector<float> mag_extended;

    auto pushPoint = [&freq_extended, &mag_extended](float f, float m) {
        freq_extended.push_back(f);
        mag_extended.push_back(m);
    };

    pushPoint(0.0f, mags[0]);
    for (int i = 0; i < (int)freq_bins.size(); i++) {
        pushPoint(freq_bins[i], mags[i]);
    }
    pushPoint(getSampleRate() / 2.0f, mag_extended.back());

    if (freq_extended.size() < 2) {
        runable = false;
        return {};
    }

    CubicSpline cubic_spline(freq_extended, mag_extended);
    
    int target_nums = fft_half + 1;
    std::vector<float> target_freqs(target_nums);
    for (int i = 0; i < target_nums; i++) {
        target_freqs[i] = 0.5f * i * getSampleRate() / (target_nums - 1);
    }

    auto mag_interpolated = cubic_spline.interpolate(target_freqs);
    for (int i = 0; i < mag_interpolated.size(); i++) {
        mag_interpolated[i] = std::clamp(mag_interpolated[i], -24.0f, 24.0f);
    }

    static constexpr float db_to_log = 0.1151292546f;
    cache.init(0);

    cache[0][0] = mag_interpolated[0] * db_to_log;
    cache[0][1] = 0.0f;

    for (int i = 1; i < target_nums - 1; i++) {
        cache[i][0] = mag_interpolated[i] * db_to_log;
        cache[i][1] = 0.0f;

        cache[fft_size - i][0] = cache[i][0];
        cache[fft_size - i][1] = 0.0f;
    }

    cache[fft_half][0] = mag_interpolated[fft_half] * db_to_log;
    cache[fft_half][1] = 0.0f;

    backward_plan.execute(cache, cache);

    cache[0][0] = cache[0][0] / fft_size;
    for (int i = 1; i < fft_half; i++) {
        cache[i][0] = cache[i][0] / fft_size * 2.0f;
    }
    cache[fft_half][0] = cache[fft_half][0] / fft_size;
    cache.init(0, fft_half + 1, fft_half - 1);

    forward_plan.execute(cache, cache);

    for (int i = 0; i < fft_size; i++) {
        *((std::complex<float>*)&(cache[i])) = std::exp(*((std::complex<float>*)&(cache[i])));
    }

    backward_plan.execute(cache, cache);
    for (int i = 0; i < fft_size; i++) {
        cache[i][0] /= fft_size;
    }

    std::vector<std::vector<float>> irs(2, std::vector<float>(fft_size));
    for (int i = 0; i < fft_size; i++) {
        irs[0][i] = cache[i][0];
        irs[1][i] = cache[i][0];
    }

    return irs;
}

void DeviceSimulationEffect::copyParamsFrom(const DeviceSimulationEffect& other) {
    this->reset();

    this->config = other.config;
    setFreqResponseConfig(config);

    this->setEnabled(other.isEnabled());
}

void DeviceSimulationEffect::computeFreqResponse(const std::vector<float>& ir) {
    freq_response_db.clear();

    if (ir.size() != fft_size) {
        return;
    }

    double energy = 1e-13;
    for (int i = 0; i < fft_size; i++) {
        energy += (double)ir[i] * ir[i];
    }
    float gain_db = 20.0f * std::log10(1.0 / std::sqrt(energy));

    cache.init(0);
    for (int i = 0; i < fft_size; i++) {
        cache[i][0] = ir[i];
    }

    forward_plan.execute(cache, cache);

    freq_response_db.resize(fft_half + 1);
    for (int i = 0; i <= fft_half; i++) {
        float re = cache[i][0];
        float im = cache[i][1];
        float mag = std::sqrt(re * re + im * im);
        freq_response_db[i] = 20.0f * std::log10(mag + 1e-9f) + gain_db;
    }
}