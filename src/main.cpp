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
#include <boost/program_options.hpp>

#include <chrono>
#include <filesystem>

#include <sys/stat.h>

namespace fs = std::filesystem;

namespace po = boost::program_options;

static void printUsage(const po::options_description& desc)
{
    std::cout << "Usage: myprogram <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  collect_phrases           Collect phrases for each text, save phrase storage.\n";
    std::cout << "  merge_clusters            Merge similar clusters, compute text metrics.\n";
    std::cout << "  load_hypernyms            Load WikiWordNet relations (hypernyms/hyponyms) into clusters.\n";
    std::cout << "  build_tokenized_corpus    Build & save a tokenized sentence corpus.\n";
    std::cout << "  perform_lsa               Perform LSA analysis on previously saved data.\n";
    std::cout << "  get_prepared_results      Load prepared results without waiting for intermediate steps.\n";
    std::cout << "\nOptions:\n" << desc << "\n";
}

int main(int argc, char** argv)
{
    using namespace PhrasesCollectorUtils;

    ::Embedding e;
    auto start = std::chrono::high_resolution_clock::now();

    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);

    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Show help message")("mydata-dir", po::value<std::string>(),
                                                      "Path to 'my_data' directory")(
        "texts-dir", po::value<std::string>(), "Path to texts directory (inside myDataDir by default)")(
        "stop-words", po::value<std::string>(), "Path to stop_words.txt")("tags-and-hubs", po::value<std::string>(),
                                                                          "Path to tags_and_hubs_line_counts.txt")(
        "res-dir", po::value<std::string>(), "Path to 'res' directory")("corpus-file", po::value<std::string>(),
                                                                        "Path to corpus file")(
        "sentences-file", po::value<std::string>(), "Path to sentences.json")("limit", po::value<int>(),
                                                                              "How many text files to process");

    // Parse at least one argument (the command). If not provided, show help.
    if (argc < 2) {
        std::cerr << "No command provided.\n";
        printUsage(desc);
        return 1;
    }

    std::string command = argv[1];

    // Parse additional options
    po::variables_map vm;
    try {
        std::vector<std::string> opts(argv + 2, argv + argc);
        po::store(po::command_line_parser(opts).options(desc).run(), vm);
        po::notify(vm);
    } catch (std::exception& ex) {
        std::cerr << "Error parsing command line: " << ex.what() << "\n";
        return 1;
    }

    // Show help message if requested
    if (vm.count("help")) {
        printUsage(desc);
        return 0;
    }

    // Override default global options if provided by the user
    if (vm.count("limit")) {
        g_options.textToProcessCount = vm["limit"].as<int>();
    }
    if (vm.count("mydata-dir")) {
        g_options.myDataDir = fs::path(vm["mydata-dir"].as<std::string>());
    }
    if (vm.count("texts-dir")) {
        g_options.textsDir = fs::path(vm["texts-dir"].as<std::string>());
    }
    if (vm.count("stop-words")) {
        g_options.stopWordsFile = fs::path(vm["stop-words"].as<std::string>());
    }
    if (vm.count("tags-and-hubs")) {
        g_options.tagsAndHubsFile = fs::path(vm["tags-and-hubs"].as<std::string>());
    }
    if (vm.count("res-dir")) {
        g_options.resDir = fs::path(vm["res-dir"].as<std::string>());
    }
    if (vm.count("corpus-file")) {
        g_options.corpusFile = fs::path(vm["corpus-file"].as<std::string>());
    }
    if (vm.count("sentences-file")) {
        g_options.sentencesFile = fs::path(vm["sentences-file"].as<std::string>());
    }

    // Initialize logging system
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);

    // Define paths for output JSON files
    fs::path jsonFilePath = g_options.myDataDir / "total_results.json";
    fs::path jsonFileResPath = g_options.myDataDir / "terms.json";

    // Execute command
    try {
        if (command == "collect_phrases") {
            // Collect phrases from texts and save the storage
            fs::path patternsPath = g_options.myDataDir / "patterns.txt";
            GrammarPatternManager::GetManager()->readPatterns(patternsPath);

            BuildPhraseStorage();
        } else if (command == "merge_clusters") {
            // Merge similar phrase clusters and compute text-based metrics
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile((g_options.myDataDir / "filtered_corpus").string());

            PhrasesStorageLoader loader;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadPhraseStorageFromResultsDir(storage);

            storage.MergeSimilarClusters();
            storage.ComputeTextMetrics();
            storage.OutputClustersToJsonFile(jsonFilePath.string());
        } else if (command == "load_hypernyms") {
            // Load hypernym and hyponym relations for stored lemmas
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile((g_options.myDataDir / "filtered_corpus").string());

            PhrasesStorageLoader loader;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, jsonFilePath.string());
            storage.LoadWikiWNRelations();
            storage.OutputClustersToJsonFile(jsonFilePath);
        } else if (command == "build_tokenized_corpus") {
            // Generate a tokenized sentence corpus and save it
            BuildTokenizedSentenceCorpus();
        } else if (command == "perform_lsa") {
            // Load preprocessed data and execute Latent Semantic Analysis (LSA)
            PhrasesStorageLoader loader;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, jsonFilePath.string());

            auto& sentences = TokenizedSentenceCorpus::GetCorpus();
            sentences.LoadFromFile(g_options.sentencesFile.string()); // например

            std::cout << "Sentences loaded successfully. Total sentences: " << sentences.totalSentences << std::endl;

            LSA lsa(sentences);
            std::cout << "Starting LSA analysis..." << std::endl;
            lsa.PerformAnalysis(false);

            MatrixXd U = lsa.GetU();
            MatrixXd Sigma = lsa.GetSigma();
            MatrixXd V = lsa.GetV();
            std::vector<std::string> words = lsa.GetWords();

            std::cout << "LSA analysis completed successfully!" << std::endl;
            std::cout << "Matrix U size: " << U.rows() << "x" << U.cols() << std::endl;
            std::cout << "Matrix Sigma size: " << Sigma.rows() << "x" << Sigma.cols() << std::endl;
            std::cout << "Matrix V size: " << V.rows() << "x" << V.cols() << std::endl;

            std::cout << "\nAnalyzing top topics..." << std::endl;
            lsa.AnalyzeTopics(5, 30);

            storage.UpdateClusterMetrics(U, words, lsa.GetTopics());
            storage.OutputClustersToJsonFile(jsonFilePath);
        } else if (command == "get_prepared_results") {
            // Load precomputed results without additional processing
            PhrasesStorageLoader loader;
            auto& corpus = TokenizedSentenceCorpus::GetCorpus();
            corpus.LoadFromFile(g_options.sentencesFile.string());

            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, jsonFilePath.string());
            storage.AddContextsToClusters();
            storage.CollectTerms();
            storage.OutputClustersToJsonFile(jsonFileResPath.string(), false, true);
        } else {
            std::cerr << "Unknown command: " << command << "\n";
            printUsage(desc);
            return 1;
        }
    } catch (const std::exception& ex) {
        Logger::log("Main", LogLevel::Error, std::string("Exception: ") + ex.what());
        return 1;
    } catch (...) {
        Logger::log("Main", LogLevel::Error, "Unknown exception");
        return 1;
    }

    // Measure execution time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Processing took " << duration.count() << " seconds.\n";

    return 0;
}
