#ifndef __FREQUENCY_RESPONSE_READER_HPP__
#define __FREQUENCY_RESPONSE_READER_HPP__

#include <string_view>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

#include "utils.h"
#include "parameterEqParser.hpp"

class FrequencyResponseReader: public Utils {
public:
    FrequencyResponseReader() = default;
    ~FrequencyResponseReader() = default;

#ifdef __ANDROID__
    static void setAssetManager(AAssetManager* manager) {
        asset_manager = manager;
    }
#endif

    auto get(const std::string& header) -> const std::vector<float>& {
        return data[header];
    }

    auto getHeaders() -> const std::vector<std::string>& {
        return headers;
    }

    void load(std::string_view path, float clamp = 9.0f) {
        readCsv(path, clamp);
    }

private:
#ifdef __ANDROID__
    inline static AAssetManager* asset_manager = nullptr;
#endif

    void readCsv(std::string_view path, float clamp = 9.0f) {
        headers.clear();
        data.clear();

        std::stringstream ss;

        if (!readIntoStream(std::string(path), ss)) {
            return;
        }

        std::string line;
        std::vector<std::string_view> line_data;

        std::getline(ss, line);

        if (line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3);
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line_data = ParameterEqParser::split(line, ',');
        for (auto item: line_data) {
            headers.push_back(std::string(item));
        }

        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            line_data.clear();
            line_data = ParameterEqParser::split(line, ',');

            if (line_data.size() != headers.size()) {
                continue;
            }

            for (int i = 0; i < (int)headers.size(); i++) {
                float v = std::strtof(std::string(line_data[i]).c_str(), nullptr);
                if (headers[i] != "frequency") {
                    v = std::clamp(v, -clamp, clamp);
                }
                data[headers[i]].emplace_back(v);
            }
        }
    }

    bool readIntoStream(const std::string& path, std::stringstream& out) {
#ifdef __ANDROID__
        if (asset_manager != nullptr) {
            AAsset* asset = AAssetManager_open(asset_manager, path.c_str(), AASSET_MODE_BUFFER);

            if (asset != nullptr) {
                off_t len = AAsset_getLength(asset);
                std::vector<char> buf(len > 0 ? len : 1);
                int n = AAsset_read(asset, buf.data(), buf.size());
                AAsset_close(asset);

                if (n > 0) {
                    out.write(buf.data(), n);
                    return true;
                }

                return false;
            }
        }
#endif
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }
        out << file.rdbuf();
        return true;
    }

    std::unordered_map<std::string, std::vector<float>> data;
    std::vector<std::string> headers;
};
#endif
