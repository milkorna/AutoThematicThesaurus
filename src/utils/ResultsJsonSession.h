#pragma once

#include "Logger.h"
#include "utils/PathUtils.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;

using util::path::extractNumberFromPath;

struct Process {
    fs::path inputFile;
    fs::path outputFile;
    json jsonData;
    size_t docNum;
    size_t sentNum;

    explicit Process(const fs::path& inputFile, const fs::path& outputFile, size_t sentNum = 0)
        : inputFile(inputFile), outputFile(outputFile), docNum(extractNumberFromPath(inputFile)), sentNum(sentNum) {

        // Открываем существующий JSON или создаем новый
        if (fs::exists(outputFile)) {
            std::ifstream inFile(outputFile);
            if (inFile) {
                try {
                    inFile >> jsonData; // Читаем JSON, если он есть
                    Logger::log("Process", LogLevel::Info, "Loaded existing JSON file: " + outputFile.string());
                } catch (...) {
                    Logger::log("Process", LogLevel::Error, "Failed to parse JSON file: " + outputFile.string());
                    jsonData = json::array(); // Если ошибка, создаём новый массив
                }
            }
        } else {
            jsonData = json::array(); // Если файла нет, создаём пустой JSON
            Logger::log("Process", LogLevel::Debug, "Created new JSON file: " + outputFile.string());
        }
    }

    void addJsonObject(const json& newObj) {
        jsonData.push_back(newObj);
    }

    ~Process() {
        std::ofstream outFile(outputFile, std::ios::trunc | std::ios::binary);
        if (!outFile) {
            Logger::log("Process", LogLevel::Error, "Failed to open JSON file for writing: " + outputFile.string());
            return;
        }
        outFile << jsonData.dump(4) << std::endl;
        Logger::log("Process", LogLevel::Info, "Successfully saved JSON file: " + outputFile.string());
    }
};
