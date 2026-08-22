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

#ifndef __CONVOLVER_HPP__
#define __CONVOLVER_HPP__

#ifdef __WIN32__
#define FFTW_DLL
#endif

#include "../fftw/fftw3.h"

#include "../utils/AudioFile.hpp"
#include <memory>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <span>
#include "utils.h"

class FFTWFComplexArray: public Utils {
private:
    struct FFTWFMemoryDeleter {
        void operator()(fftwf_complex* ptr) {
            fftwf_free(ptr);
        }
    };

    std::unique_ptr<fftwf_complex[], FFTWFMemoryDeleter> data;
    int n;

public:
    explicit FFTWFComplexArray(int n = getSamplesPerChannel() * 2)
        : n(n)
        , data(nullptr) {

        data.reset(fftwf_alloc_complex(n));
    }

    FFTWFComplexArray(const FFTWFComplexArray&) = delete;
    FFTWFComplexArray& operator=(const FFTWFComplexArray&) = delete;

    FFTWFComplexArray(FFTWFComplexArray&& other)
        : n(other.n)
        , data(std::move(other.data)) {}

    FFTWFComplexArray& operator=(FFTWFComplexArray&& other) {
        n = other.n;
        data = std::move(other.data);
        return *this;
    }

    fftwf_complex& operator[](int i) {
        return data[i];
    }

    const fftwf_complex& operator[](int i) const {
        return data[i];
    }

    fftwf_complex* get() {
        return data.get();
    }

    const fftwf_complex* get() const {
        return data.get();
    }

    int size() const {
        return n;
    }

    void init(uint8_t value) {
        memset(data.get(), value, n * sizeof(fftwf_complex));
    }

    void init(uint8_t value, int start, int n) {
        memset(data.get() + start, value, n * sizeof(fftwf_complex));
    }

};

class FFTWFPlan: public Utils {
private:
    struct FFTWFPlanDeleter {
        void operator()(fftwf_plan ptr) {
            fftwf_destroy_plan(ptr);
        }
    };

    std::unique_ptr<std::remove_pointer_t<fftwf_plan>, FFTWFPlanDeleter> plan;
    size_t n;
    int sign;
    unsigned int flags;

    static void importWisdom() {
        if (std::filesystem::exists(wisdom_path) && !wisdom_imported) {
            fftwf_import_wisdom_from_filename(wisdom_path.data());
            wisdom_imported = true;
        }
    }

    static void exportWisdom() {
        if (std::filesystem::exists(wisdom_path) && wisdom_imported) {
            fftwf_export_wisdom_to_filename(wisdom_path.data());
        }
    }

public:
    static void initWisdom(std::string_view _wisdom_path) {
        wisdom_path = _wisdom_path;

        std::filesystem::path wisdom_dir = std::filesystem::path(wisdom_path).parent_path();
        if (!std::filesystem::exists(wisdom_dir)) {
            std::filesystem::create_directories(wisdom_dir);
        }

        std::string version_path = std::string(wisdom_path) + ".version";

        if (std::filesystem::exists(wisdom_path)) {
            if (std::filesystem::exists(version_path)) {
                std::ifstream ver_file(version_path);
                int stored_fft_size = 0;
                if (ver_file >> stored_fft_size && stored_fft_size != (getSamplesPerChannel() * 2)) {
                    std::filesystem::remove(wisdom_path);
                    std::filesystem::remove(version_path);
                }
            } else {
                std::filesystem::remove(wisdom_path);
            }
        }

        if (!std::filesystem::exists(wisdom_path)) {
            importWisdom();

            FFTWFComplexArray tmp(getSamplesPerChannel() * 2);
            FFTWFPlan plan(getSamplesPerChannel() * 2, FFTW_FORWARD, tmp, tmp, FFTW_MEASURE);
            FFTWFPlan backward_plan(getSamplesPerChannel() * 2, FFTW_BACKWARD, tmp, tmp, FFTW_MEASURE);

            std::ofstream ver_file(version_path);
            ver_file << getSamplesPerChannel() * 2;
        } else {
            importWisdom();
        }
        
    }

