#include "GrammarPatternManager.h"
#include "Logger.h"
#include "Options.h"
#include "RawDataLoader.h"
#include "RawTextProcessor.h"

#include <filesystem>
#include <fstream>
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
        options.corpusFile = testOutputDir / "corpus";

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
 */
TEST_F(RawTextProcessorTest, BasicPipelineWithRawDataLoader) {
    // Load documents using RawDataLoader
    auto& options = Options::getOptions();
    std::vector<Document> documents = RawDataLoader::loadFromJson(options.rawDataFile);

    ASSERT_GT(documents.size(), 0) << "RawDataLoader should load at least one document";

    Logger::log("Test", LogLevel::Info, "Loaded " + std::to_string(documents.size()) + " documents from test file");

    // Process documents
    auto& processor = RawTextProcessor::GetProcessor();
    processor.processRawData(documents);

    // Verify output files were created for all documents
    for (const auto& doc : documents) {
        EXPECT_TRUE(OutputFileExists(doc.getDocId())) << "Output file for " << doc.getDocId() << " should exist";
    }

    Logger::log("Test", LogLevel::Info, "Pipeline execution completed successfully");
}