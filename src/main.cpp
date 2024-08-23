#include <Component.h>
#include <Embedding.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>
#include <SemanticRelations.h>
#include <TextCorpus.h>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);

    Embedding e;

    fs::path patternsPath = repoPath / "my_data/patterns.txt";
    GrammarPatternManager::GetManager()->readPatterns(patternsPath);

    auto start = std::chrono::high_resolution_clock::now();
    BuildPhraseStorage();
    auto end = std::chrono::high_resolution_clock::now();

    fs::path totalResultsFile;
    if (g_options.cleaningStopWords) {
        totalResultsFile = repoPath / "my_data/total_results_no_sw";
    } else {
        totalResultsFile = repoPath / "my_data/total_results_sw";
    }

    const auto& corpus = TextCorpus::LoadCorpusFromFile((repoPath / "my_data" / "corpusDict.txt").string());
    auto& storage = PatternPhrasesStorage::GetStorage();
    storage.LoadPhraseStorage();
    fs::path textFilePath = totalResultsFile;
    textFilePath.replace_extension(".txt");

    fs::path jsonFilePath = totalResultsFile;
    jsonFilePath.replace_extension(".json");

    storage.ComputeTextMetrics();
    storage.OutputClustersToJsonFile(jsonFilePath);

    std::chrono::duration<double> duration = end - start;
    Logger::log("\n\nmain", LogLevel::Info, "Processing texts took " + std::to_string(duration.count()) + "seconds.");

    return 0;
}
