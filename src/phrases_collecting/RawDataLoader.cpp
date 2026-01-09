#include "RawDataLoader.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

// Не используем alias, чтобы избежать проблем с типами шаблонов

std::vector<Document> RawDataLoader::loadFromJson(const fs::path& jsonFile) {
    std::vector<Document> documents;
    if (!validateFilePath(jsonFile)) {
        return documents;
    }

    std::ifstream file(jsonFile);
    if (!file.is_open()) {
        Logger::log("RawDataLoader", LogLevel::Error, "Failed to open JSON file: " + jsonFile.string());
        return documents;
    }

    try {
        // Read entire file as string
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // Parse and extract documents
        documents = parseDocumentsArray(fileContent, jsonFile);
        Logger::log("RawDataLoader", LogLevel::Info,
                    "Successfully loaded " + std::to_string(documents.size()) +
                        " documents from: " + jsonFile.filename().string());

    } catch (const nlohmann::json::parse_error& e) {
        Logger::log("RawDataLoader", LogLevel::Error, "JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        Logger::log("RawDataLoader", LogLevel::Error, "Error loading JSON: " + std::string(e.what()));
    }

    return documents;
}

bool RawDataLoader::validateFilePath(const fs::path& filePath) {
    if (filePath.empty()) {
        Logger::log("RawDataLoader", LogLevel::Error, "File path is empty");
        return false;
    }

    if (!fs::exists(filePath)) {
        Logger::log("RawDataLoader", LogLevel::Error, "File does not exist: " + filePath.string());
        return false;
    }

    if (!fs::is_regular_file(filePath)) {
        Logger::log("RawDataLoader", LogLevel::Error, "Path is not a regular file: " + filePath.string());
        return false;
    }

    return true;
}

std::vector<Document> RawDataLoader::parseDocumentsArray(const std::string& jsonStr, const fs::path& sourceFilePath) {
    std::vector<Document> documents;
    try {
        // Явно парсим в стандартный тип nlohmann::json
        nlohmann::json data = nlohmann::json::parse(jsonStr);

        // Validate that "documents" field exists and is an array
        if (!data.contains("documents") || !data["documents"].is_array()) {
            Logger::log("RawDataLoader", LogLevel::Warning, "JSON does not contain 'documents' array");
            return documents;
        }

        const auto& docsArray = data["documents"];
        std::string currentTimestamp = getCurrentTimestamp();
        std::string sourceFileName = sourceFilePath.filename().string();

        for (size_t i = 0; i < docsArray.size(); ++i) {
            try {
                const auto& docJson = docsArray[i];
                Document doc;

                // Используем get() для явного преобразования типов
                doc.doc_id = docJson.value("doc_id", "");
                doc.title = docJson.value("title", "");
                doc.text = docJson.value("text", "");

                doc.filename = sourceFileName;
                doc.processing_timestamp = currentTimestamp;

                doc.stats = Document::DocumentStats();
                doc.word_frequency_local.clear();
                doc.document_lemmas.clear();

                // Validate document
                if (!validateDocument(doc, i)) {
                    continue;
                }

                documents.push_back(doc);

            } catch (const std::exception& e) {
                Logger::log("RawDataLoader", LogLevel::Warning,
                            "Error parsing document at index " + std::to_string(i) + ": " + std::string(e.what()));
                continue;
            }
        }

    } catch (const nlohmann::json::parse_error& e) {
        Logger::log("RawDataLoader", LogLevel::Error,
                    "JSON parse error in ParseDocumentsArray: " + std::string(e.what()));
    } catch (const std::exception& e) {
        Logger::log("RawDataLoader", LogLevel::Error, "Error in ParseDocumentsArray: " + std::string(e.what()));
    }

    return documents;
}

bool RawDataLoader::validateDocument(const Document& doc, size_t index) {
    // Validate doc_id
    if (doc.doc_id.empty()) {
        Logger::log("RawDataLoader", LogLevel::Warning,
                    "Document at index " + std::to_string(index) + " has empty doc_id, skipping");
        return false;
    }

    // Validate that at least one of title or text is non-empty
    if (doc.title.empty() && doc.text.empty()) {
        Logger::log("RawDataLoader", LogLevel::Warning,
                    "Document '" + doc.doc_id + "' (index " + std::to_string(index) +
                        ") has empty title and text, skipping");
        return false;
    }

    return true;
}

std::string RawDataLoader::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    char buffer[30];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", std::gmtime(&time));

    std::string timestamp(buffer);
    timestamp += ".";

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3) << ms.count();
    timestamp += ss.str() + "Z";

    return timestamp;
}