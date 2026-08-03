#ifndef __RMS_HPP__
#define __RMS_HPP__
#include <vector>

class RMS {
private:
    static constexpr int SAMPLE_RATE = 48000;
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
        buffer.reserve(SAMPLE_RATE / 10);
    }

    ~RMS() = default;

    void setWindowMs(int ms) {
        window_ms = ms;
        buffer.resize(SAMPLE_RATE / 1000 * ms);
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        rms_idx = 0;
    }

    void setAttackMs(int ms) {
        attack_ms = ms;
        attack_coeff = 1.0f - std::exp(-1.0f / (SAMPLE_RATE / 1000.0f * ms));
    }

    void setReleaseMs(int ms) {
        release_ms = ms;
        release_coeff = 1.0f - std::exp(-1.0f / (SAMPLE_RATE / 1000.0f * ms));
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
