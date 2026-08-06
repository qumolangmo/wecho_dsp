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

#ifndef __UTILS_H__
#define __UTILS_H__

class Utils {
private:
    static int sample_rate;
    static int samples_per_channel;
    static int channels;
public:
    static void setSampleRate(int);
    static void setSamplesPerChannel(int);
    static void setChannels(int);
    static int getSampleRate();
    static int getSamplesPerChannel();
    static int getChannels();
};

#endif