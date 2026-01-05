#include "Options.h"
#include "Logger.h"

#include <fstream>
#include <nlohmann/json.hpp>

Options::Options() {
    fs::path repoPath = fs::current_path();

    dataDir = repoPath / "data";
    corpusDir = dataDir / "raw" / "ruTermEval";
    rawDataFile = corpusDir / "RuTermEval_processed.json";
    patternsFile = dataDir / "resources" / "patterns.json";
    stopWordsFile = dataDir / "resources" / "stop_words";
    resDir = corpusDir / "results";
    corpusFile = corpusDir / "corpus";
    filteredCorpusFile = corpusDir / "filtered_corpus";
    sentencesFile = corpusDir / "sentences.json";
    embeddingModelFile = repoPath / "my_custom_fasttext_model_finetuned.bin";
    totalResultsPath = corpusDir / "total_results.json";
    termsCandidatesPath = corpusDir / "term_candidates.json";

    totalDocuments = 0;
    thresholdTopicsCount = 7;
    cleanStopWords = true;
    validateBoundaries = true;
    mergeDocumentTitleAndText = true;
    topicsThreshold = 0.6;
    topicsHyponymThreshold = 0.98;
    freqThresholdCoeff = 0.12;
}

void Options::setCorpusDir(const fs::path& newCorpusDir) {
    corpusDir = newCorpusDir;
    rawDataFile = corpusDir / "raw_data.json";
    resDir = corpusDir / "results";
    corpusFile = corpusDir / "corpus";
    sentencesFile = corpusDir / "sentences.json";
    totalResultsPath = corpusDir / "total_results.json";
    termsCandidatesPath = corpusDir / "term_candidates.json";
}

void Options::updateDocumentCount() {
    try {
        // Путь к файлу корпуса с метаданными
        if (!fs::exists(rawDataFile)) {
            Logger::log("Options", LogLevel::Error,
                        "Corpus JSON file not found: " + rawDataFile.string() + ". Exiting.");
            Logger::flushLogs();
            std::exit(EXIT_FAILURE);
        }

        // Парсим JSON файл
        std::ifstream file(rawDataFile);
        if (!file.is_open()) {
            Logger::log("Options", LogLevel::Error,
                        "Failed to open corpus file: " + rawDataFile.string() + ". Exiting.");
            Logger::flushLogs();
            std::exit(EXIT_FAILURE);
        }

        nlohmann::json data = nlohmann::json::parse(file);
        file.close();

        // Извлекаем metadata.total_documents
        if (!data.contains("metadata") || !data["metadata"].is_object()) {
            Logger::log("Options", LogLevel::Error, "Corpus JSON does not contain 'metadata' object. Exiting.");
            Logger::flushLogs();
            std::exit(EXIT_FAILURE);
        }

        if (!data["metadata"].contains("total_documents")) {
            Logger::log("Options", LogLevel::Error, "Metadata does not contain 'total_documents' field. Exiting.");
            Logger::flushLogs();
            std::exit(EXIT_FAILURE);
        }

        totalDocuments = data["metadata"]["total_documents"].get<int>();

        Logger::log("Options", LogLevel::Info,
                    "Loaded " + std::to_string(totalDocuments) + " documents from corpus metadata.");

        if (totalDocuments == 0) {
            Logger::log("Options", LogLevel::Error, "No documents found in corpus metadata. Exiting");
            Logger::flushLogs();
            std::exit(EXIT_FAILURE);
        }

    } catch (const nlohmann::json::parse_error& e) {
        Logger::log("Options", LogLevel::Error,
                    std::string("JSON parse error in corpus file: ") + e.what() + ". Exiting.");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    } catch (const std::exception& ex) {
        Logger::log("Options", LogLevel::Error,
                    std::string("Failed to read corpus metadata: ") + ex.what() + ". Exiting.");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    } catch (...) {
        Logger::log("Options", LogLevel::Error, "Unknown error while reading corpus metadata. Exiting.");
        Logger::flushLogs();
        std::exit(EXIT_FAILURE);
    }
}