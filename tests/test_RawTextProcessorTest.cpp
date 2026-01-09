#include "CorpusVocabulary.h"
#include "GrammarPatternManager.h"
#include "Logger.h"
#include "Options.h"
#include "RawDataLoader.h"
#include "RawTextProcessor.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace fs = std::filesystem;

/**
 * @class RawTextProcessorTest
 * @brief Test suite for RawTextProcessor phrase collection pipeline
 * @details Tests the complete workflow using RawDataLoader and RawTextProcessor
 */
class RawTextProcessorTest : public ::testing::Test {
  protected:
    /**
     * @brief Set up test fixtures before each test
     */
    void SetUp() override {
        // Initialize logging for tests
        Logger::enableLogging(true);
        Logger::setGlobalLogLevel(LogLevel::Info);

        fs::path testDataDir = fs::current_path() / "tests" / "data";
        testOutputDir = testDataDir / "rtp_test_output";
        fs::create_directories(testOutputDir);

        std::string logFilePath = (testOutputDir / "collect_phrases_test_logs.txt").string();
        Logger::initializeLogFile(logFilePath);

        // Initialize Options singleton with test paths
        auto& options = Options::getOptions();

        options.setCorpusDir(testDataDir);
        options.rawDataFile = testDataDir / "test_file.json";
        options.resDir = testOutputDir / "results";
        options.corpusFile = testOutputDir / "corpus.json";

        options.updateDocumentCount();

        fs::path patternsPath = options.patternsFile;
        auto& patternManager = GrammarPatternManager::GetManager();
        patternManager.readPatterns(patternsPath);
    }

    /**
     * @brief Tear down test fixtures after each test
     */
    void TearDown() override {
        Logger::flushLogs();

        CorpusVocabulary::GetCorpus().clear();

        // Clean up test output directory
        // if (fs::exists(testOutputDir)) {
        //     fs::remove_all(testOutputDir);
        // }
    }

    /**
     * @brief Helper to check if output file exists for a document
     */
    bool OutputFileExists(const std::string& docId) {
        auto& options = Options::getOptions();
        fs::path outputFile = options.resDir / (docId + "_res.json");
        return fs::exists(outputFile);
    }

