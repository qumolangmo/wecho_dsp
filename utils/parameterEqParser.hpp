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

#ifndef __PARAMETER_EQ_PARSER_HPP__
#define __PARAMETER_EQ_PARSER_HPP__

#include "utils.h"
#include <vector>
#include <string>
#include "debug.hpp"

class ParameterEqParser: public Utils {
public:
    enum class FilterType {
        LSC,
        HSC,
        PK
    };

    struct FilterParam {
        bool enabled;
        FilterType type;
        float fc;
        float gain;
        float q;
    };

    ParameterEqParser() = default;
    ~ParameterEqParser() = default;

    float getPreamp() const {
        return preamp;
    }

    auto parse(std::string_view param) -> const std::vector<FilterParam>& {
        data.clear();

        if (param.empty()) {
            return data;
        }

        auto lines = split(param, '\n');
        auto preamp_ref = split(lines[0], ' ')[1];
        preamp = std::stof(std::string(preamp_ref));

        for (int i = 1; i < lines.size(); i++) {
            auto item = split(lines[i], ' ');

            bool enabled = item[2] == "ON" ? true : false;
            FilterType type;
            if (item[3] == "PK") {
                type = FilterType::PK;
            } else if (item[3] == "LSC") {
                type = FilterType::LSC;
            } else if (item[3] == "HSC") {
                type = FilterType::HSC;
            } else {
                LOG_D("invalid filter type: %s, force PK", std::string(item[3]).c_str());
                type = FilterType::PK;
            }

            float fc = std::stof(std::string(item[5]));
            float gain = std::stof(std::string(item[8]));
            float q = std::stof(std::string(item[11]));

            data.emplace_back(FilterParam{enabled, type, fc, gain, q});
        }

        return data;
    }

    void reset() {
        data.clear();
    }

private:
    std::vector<FilterParam> data;
    float preamp;

    static inline std::string_view trim(std::string_view str) {
        auto start = str.find_first_not_of(" \t\n\r");
        if (start == std::string_view::npos) {
            return "";
        }
        auto end = str.find_last_not_of(" \t\n\r");

        return str.substr(start, end - start + 1);
    }

    static inline std::vector<std::string_view> split(std::string_view str, char delimiter) {
        std::vector<std::string_view> result;
        int start = 0, end = 0;

        while ((end = str.find(delimiter, start)) != std::string_view::npos) {
            auto item = trim(str.substr(start, end - start));

            if (!item.empty()) {
                result.push_back(item);
            }

            start = end + 1;
        }

        auto last = trim(str.substr(start));
        if (!last.empty()) {
            result.push_back(last);
        }

        return result;
    }
};
#endif