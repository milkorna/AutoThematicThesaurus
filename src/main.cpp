#include <Component.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>

#include <SemanticRelations.h>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

void printFirstFiveRows(const std::string& table_name)
{
}

int main()
{
    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);

    // std::string semantic_data = (repoPath / "wikiwordnet.db").string();
    // SemanticRelationsDB db(semantic_data);
    // DB::RunTest();

    fs::path patternsPath = repoPath / "my_data/patterns.txt";
    GrammarPatternManager::GetManager()->readPatterns(patternsPath);

    auto start = std::chrono::high_resolution_clock::now();
    BuildPhraseStorage();
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;
    Logger::log("\n\nmain", LogLevel::Info, "Processing texts took " + std::to_string(duration.count()) + "seconds.");

    return 0;
}
