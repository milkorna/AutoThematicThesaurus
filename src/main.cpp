#include <Component.h>
#include <Embedding.h>
#include <LSA.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>
#include <PhrasesStorageLoader.h>
#include <SemanticRelations.h>
#include <TextCorpus.h>
#include <TokenizedSentenceCorpus.h>

#include <chrono>
#include <filesystem>

#include <sys/stat.h>

namespace fs = std::filesystem;

int main()
{
    ::Embedding e;
    auto start = std::chrono::high_resolution_clock::now();

    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);
    fs::path jsonFilePath = repoPath / "my_data" / "total_results.json";
    fs::path jsonFileResPath = repoPath / "my_data" / "terms.json";

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

    // build and save a tokenized sentence corpus based on the available text data
    // this step tokenizes sentences and prepares them for LSA analysis
    // {
    //     BuildTokenizedSentenceCorpus();
    // }

    // load previously saved clusters and sentence data, perform LSA analysis, and update cluster metrics with topic
    // relevance and centrality score
    // {
    //     auto& storage = PatternPhrasesStorage::GetStorage();
    //     storage.LoadStorageFromFile(jsonFilePath.string());

    //     auto& sentences = TokenizedSentenceCorpus::GetCorpus();
    //     sentences.LoadFromFile((repoPath / "my_data" / "sentences.json").string());
    //     std::cout << "Sentences loaded successfully. Total sentences: " << sentences.totalSentences << std::endl;
    //     LSA lsa(sentences);
    //     std::cout << "Starting LSA analysis..." << std::endl;
    //     lsa.PerformAnalysis(false);
    //     MatrixXd U = lsa.GetU();
    //     MatrixXd Sigma = lsa.GetSigma();
    //     MatrixXd V = lsa.GetV();
    //     std::vector<std::string> words = lsa.GetWords();

    //     std::cout << "LSA analysis completed successfully!" << std::endl;
    //     std::cout << "Matrix U size: " << U.rows() << "x" << U.cols() << std::endl;
    //     std::cout << "Matrix Sigma size: " << Sigma.rows() << "x" << Sigma.cols() << std::endl;
    //     std::cout << "Matrix V size: " << V.rows() << "x" << V.cols() << std::endl;

    //     std::cout << "\nAnalyzing top topics..." << std::endl;
    //     lsa.AnalyzeTopics(5, 30);

    //     storage.UpdateClusterMetrics(U, words, lsa.GetTopics());
    //     storage.OutputClustersToJsonFile(jsonFilePath);
    // }

    // getting ready-made results without waiting for intermediate steps
    {
        // auto& corpus = TextCorpus::GetCorpus();
        // corpus.LoadCorpusFromFile((repoPath / "my_data" / "filtered_corpus").string());
        PhrasesStorageLoader loader;
        auto& corpus = TokenizedSentenceCorpus::GetCorpus();
        corpus.LoadFromFile((repoPath / "my_data" / "nlp_corpus" / "sentences.json").string());

        auto& storage = PatternPhrasesStorage::GetStorage();
        loader.LoadStorageFromFile(storage, jsonFilePath.string());
        storage.AddContextsToClusters();
        storage.CollectTerms();
        storage.OutputClustersToJsonFile(jsonFileResPath.string(), false, true);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Processing took " + std::to_string(duration.count()) + "seconds.";

    return 0;
}