    static std::string wisdom_path;

    static bool wisdom_imported;

    static void saveWisdom() {
        exportWisdom();
    }

    FFTWFPlan(size_t n, int sign, FFTWFComplexArray& in, FFTWFComplexArray& out, unsigned int flags = FFTW_WISDOM_ONLY)
        : n(n)
        , sign(sign)
        , flags(flags) {

        plan.reset(fftwf_plan_dft_1d(n, in.get(), out.get(), sign, flags));
    }

    int fftSize() const {
        return n;
    }

    void execute(FFTWFComplexArray& in, FFTWFComplexArray& out) {
        fftwf_execute_dft(plan.get(), in.get(), out.get());
    }
};

class Convolver: public Utils {
public:
    Convolver()
        : Utils()
        , samples(4, std::vector<float>(MAX_SAMPLES_PER_CHANNEL))
        , compute_cache_left(fft_size)
        , compute_cache_right(fft_size)
        , sliding_window_left(fft_size)
        , sliding_window_right(fft_size)
        , forward_plan(fft_size, FFTW_FORWARD, compute_cache_left, compute_cache_right, FFTW_ESTIMATE)
        , backward_plan(fft_size, FFTW_BACKWARD, compute_cache_right, compute_cache_left, FFTW_ESTIMATE)
        , cache(fft_size)
        , delay_head(0)
        , valid_channels(0)
        , num_ir_blocks(0) {

        reset();
    }

    ~Convolver() {}

    void reset() {
        sliding_window_left.init(0);
        sliding_window_right.init(0);
        compute_cache_left.init(0);
        compute_cache_right.init(0);

        if (num_ir_blocks > 0) {
            delay_left.init(0);
            delay_right.init(0);
        }

        delay_head = 0;

        valid_channels = 0;
    }

    void normalize() {
        double energy = 1e-13;

        for (int i = 0; i < valid_channels; i++) {
            for (int j = 0; j < samples[i].size(); j++) {
                energy += samples[i][j] * samples[i][j];
            }
        }

        float gain = 1 / std::sqrt(energy);

        for (int i = 0; i < valid_channels; i++) {
            for (int j = 0; j < samples[i].size(); j++) {
                samples[i][j] *= gain;
            }
        }
    }

    void setIr(const std::vector<std::vector<float>>& ir, int channels) {
        valid_channels = channels;

        for (int i = 0; i < ir.size() && i < samples.size(); i++) {
            samples[i] = ir[i];
        }

        normalize();

        num_ir_blocks = (int)std::ceil(samples[0].size() / (float)samples_per_channel);

        this->ir = FFTWFComplexArray(num_ir_blocks * valid_channels * fft_size);
        this->ir.init(0);

        delay_left = FFTWFComplexArray(num_ir_blocks * fft_size);
        delay_right = FFTWFComplexArray(num_ir_blocks * fft_size);
        delay_left.init(0);
        delay_right.init(0);
        delay_head = 0;

        FFTWFComplexArray tmp(fft_size);
        for (int i = 0; i < num_ir_blocks; i++) {
            for (int j = 0; j < valid_channels; j++) {
                int k;
                for (k = 0; k < samples_per_channel && i * samples_per_channel + k < samples[j].size(); k++) {
                    tmp[k][0] = samples[j][i * samples_per_channel + k];
                    tmp[k][1] = 0.0f;
                }
                memset(tmp.get() + k, 0, (fft_size - k) * sizeof(fftwf_complex));

                forward_plan.execute(tmp, tmp);

                fftwf_complex* ir_block = this->ir.get() + i * fft_size * valid_channels;
                for (int k = 0; k < fft_size; k++) {
                    ir_block[k * valid_channels + j][0] = tmp[k][0];
                    ir_block[k * valid_channels + j][1] = tmp[k][1];
                }
            }
        }
    }