  protected:
    fs::path testOutputDir;
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @brief Test: Basic pipeline execution with RawDataLoader and RawTextProcessor
 * @details This is the main integration test that verifies:
 *   - Documents are loaded by RawDataLoader
 *   - Documents are processed by RawTextProcessor
 *   - Output files are created with correct naming
 *   - Corpus vocabulary file is created
 */
TEST_F(RawTextProcessorTest, BasicPipelineWithRawDataLoader) {
    // Load documents using RawDataLoader
    auto& options = Options::getOptions();
    std::vector<Document> documents = RawDataLoader::loadFromJson(options.rawDataFile);

    ASSERT_GT(documents.size(), 0) << "RawDataLoader should load at least one document";

    // Process documents
    auto& processor = RawTextProcessor::GetProcessor();
    processor.processRawData(documents);

    // Verify output files were created for all documents
    for (const auto& doc : documents) {
        EXPECT_TRUE(OutputFileExists(doc.getDocId())) << "Output file for " << doc.getDocId() << " should exist";
    }

    // Verify corpus vocabulary file exists
    fs::path corpusFile = options.corpusFile;
    EXPECT_TRUE(fs::exists(corpusFile)) << "Corpus vocabulary should exist at: " << corpusFile.string();
}

/**
 * @brief Test: Verify corpus.json is created with correct statistics
 * @details Checks that:
 *   - corpus.json file exists
 *   - corpus_metadata contains total_documents > 0
 *   - corpus_metadata contains total_words > 0
 *   - global_statistics has word_frequency map (not empty)
 *   - global_statistics has document_frequency map (not empty)
 *   - word_frequency and document_frequency have same lemmas
 */
TEST_F(RawTextProcessorTest, CorpusJsonCreatedWithCorrectStatistics) {
    // Load and process documents
    auto& options = Options::getOptions();
    std::vector<Document> documents = RawDataLoader::loadFromJson(options.rawDataFile);

    ASSERT_GT(documents.size(), 0) << "Should have documents to process";

    auto& processor = RawTextProcessor::GetProcessor();
    processor.processRawData(documents);

    // Verify corpus.json exists
    fs::path corpusFile = options.corpusFile;

    // Try both with and without .json extension
    if (!fs::exists(corpusFile)) {
        corpusFile.replace_extension(".json");
    }

    EXPECT_TRUE(fs::exists(corpusFile)) << "corpus.json should exist at: " << corpusFile.string();

    // Parse and verify JSON structure
    try {
        std::ifstream file(corpusFile);
        ASSERT_TRUE(file.is_open()) << "Failed to open corpus.json for reading";

        nlohmann::ordered_json j;
        file >> j;
        file.close();

        // ═══════════════════════════════════════════════════════════════
        // Check corpus_metadata
        // ═══════════════════════════════════════════════════════════════
        EXPECT_TRUE(j.contains("corpus_metadata")) << "JSON should contain 'corpus_metadata'";

        auto metadata = j.at("corpus_metadata");
        EXPECT_TRUE(metadata.contains("total_documents")) << "Metadata should contain 'total_documents'";
        EXPECT_TRUE(metadata.contains("total_words")) << "Metadata should contain 'total_words'";

        size_t totalDocuments = metadata.at("total_documents").get<size_t>();
        size_t totalWords = metadata.at("total_words").get<size_t>();

        EXPECT_GT(totalDocuments, 0) << "total_documents should be > 0, got: " << totalDocuments
                                     << " (THIS IS THE BUG - incrementDocumentCount() not called!)";
        EXPECT_GT(totalWords, 0) << "total_words should be > 0, got: " << totalWords;

        // ═══════════════════════════════════════════════════════════════
        // Check global_statistics
        // ═══════════════════════════════════════════════════════════════
        EXPECT_TRUE(j.contains("global_statistics")) << "JSON should contain 'global_statistics'";

        auto stats = j.at("global_statistics");
        EXPECT_TRUE(stats.contains("word_frequency")) << "Statistics should contain 'word_frequency'";
        EXPECT_TRUE(stats.contains("document_frequency")) << "Statistics should contain 'document_frequency'";

        auto wordFreq = stats.at("word_frequency");
        auto docFreq = stats.at("document_frequency");

        EXPECT_GT(wordFreq.size(), 0) << "word_frequency map should not be empty";
        EXPECT_GT(docFreq.size(), 0) << "document_frequency map should not be empty (THIS IS THE BUG!)";

        // ═══════════════════════════════════════════════════════════════
        // Check that word and document frequency have same lemmas
        // ═══════════════════════════════════════════════════════════════
        EXPECT_EQ(wordFreq.size(), docFreq.size()) << "word_frequency and document_frequency should have same lemmas";

        // Check first few entries are sorted alphabetically
        std::vector<std::string> lemmas;
        for (const auto& [lemma, count] : wordFreq.items()) {
            lemmas.push_back(lemma);
            if (lemmas.size() >= 5)
                break;
        }

        EXPECT_TRUE(std::is_sorted(lemmas.begin(), lemmas.end())) << "Lemmas should be sorted alphabetically";

        // ═══════════════════════════════════════════════════════════════
        // Logging results
        // ═══════════════════════════════════════════════════════════════
        Logger::log("Test", LogLevel::Info, "✓ Corpus statistics verified successfully!");
        Logger::log("Test", LogLevel::Info, "  total_documents: " + std::to_string(totalDocuments));
        Logger::log("Test", LogLevel::Info, "  total_words: " + std::to_string(totalWords));
        Logger::log("Test", LogLevel::Info, "  unique_lemmas: " + std::to_string(wordFreq.size()));
        Logger::log("Test", LogLevel::Info, "  document_frequencies: " + std::to_string(docFreq.size()));

    } catch (const std::exception& e) {
        FAIL() << "Failed to parse corpus.json: " << e.what();
    }
}