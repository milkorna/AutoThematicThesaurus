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
auto& options = Options::getOptions();

static void printUsage(const po::options_description& desc)
{
    std::cout << "Usage: myprogram <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  collect_phrases           Collect phrases for each text, save phrase storage.\n";
    std::cout << "  filter_corpus             Remove invalid words and sentences from the corpus data and save.\n";
    std::cout << "  compute_text_metrics      Merge identical clusters and compute text metrics: tf, idf, tf-idf, "
                 "tag_match. Inicialize topic_relevance and centrality score.\n";
    std::cout << "  load_hypernyms            Load WikiWordNet relations (hypernyms/hyponyms) into clusters.\n";
    std::cout << "  build_tokenized_corpus    Save a all sentence from corpus in lemmatized form.\n";
    std::cout << "  perform_lsa               Perform LSA analysis on previously saved data, compute topic_relevance "
                 "and centrality score.\n";
    std::cout << "  get_terminological_phrases      Filter out more relevant and terminological phrases from all the "
                 "results.\n";
    std::cout << "\nOptions:\n" << desc << "\n";
}

void validatePathOption(const po::variables_map& vm, const std::string& option_name, fs::path& target)
{
    if (vm.count(option_name)) {
        try {
            std::string value = vm[option_name].as<std::string>();

            if (value.empty()) {
                throw std::runtime_error("Value for '" + option_name + "' is empty.");
            }

            target = target.parent_path() / value;

            if (!fs::exists(target)) {
                throw std::runtime_error("Path for '" + option_name + "' does not exist: " + value);
            }
        } catch (const std::exception& ex) {
            throw std::runtime_error("Invalid value for '" + option_name + "': " + std::string(ex.what()));
        }
    }
}

void validateBoolOption(const po::variables_map& vm, const std::string& option_name, bool& target)
{
    if (vm.count(option_name)) {
        try {
            target = vm[option_name].as<bool>();
        } catch (const std::exception& ex) {
            throw std::runtime_error("Invalid boolean value for '" + option_name + "': " + std::string(ex.what()));
        }
    }
}

void validateLimitOption(const po::variables_map& vm, int minVal = 1, int maxVal = INT_MAX)
{
    if (vm.count("limit")) {
        try {
            int value = vm["limit"].as<int>();

            if (value < minVal || value > maxVal) {
                throw std::runtime_error("Value for 'limit' is out of range (" + std::to_string(minVal) + " - " +
                                         std::to_string(maxVal) + ")");
            }

            options.textToProcessCount = value;
        } catch (const std::exception& ex) {
            throw std::runtime_error("Invalid integer value for 'limit': " + std::string(ex.what()));
        }
    }
}

void setGlobalOptions(const po::variables_map& vm)
{
    // Override default global options if provided by the user
    validatePathOption(vm, "corpus-dir", options.corpusDir);
    options.recomputeCorpusDependenciesPaths();
    options.updateFileCount();

    validatePathOption(vm, "stop-words-file", options.stopWordsFile);
    validatePathOption(vm, "patterns-file", options.patternsFile);
    validatePathOption(vm, "emb-model-file", options.embeddingModelFile);

    validateLimitOption(vm, 1, options.textToProcessCount);

    validateBoolOption(vm, "clean-stop-words", options.cleanStopWords);
    validateBoolOption(vm, "validate-boundaries", options.validateBoundaries);

    Logger::log("Main", LogLevel::Info, "corpusDir: " + options.corpusDir.string());
    Logger::log("Main", LogLevel::Info, "textsDir:  " + options.textsDir.string());
    Logger::log("Main", LogLevel::Info, "textToProcessCount: " + std::to_string(options.textToProcessCount));
}