    void setIr(const std::string& ir_path) {
        if (!loadAudioFile(ir_path, samples)) {
            valid_channels = 2;
            samples[0].resize(samples_per_channel);
            samples[1].resize(samples_per_channel);
            std::memset(samples[0].data(), 0, sizeof(float) * samples_per_channel);
            std::memset(samples[1].data(), 0, sizeof(float) * samples_per_channel);
            samples[0][0] = 1.0f;
            samples[1][0] = 1.0f;
        }

        setIr(samples, valid_channels);
    }

    void convolve(const std::span<const float> input_frame, std::span<float> output_frame) {
        float mix_factor = mix.load(std::memory_order_relaxed);

        std::array<std::span<const float>, 2> input = {{
            std::span<const float>(input_frame.data(), samples_per_channel),
            std::span<const float>(input_frame.data() + samples_per_channel, samples_per_channel),
        }};

        std::array<std::span<float>, 2> output = {{
            std::span<float>(output_frame.data(), samples_per_channel),
            std::span<float>(output_frame.data() + samples_per_channel, samples_per_channel),
        }};

        memcpy(sliding_window_left.get(),
                sliding_window_left.get() + samples_per_channel,
                (fft_size - samples_per_channel) * sizeof(fftwf_complex));
        memcpy(sliding_window_right.get(),
                sliding_window_right.get() + samples_per_channel,
                (fft_size - samples_per_channel) * sizeof(fftwf_complex));

        for (int i = samples_per_channel; i < fft_size; i++) {
            if ((i - samples_per_channel) < input[0].size()) {
                sliding_window_left[i][0] = input[0][i - samples_per_channel];
                sliding_window_right[i][0] = input[1][i - samples_per_channel];
                sliding_window_left[i][1] = 0.0f;
                sliding_window_right[i][1] = 0.0f;
            } else {
                sliding_window_left[i][0] = 0.0f;
                sliding_window_right[i][0] = 0.0f;
                sliding_window_left[i][1] = 0.0f;
                sliding_window_right[i][1] = 0.0f;
            }
        }

        delay_head = (delay_head - 1 + num_ir_blocks) % num_ir_blocks;

        forward_plan.execute(sliding_window_left, compute_cache_left);
        forward_plan.execute(sliding_window_right, compute_cache_right);

        memcpy(delay_left.get() + delay_head * fft_size,
               compute_cache_left.get(),
               fft_size * sizeof(fftwf_complex));
        memcpy(delay_right.get() + delay_head * fft_size,
               compute_cache_right.get(),
               fft_size * sizeof(fftwf_complex));

        if (valid_channels == 2) {
            multiply<2>();
        } else if (valid_channels == 4) {
            multiply<4>();
        }

        backward_plan.execute(compute_cache_left, compute_cache_left);
        backward_plan.execute(compute_cache_right, compute_cache_right);

        for (int i = 0; i < input[0].size(); i++) {
            output[0][i] = (compute_cache_left[i + samples_per_channel][0] / fft_size) 
                * mix_factor + (1.0f - mix_factor) * input[0][i];
            output[1][i] = (compute_cache_right[i + samples_per_channel][0] / fft_size) 
                * mix_factor + (1.0f - mix_factor) * input[1][i];
        }

        return;
    }

    void setMix(float mix) {
        this->mix.store(mix, std::memory_order_release);
    }

private:
    static constexpr int MAX_SAMPLES_PER_CHANNEL = 65536;

