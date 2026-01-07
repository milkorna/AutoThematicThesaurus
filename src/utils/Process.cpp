#include "Process.h"
#include "Logger.h"
#include "MorphAnalyzer.h"
#include "WordComplex.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

Process::Process(const std::string& docId, const std::filesystem::path& outputFile, size_t sentNum)
    : docId(docId), sentNum(sentNum), sentenceOffsetInDocument(0), rawSentenceText(""), outputFile(outputFile),
      tokenPositions() {

    // Открываем существующий JSON или создаем новый
    if (std::filesystem::exists(outputFile)) {
        std::ifstream inFile(outputFile);
        if (inFile) {
            try {
                inFile >> jsonData; // Читаем JSON, если он есть
                Logger::log("Process", LogLevel::Info, "Loaded existing JSON file: " + outputFile.string());
            } catch (const json::exception& e) {
                Logger::log("Process", LogLevel::Error,
                            "Failed to parse JSON file: " + outputFile.string() + " (" + e.what() + ")");
                jsonData = json::array(); // Если ошибка, создаём новый массив
            }
        }
    } else {
        jsonData = json::array(); // Если файла нет, создаём пустой JSON
        Logger::log("Process", LogLevel::Debug, "Created new JSON file: " + outputFile.string());
    }
}

Process::~Process() {
    try {
        std::ofstream outFile(outputFile, std::ios::trunc | std::ios::binary);
        if (!outFile) {
            Logger::log("Process", LogLevel::Error, "Failed to open JSON file for writing: " + outputFile.string());
            return;
        }
        outFile << jsonData.dump(4) << std::endl;
        Logger::log("Process", LogLevel::Info, "Successfully saved JSON file: " + outputFile.string());
    } catch (const std::exception& e) {
        std::cerr << "Destructor error: " << e.what() << std::endl;
    }
}

void Process::setSentenceData(const std::string& sentence, const std::vector<std::pair<size_t, size_t>>& tokenSpans,
                              size_t sentenceOffset) {
    rawSentenceText = sentence;
    tokenPositions = tokenSpans;
    sentenceOffsetInDocument = sentenceOffset;

    Logger::log("Process", LogLevel::Debug,
                "Set sentence data: " + std::to_string(tokenPositions.size()) + " tokens, " +
                    std::to_string(rawSentenceText.length()) + " chars");
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
            key.pop_back();
        }

        // Build lemmas JSON array
        json lemmas_json = json::array();
        for (const auto& lemma : wc->lemmas) {
            lemmas_json.push_back(lemma);
        }

        // Calculate character span from token indices
        size_t charStart = 0;
        size_t charEnd = 0;

        if (wc->pos.start < tokenPositions.size()) {
            charStart = tokenPositions[wc->pos.start].first;
        }
        if (wc->pos.end < tokenPositions.size()) {
            charEnd = tokenPositions[wc->pos.end].second;
        }

        // Validate span (optional, for debugging)
        if (charStart > rawSentenceText.length() || charEnd > rawSentenceText.length()) {
            Logger::log("Process", LogLevel::Warning,
                        "Span out of bounds: [" + std::to_string(charStart) + ", " + std::to_string(charEnd) +
                            "] for text of length " + std::to_string(rawSentenceText.length()));
        }

        // Create result JSON object
        json j = json::object();
        j["key"] = key;
        j["textForm"] = wc->textForm;
        j["modelName"] = wc->modelName;
        j["docId"] = docId;
        j["sentNum"] = sentNum;
        j["start_token_ind"] = wc->pos.start;
        j["end_token_ind"] = wc->pos.end;
        j["span"] = json::array({charStart + sentenceOffsetInDocument, charEnd + sentenceOffsetInDocument});
        j["lemmas"] = lemmas_json;

        addJsonObject(j);
    }

    Logger::log("Process", LogLevel::Info, "Appended " + std::to_string(phrases.size()) + " results to JSON.");
}

void Process::nextSentence() {
    ++sentNum;
}

const std::string& Process::getDocId() const {
    return docId;
}

size_t Process::getSentNum() const {
    return sentNum;
}

const std::string& Process::getRawSentenceText() const {
    return rawSentenceText;
}

size_t Process::getSentenceOffsetInDocument() const {
    return sentenceOffsetInDocument;
}

void Process::addJsonObject(const json& newObj) {
    jsonData.push_back(newObj);
}