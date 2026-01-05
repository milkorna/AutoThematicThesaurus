#pragma once

#include "Logger.h"
#include "utils/PathUtils.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

struct Process {
    std::string docId;
    fs::path outputFile;
    nlohmann::json jsonData;
    size_t sentNum;

    explicit Process(const std::string& docId, const fs::path& outputFile, size_t sentNum = 0);
    ~Process();

    void addJsonObject(const nlohmann::json& newObj);
};