    template<int VALID_CHANNELS>
    void multiply() {
        compute_cache_left.init(0);
        compute_cache_right.init(0);

        const fftwf_complex* ir_base = this->ir.get();

        for (int i = 0; i < num_ir_blocks; i++) {
            const int delay_idx = (delay_head + i) % num_ir_blocks;
            const fftwf_complex* dl = delay_left.get() + delay_idx * fft_size;
            const fftwf_complex* dr = delay_right.get() + delay_idx * fft_size;

            const fftwf_complex* ir_ptr = ir_base + i * fft_size * VALID_CHANNELS;

            for (int j = 0; j < fft_size; j++) {
                const float dlr = dl[j][0];
                const float dli = dl[j][1];
                const float drr = dr[j][0];
                const float dri = dr[j][1];

                const fftwf_complex& ir0 = ir_ptr[0];
                const fftwf_complex& ir1 = ir_ptr[1];

                compute_cache_left[j][0] += dlr * ir0[0] - dli * ir0[1];
                compute_cache_left[j][1] += dlr * ir0[1] + dli * ir0[0];
                compute_cache_right[j][0] += drr * ir1[0] - dri * ir1[1];
                compute_cache_right[j][1] += drr * ir1[1] + dri * ir1[0];

                if constexpr (VALID_CHANNELS == 4) {
                    const fftwf_complex& ir2 = ir_ptr[2];
                    const fftwf_complex& ir3 = ir_ptr[3];
                    compute_cache_left[j][0] += dlr * ir2[0] - dli * ir2[1];
                    compute_cache_left[j][1] += dlr * ir2[1] + dli * ir2[0];
                    compute_cache_right[j][0] += drr * ir3[0] - dri * ir3[1];
                    compute_cache_right[j][1] += drr * ir3[1] + dri * ir3[0];
                }

                ir_ptr += VALID_CHANNELS;
            }
        }
    }

    /* max load 65536 samples per channel */
    bool loadAudioFile(const std::string& path, std::vector<std::vector<float>>& samples) {
        /* reserve 1MB date area default */
        static AudioFile<float> ir;

        if (!ir.load(path)) {
            return false;
        }

        if (ir.getNumChannels() == 1) {
            /* 128k samples = 128k * sizeof(float) = 512kB per cahnnel */
            int j;

            for (j = 0; j < ir.getNumSamplesPerChannel() && j < MAX_SAMPLES_PER_CHANNEL; j++) {
                samples[0][j] = ir.samples[0][j];
                samples[1][j] = ir.samples[0][j];
            }

            samples[0].resize(j);
            samples[1].resize(j);

            valid_channels = 2;
        } else if (ir.getNumChannels() == 2) {
            int i, j;

            for (i = 0; i < 2; i++) {
                for (j = 0; j < ir.getNumSamplesPerChannel() && j < MAX_SAMPLES_PER_CHANNEL; j++) {
                    samples[i][j] = ir.samples[i][j];
                }

                samples[i].resize(j);
            }

            valid_channels = 2;
        } else if (ir.getNumChannels() == 4) {
            int i, j;

            for (i = 0; i < 4; i++) {
                for (j = 0; j < ir.getNumSamplesPerChannel() && j < MAX_SAMPLES_PER_CHANNEL; j++) {
                    samples[i][j] = ir.samples[i][j];
                }

                samples[i].resize(j);
            }

            valid_channels = 4;
        }

        return true;
    }

    int samples_per_channel = getSamplesPerChannel();
    int fft_size = getSamplesPerChannel() * 2;
    int samples_per_frame = getSamplesPerChannel() * getChannels();

    FFTWFComplexArray cache;

    AudioFile<float> ir_file;

    std::vector<std::vector<float>> samples;
    std::atomic<float> mix;
    int valid_channels;

    FFTWFPlan forward_plan, backward_plan;

    FFTWFComplexArray compute_cache_left;
    FFTWFComplexArray compute_cache_right;

    FFTWFComplexArray sliding_window_left;
    FFTWFComplexArray sliding_window_right;

    FFTWFComplexArray ir;
    int num_ir_blocks;

    FFTWFComplexArray delay_left;
    FFTWFComplexArray delay_right;
    int delay_head;
};

#endif
