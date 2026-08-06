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

#ifndef __AUDIO_STREAM_H__
#define __AUDIO_STREAM_H__

#include "../utils/crossFader.hpp"
#include "utils.h"

template<typename T>
concept HasBufferType = requires(T& t, std::span<float> audio) {
    {T::bufferType()};
    {t.isEnabled()};
    {t.run(audio)};
};

class AudioStream: public Utils {
private:
    int samples_per_frame = getSamplesPerChannel() * getChannels();
    int samples_per_channel = getSamplesPerChannel();

    std::vector<float> audio, temp;

    BufferType current_buffer_type;

public:
    AudioStream()
        : current_buffer_type(BufferType::INTERLEAVED)
        , audio(samples_per_frame)
        , temp(samples_per_frame) {}
    
    template<typename T>
    AudioStream& operator>>(T& next) {
        if (!next.isEnabled()) {
            return *this;
        }

        checkAndConvertBufferType(next);

        next.run(audio);

        return *this;
    }

    void operator>>(float *output) {
        if (current_buffer_type == BufferType::INTERLEAVED) {
            memcpy(output, audio.data(), sizeof(float) * samples_per_frame);
        } else {
            for (int i = 0; i < samples_per_channel; i++) {
                output[i * 2] = audio[i];
                output[i * 2 + 1] = audio[i + samples_per_channel];
            }
        }
    }

    void operator<<(float *input) {
        if (current_buffer_type == BufferType::INTERLEAVED) {
            memcpy(audio.data(), input, sizeof(float) * samples_per_frame);
        } else {
            for (int i = 0; i < samples_per_channel; i++) {
                audio[i] = input[i * 2] * 0.8f;
                audio[i + samples_per_channel] = input[i * 2 + 1] * 0.8f;
            }
        }
    }

    template<typename T>
    AudioStream& operator>>(CrossFader<T>& cross_fader) {
        if (!cross_fader.isEnabled()) {
            return *this;
        }

        checkAndConvertBufferType(cross_fader);

        cross_fader.run(audio);

        return *this;
    }



    template<HasBufferType T>
    void checkAndConvertBufferType(T& next) {
        if (current_buffer_type == T::bufferType()) {
            return;
        }

        temp = audio;

        if (current_buffer_type != BufferType::INTERLEAVED) {
            current_buffer_type = BufferType::INTERLEAVED;

            for (int i = 0; i < samples_per_channel; i++) {
                audio[i * 2] = temp[i];
                audio[i * 2 + 1] = temp[i + samples_per_channel];
            }
        } else {
            current_buffer_type = BufferType::PLANAR;

            for (int i = 0; i < samples_per_channel; i++) {
                audio[i] = temp[i * 2];
                audio[i + samples_per_channel] = temp[i * 2 + 1];
            }
        }
    }
};
#endif