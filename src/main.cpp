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

void setGlobalOptions(const po::variables_map& vm)
{
    // Override default global options if provided by the user
    if (vm.count("mydata-dir")) {
        g_options.myDataDir = fs::path(vm["mydata-dir"].as<std::string>());
    }
    if (vm.count("corpus-dir")) {
        g_options.corpusDir = fs::path(vm["corpus-dir"].as<std::string>());
    }
    if (vm.count("texts-dir")) {
        g_options.textsDir = fs::path(vm["texts-dir"].as<std::string>());
    }
    if (vm.count("stop-words-file")) {
        g_options.stopWordsFile = fs::path(vm["stop-words-file"].as<std::string>());
    }
    if (vm.count("tags-and-hubs-file")) {
        g_options.tagsAndHubsFile = fs::path(vm["tags-and-hubs"].as<std::string>());
    }
    if (vm.count("results-dir")) {
        g_options.resDir = fs::path(vm["results-dir"].as<std::string>());
    }
    if (vm.count("corpus-file")) {
        g_options.corpusFile = fs::path(vm["corpus-file"].as<std::string>());
    }
    if (vm.count("filtered-corpus-file")) {
        g_options.corpusFile = fs::path(vm["filtered-corpus-file"].as<std::string>());
    }
    if (vm.count("sentences-file")) {
        g_options.sentencesFile = fs::path(vm["sentences-file"].as<std::string>());
    }
    if (vm.count("limit")) {
        g_options.textToProcessCount = vm["limit"].as<int>();
    }
    if (vm.count("clean-stop-words")) {
        g_options.cleanStopWords = vm["clean-stop-words"].as<bool>();
    }
    if (vm.count("validate-boundaries")) {
        g_options.validateBoundaries = vm["validate-boundaries"].as<bool>();
    }
}

void addOptions(po::options_description& desc)
{
    desc.add_options()("help,h", "Show help message");
    desc.add_options()("mydata-dir", po::value<std::string>(), "Path to 'my_data' directory");
    desc.add_options()("corpus-dir", po::value<std::string>(),
                       "Path to 'my_data' directory (default is inside 'my_data')");
    desc.add_options()("texts-dir", po::value<std::string>(),
                       "Path to texts directory (default is inside 'corpusDir')");
    desc.add_options()("stop-words-file", po::value<std::string>(),
                       "Path to stop_words file (default is inside in 'my_data')");
    desc.add_options()("tags-and-hubs-file", po::value<std::string>(),
                       "Path to tags_and_hubs file (default is inside 'corpusDir')");
    desc.add_options()("results-dir", po::value<std::string>(),
                       "Path to 'results' directory (default is inside 'corpusDir')");
    desc.add_options()("corpus-file", po::value<std::string>(), "Path to corpus file (default is inside 'corpusDir')");
    desc.add_options()("filtered-corpus-file", po::value<std::string>(),
                       "Path to filtered corpus file (default is inside 'corpusDir')");
    desc.add_options()("sentences-file", po::value<std::string>(),
                       "Path to sentences.json (default is inside 'corpusDir')");
    desc.add_options()("limit", po::value<int>(),
                       "How many text files to process (by default, it is calculated as the number of all files in the "
                       "texts directory.)");
    desc.add_options()("clean-stop-words", po::value<bool>(), "Option for clearing stop words (by default is true)");
    desc.add_options()("validate-boundaries", po::value<bool>(),
                       "Option for sentence boundaries validation (by default is true)");
}

int main(int argc, char** argv)
{
    using namespace PhrasesCollectorUtils;

    auto start = std::chrono::high_resolution_clock::now();

    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);

    po::options_description desc("Allowed options");
    addOptions(desc);

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

    // Set global options
    setGlobalOptions(vm);

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
            ::Embedding e;
            BuildPhraseStorage();
        } else if (command == "merge_clusters") {
            // Merge similar phrase clusters and compute text-based metrics
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile(g_options.filteredCorpusFile.string());

            PhrasesStorageLoader loader;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadPhraseStorageFromResultsDir(storage);

            ::Embedding e;

            storage.MergeSimilarClusters();
            storage.ComputeTextMetrics();
            storage.OutputClustersToJsonFile(jsonFilePath.string());
        } else if (command == "load_hypernyms") {
            // Load hypernym and hyponym relations for stored lemmas
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile(g_options.filteredCorpusFile.string());

            ::Embedding e;

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

            ::Embedding e;

            auto& sentences = TokenizedSentenceCorpus::GetCorpus();
            sentences.LoadFromFile(g_options.sentencesFile.string());

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

            ::Embedding e;

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