void addOptions(po::options_description& desc)
{
    desc.add_options()("help,h", "Show help message");
    desc.add_options()("corpus-dir", po::value<std::string>(), "Path to data directory (default is inside 'my_data')");
    desc.add_options()("patterns-file", po::value<std::string>(),
                       "Path to grammatical patterns file (default is inside 'my_data')");
    desc.add_options()("stop-words-file", po::value<std::string>(),
                       "Path to list of stop words file (default is inside in 'my_data')");
    desc.add_options()("emb-model-file", po::value<std::string>(),
                       "Path to embeddings model file (default is my_custom_fasttext_model_finetuned.bin)");
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
    auto& options = Options::getOptions();

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

    // Initialize logging system
    fs::path repoPath = fs::current_path();
    std::string logFilePath = (repoPath / "my_logs.txt").string();
    Logger::initializeLogFile(logFilePath);

    // Set global options
    setGlobalOptions(vm);

    // Execute command
    try {
        if (command == "collect_phrases") {
            Logger::log("Main", LogLevel::Info, "Starting phrase collection...");
            fs::path patternsPath = options.patternsFile;
            GrammarPatternManager::GetManager()->readPatterns(patternsPath);
            BuildPhraseStorage();
            Logger::log("Main", LogLevel::Info, "Phrase collection completed successfully.");
        } else if (command == "filter_corpus") {
            Logger::log("Main", LogLevel::Info, "Starting filtering corpus...");
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile(options.corpusFile.string());
            corpus.SaveCorpusToFile(options.filteredCorpusFile);
            Logger::log("Main", LogLevel::Info, "Filtering corpus completed successfully.");
        } else if (command == "compute_text_metrics") {
            Logger::log("Main", LogLevel::Info, "Starting computing text metrics...");
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile(options.filteredCorpusFile.string());
            PhrasesStorageLoader loader;
            ::Embedding e;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadPhraseStorageFromResultsDir(storage);
            storage.MergeSimilarClusters();
            storage.ComputeTextMetrics();
            storage.OutputClustersToJsonFile(options.totalResultsPath.string());
            Logger::log("Main", LogLevel::Info, "Computing text metrics completed successfully.");
        } else if (command == "load_hypernyms") {
            // Load hypernym and hyponym relations for stored lemmas
            Logger::log("Main", LogLevel::Info, "Loading hypernyms and hyponyms...");
            auto& corpus = TextCorpus::GetCorpus();
            corpus.LoadCorpusFromFile(options.filteredCorpusFile.string());

            ::Embedding e;

            PhrasesStorageLoader loader;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, options.totalResultsPath.string());
            storage.LoadWikiWNRelations();
            storage.OutputClustersToJsonFile(options.totalResultsPath);

        } else if (command == "build_tokenized_corpus") {
            // Generate a tokenized sentence corpus and save it
            BuildTokenizedSentenceCorpus();
        } else if (command == "perform_lsa") {
            // Load preprocessed data and execute Latent Semantic Analysis (LSA)
            Logger::log("Main", LogLevel::Info, "Starting LSA analysis...");
            PhrasesStorageLoader loader;
            ::Embedding e;
            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, options.totalResultsPath.string());

            auto& sentences = TokenizedSentenceCorpus::GetCorpus();
            sentences.LoadFromFile(options.sentencesFile.string());

            LSA lsa(sentences);
            lsa.PerformAnalysis(false);

            MatrixXd U = lsa.GetU();
            MatrixXd Sigma = lsa.GetSigma();
            MatrixXd V = lsa.GetV();
            std::vector<std::string> words = lsa.GetWords();

            Logger::log("LSA", LogLevel::Info, "LSA analysis completed successfully.");
            Logger::log("LSA", LogLevel::Info,
                        "Matrix U size: " + std::to_string(U.rows()) + "x" + std::to_string(U.cols()));
            Logger::log("LSA", LogLevel::Info,
                        "Matrix Sigma size: " + std::to_string(Sigma.rows()) + "x" + std::to_string(Sigma.cols()));
            Logger::log("LSA", LogLevel::Info,
                        "Matrix V size: " + std::to_string(V.rows()) + "x" + std::to_string(V.cols()));

            Logger::log("LSA", LogLevel::Info, "Analyzing top topics...");
            lsa.AnalyzeTopics(5, 30);

            storage.UpdateClusterMetrics(U, words, lsa.GetTopics());
            storage.OutputClustersToJsonFile(options.totalResultsPath);
        } else if (command == "get_terminological_phrases") {
            // Load precomputed results without additional processing
            Logger::log("Main", LogLevel::Info, "Loading precomputed results...");
            PhrasesStorageLoader loader;
            auto& corpus = TokenizedSentenceCorpus::GetCorpus();
            corpus.LoadFromFile(options.sentencesFile.string());

            ::Embedding e;

            auto& storage = PatternPhrasesStorage::GetStorage();
            loader.LoadStorageFromFile(storage, options.totalResultsPath.string());
            storage.AddContextsToClusters();
            storage.CollectTerms();
            storage.OutputClustersToJsonFile(options.termsCandidatesPath.string(), false, true);
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
