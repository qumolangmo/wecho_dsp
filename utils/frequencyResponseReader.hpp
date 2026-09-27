#ifndef __FREQUENCY_RESPONSE_READER_HPP__
#define __FREQUENCY_RESPONSE_READER_HPP__

#include <string_view>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <bit>
#include <cstdint>

#include "utils.h"
#include "debug.hpp"

#ifdef __ANDROID__
extern "C" {
extern const unsigned char autoeq_data_start[];
extern const unsigned char autoeq_data_end[];
}
#else
inline const unsigned char* autoeq_data_start = nullptr;
inline const unsigned char* autoeq_data_end = nullptr;

inline void autoeq_bind_resource(const void* start, const void* end) {
    autoeq_data_start = static_cast<const unsigned char*>(start);
    autoeq_data_end = static_cast<const unsigned char*>(end);
}
#endif

class FrequencyResponseReader: public Utils {
public:
    FrequencyResponseReader() = default;
    ~FrequencyResponseReader() = default;

    auto get(const std::string& header) -> const std::vector<float>& {
        return data[header];
    }

    auto getHeaders() -> const std::vector<std::string>& {
        return headers;
    }

    void load(std::string_view spec, float clamp = 9.0f) {
        headers.clear();
        data.clear();

        if (spec.rfind("autoeq@", 0) != 0) {
            return;
        }

        auto at = spec.find('@');
        auto colon = spec.find(':');
        if (at == std::string_view::npos || colon == std::string_view::npos || colon <= at) {
            return;
        }

        auto row_bytes = 3 * sizeof(uint16_t);
        size_t offset = static_cast<size_t>(std::strtoull(std::string(spec.substr(at + 1, colon - at - 1)).c_str(), nullptr, 10));
        size_t rows = static_cast<size_t>(std::strtoull(std::string(spec.substr(colon + 1)).c_str(), nullptr, 10));
        size_t total = static_cast<size_t>(autoeq_data_end - autoeq_data_start);

        if (offset + rows * row_bytes > total) {
            LOG_E("autoeq range out of bounds: offset=%zu rows=%zu total=%zu", offset, rows, total);
            return;
        }

        headers = {"frequency", "equalization", "error"};
        auto& freq = data["frequency"];
        auto& eq = data["equalization"];
        auto& err = data["error"];
        freq.reserve(rows);
        eq.reserve(rows);
        err.reserve(rows);

        const auto* it = reinterpret_cast<const uint16_t*>(autoeq_data_start + offset);
        for (size_t i = 0; i < rows; i++) {
            freq.emplace_back(halfBitsToFloat(*it++));
            eq.emplace_back(std::clamp(halfBitsToFloat(*it++), -clamp, clamp));
            err.emplace_back(std::clamp(halfBitsToFloat(*it++), -clamp, clamp));
        }
    }

    /* IEEE 754 binary16 bit pattern -> float. */
    static float halfBitsToFloat(uint16_t h) {
        const uint32_t sign = static_cast<uint32_t>(h & 0x8000u) << 16;
        const uint32_t exponent = (h >> 10) & 0x1Fu;
        uint32_t mantissa = h & 0x03FFu;

        uint32_t bits;
        if (exponent == 0) {
            if (mantissa == 0) {
                bits = sign;
            } else {
                uint32_t e = 127 - 15 + 1;
                while ((mantissa & 0x0400u) == 0) {
                    mantissa <<= 1;
                    e--;
                }
                mantissa &= 0x03FFu;
                bits = sign | (e << 23) | (mantissa << 13);
            }
        } else if (exponent == 0x1Fu) {
            bits = sign | 0x7F800000u | (mantissa << 13);
        } else {
            bits = sign | ((exponent + 112u) << 23) | (mantissa << 13);
        }
        return std::bit_cast<float>(bits);
    }

private:
    std::unordered_map<std::string, std::vector<float>> data;
    std::vector<std::string> headers;
};
#endif
