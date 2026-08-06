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

#include "utils.h"

int Utils::sample_rate = 48000;
int Utils::samples_per_channel = 512;
int Utils::channels = 2;

void Utils::setSampleRate(int _sample_rate) {
    sample_rate = _sample_rate;
}

void Utils::setSamplesPerChannel(int _samples_per_channel) {
    samples_per_channel = _samples_per_channel;
}

void Utils::setChannels(int _channels) {
    channels = _channels;
}

int Utils::getChannels() {
    return channels;
}

int Utils::getSampleRate() {
    return sample_rate;
}

int Utils::getSamplesPerChannel() {
    return samples_per_channel;
}