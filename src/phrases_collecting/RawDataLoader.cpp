#include "RawDataLoader.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

// Не используем alias, чтобы избежать проблем с типами шаблонов

std::vector<DocumentRecord> RawDataLoader::LoadFromJson(const fs::path& jsonFile) {
    std::vector<DocumentRecord> documents;
    if (!ValidateFilePath(jsonFile)) {
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
        documents = ParseDocumentsArray(fileContent);
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

bool RawDataLoader::ValidateFilePath(const fs::path& filePath) {
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

std::vector<DocumentRecord> RawDataLoader::ParseDocumentsArray(const std::string& jsonStr) {
    std::vector<DocumentRecord> documents;
    try {
        // Явно парсим в стандартный тип nlohmann::json
        nlohmann::json data = nlohmann::json::parse(jsonStr);

        // Validate that "documents" field exists and is an array
        if (!data.contains("documents") || !data["documents"].is_array()) {
            Logger::log("RawDataLoader", LogLevel::Warning, "JSON does not contain 'documents' array");
            return documents;
        }

        const auto& docsArray = data["documents"];
        for (size_t i = 0; i < docsArray.size(); ++i) {
            try {
                const auto& docJson = docsArray[i];
                DocumentRecord doc;

                // Используем get() для явного преобразования типов
                doc.doc_id = docJson.value("doc_id", "");
                doc.title = docJson.value("title", "");
                doc.text = docJson.value("text", "");

                // Validate document
                if (!ValidateDocument(doc, i)) {
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

bool RawDataLoader::ValidateDocument(const DocumentRecord& doc, size_t index) {
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