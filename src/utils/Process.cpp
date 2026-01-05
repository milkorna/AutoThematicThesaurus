#include "Process.h"
#include "Logger.h"
#include "MorphAnalyzer.h"
#include "WordComplex.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Process::Process(const std::string& docId, const std::filesystem::path& outputFile, size_t sentNum)
    : docId(docId), outputFile(outputFile), sentNum(sentNum) {

    // Открываем существующий JSON или создаем новый
    if (std::filesystem::exists(outputFile)) {
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

Process::~Process() {
    std::ofstream outFile(outputFile, std::ios::trunc | std::ios::binary);
    if (!outFile) {
        Logger::log("Process", LogLevel::Error, "Failed to open JSON file for writing: " + outputFile.string());
        return;
    }
    outFile << jsonData.dump(4) << std::endl;
    Logger::log("Process", LogLevel::Info, "Successfully saved JSON file: " + outputFile.string());
}

void Process::outputResults(const std::vector<WordComplexPtr>& phrases) {
    if (phrases.empty()) {
        return;
    }

    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (const auto& wc : phrases) {
        // Build lemma key from words
        std::string key;
        for (const auto& w : wc->words) {
            key.append(morphAnalyzer.getLemma(w) + " ");
        }
        if (!key.empty()) {
            key.pop_back(); // Remove trailing space
        }

        // Build lemmas JSON array with indexed format
        json lemmas_json = json::array();
        for (size_t i = 0; i < wc->lemmas.size(); ++i) {
            lemmas_json.push_back(std::to_string(i) + "_" + wc->lemmas[i]);
        }

        // Create result JSON object with ordered keys
        json j = json::object();
        j["0_key"] = key;
        j["1_textForm"] = wc->textForm;
        j["2_modelName"] = wc->modelName;
        j["3_docId"] = docId;
        j["4_sentNum"] = sentNum;
        j["5_start_ind"] = wc->pos.start;
        j["6_end_ind"] = wc->pos.end;
        j["7_lemmas"] = lemmas_json;

        addJsonObject(j);
    }

    Logger::log("Process", LogLevel::Info, "Appended " + std::to_string(phrases.size()) + " results to JSON.");
}

void Process::addJsonObject(const json& newObj) {
    jsonData.push_back(newObj);
}