#include <Component.h>
#include <Embedding.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>
#include <SemanticRelations.h>
#include <TextCorpus.h>

#include <chrono>
#include <filesystem>

#include <sys/stat.h>

namespace fs = std::filesystem;

int main()
{
    auto start = std::chrono::high_resolution_clock::now();

    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);

    fs::path jsonFilePath = repoPath / "my_data" / "total_results_no_sw.json";

    // collecting phrases for each text and saving phrases for each text in a separate file
    // also collecting a corpus of texts and saving it
    // {
    //     fs::path patternsPath = repoPath / "my_data/patterns.txt";
    //     GrammarPatternManager::GetManager()->readPatterns(patternsPath);
    //     BuildPhraseStorage();
    // }

    // the corpus of texts is filtered during diserialization, now the result is saved and the corpus is parsed filtered

    // steps to combine clusters mistakenly separated due to inaccurate morphological analysis, TF-IDF counting and
    // topic matching. the result is saved so that it is not recalculated every time
    // {
    //     auto& corpus = TextCorpus::GetCorpus();
    //     corpus.LoadCorpusFromFile((repoPath / "my_data" / "filtered_corpus").string());
    //     auto& storage = PatternPhrasesStorage::GetStorage();
    //     storage.LoadPhraseStorageFromResultsDir();
    //     storage.MergeSimilarClusters();
    //     storage.ComputeTextMetrics();
    //     storage.OutputClustersToJsonFile(jsonFilePath.string());
    // }

    // load hyperonyms and hyponyms for lemmas from WikiWordNet
    // {
    //     auto& corpus = TextCorpus::GetCorpus();
    //     corpus.LoadCorpusFromFile((repoPath / "my_data" / "filtered_corpus").string());
    //     auto& storage = PatternPhrasesStorage::GetStorage();
    //     storage.LoadStorageFromFile(jsonFilePath.string());
    //     storage.LoadWikiWNRelations();
    //     storage.OutputClustersToJsonFile(jsonFilePath);
    // }

    // getting ready-made results without waiting for intermediate steps
    {
        auto& corpus = TextCorpus::GetCorpus();
        corpus.LoadCorpusFromFile((repoPath / "my_data" / "filtered_corpus").string());
        auto& storage = PatternPhrasesStorage::GetStorage();
        storage.LoadStorageFromFile(jsonFilePath.string());
        storage.OutputClustersToJsonFile(jsonFilePath);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Processing took " + std::to_string(duration.count()) + "seconds.";

    return 0;
}
