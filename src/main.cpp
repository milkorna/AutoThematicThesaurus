#include <boost/program_options.hpp>
#include <xmorphy/graphem/SentenceSplitter.h>
#include <xmorphy/graphem/Tokenizer.h>
#include <xmorphy/ml/Disambiguator.h>
#include <xmorphy/ml/MorphemicSplitter.h>
#include <xmorphy/ml/SingleWordDisambiguate.h>
#include <xmorphy/ml/TFDisambiguator.h>
#include <xmorphy/ml/TFJoinedModel.h>
#include <xmorphy/ml/TFMorphemicSplitter.h>
#include <xmorphy/morph/JSONEachSentenceFormater.h>
#include <xmorphy/morph/PrettyFormater.h>
#include <xmorphy/morph/Processor.h>
#include <xmorphy/morph/TSVFormater.h>
#include <xmorphy/morph/WordFormPrinter.h>
#include <xmorphy/utils/UniString.h>

#include <Component.h>
#include <Logger.h>
#include <OutputRedirector.h>
#include <PatternPhrasesStorage.h>

#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

void removeSeparatorTokens(std::vector<WordFormPtr>& forms)
{
    forms.erase(std::remove_if(forms.begin(), forms.end(),
                               [](const WordFormPtr& form) { return form->getTokenType() == TokenTypeTag::SEPR; }),
                forms.end());
}

void processTextFile(const fs::path& inputFile, const fs::path& outputDir, int& counter, std::mutex& counterMutex)
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    std::string thread_id = oss.str();
    Logger::log("processTextFile", LogLevel::Info,
                "Thread " + thread_id + " starting file processing: " + inputFile.string());

    std::string filename = inputFile.filename().string();
    {
        std::lock_guard<std::mutex> lock(counterMutex);
        ++counter;
        std::cout << counter << std::endl;
    }
    std::string outputFile = (outputDir / ("res_" + filename)).string();
    auto startProcessText = std::chrono::high_resolution_clock::now();

    Tokenizer tok;
    TFMorphemicSplitter morphemic_splitter;
    Process process(inputFile, outputFile);
    SentenceSplitter ssplitter(process.m_input);
    Processor analyzer;
    SingleWordDisambiguate disamb;
    TFJoinedModel joiner;

    do {
        std::string sentence;
        ssplitter.readSentence(sentence);

        if (sentence.empty())
            continue;

        std::vector<TokenPtr> tokens = tok.analyze(UniString(sentence));
        std::vector<WordFormPtr> forms = analyzer.analyze(tokens);

        removeSeparatorTokens(forms);
        disamb.disambiguate(forms);
        joiner.disambiguateAndMorphemicSplit(forms);

        for (auto& form : forms) {
            morphemic_splitter.split(form);
        }

        Logger::log("SentenceReading", LogLevel::Info, "Read sentence: " + sentence);
        Logger::log("TokenAnalysis", LogLevel::Debug, "Token count: " + std::to_string(tokens.size()));
        Logger::log("FormAnalysis", LogLevel::Debug, "Form count: " + std::to_string(forms.size()));

        PatternPhrasesStorage::GetStorage().Collect(forms, process);

        process.m_output.flush();
        process.m_sentNum++;
    } while (!ssplitter.eof());

    auto endProccesText = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = endProccesText - startProcessText;
    Logger::log("main", LogLevel::Info,
                "processText() for " + filename + " took " + std::to_string(duration.count()) + " seconds.");
}

int main()
{
    Logger::enableLogging(true);
    Logger::setGlobalLogLevel(LogLevel::Info);

    fs::path repoPath = fs::current_path();
    fs::path inputDir = repoPath / "my_data/texts";
    fs::path outputDir = repoPath / "res";

    fs::create_directories(outputDir);

    try {
        std::string filePath = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/patterns.txt";
        GrammarPatternManager::GetManager()->readPatterns(filePath);
        // GrammarPatternManager::GetManager()->printPatterns();
    } catch (const std::exception& e) {
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    PatternPhrasesStorage::GetStorage();

    try {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<fs::path> files_to_process;
        files_to_process.push_back("/home/milkorna/Documents/AutoThematicThesaurus/my_data/texts/art325014_text.txt");
        for (const auto& entry : fs::directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("art") == 0 && filename.find("_text.txt") != std::string::npos) {
                    files_to_process.push_back(entry.path());
                }
            }
        }

        unsigned int numThreads = std::thread::hardware_concurrency();
        PatternPhrasesStorage::GetStorage().threadController.setTotalThreads(numThreads);
        Logger::log("main", LogLevel::Info, "Amount of threads: " + std::to_string(numThreads));

        std::vector<std::thread> threads;
        int counter = 0;
        std::mutex counterMutex;

        for (unsigned int i = 0; i < 10; ++i) {
            processTextFile(files_to_process[i], outputDir, counter, counterMutex);
        }

        // for (unsigned int i = 0; i < files_to_process.size() && i < 10; ++i) {
        //     threads.emplace_back([&, i]() { processTextFile(files_to_process[i], outputDir, counter,
        //     counterMutex); });
        // }

        // for (auto& thread : threads) {
        //     if (thread.joinable()) {
        //         thread.join();
        //     }
        // }
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;

        // PatternPhrasesStorage::GetStorage().threadController.pauseUntilAllThreadsReach();

        Logger::log("\n\n\n\nmain", LogLevel::Info,
                    "Processing " + std::to_string(counter) + "texts took " + std::to_string(duration.count()) +
                        " seconds.");

        std::string totalResultsFile = "/home/milkorna/Documents/AutoThematicThesaurus/my_data/total_results.txt";
        PatternPhrasesStorage::GetStorage().OutputClustersToFile(totalResultsFile);
    } catch (const std::exception& e) {
        Logger::log("main", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        return EXIT_FAILURE;
    } catch (...) {
        Logger::log("main", LogLevel::Error, "Unknown exception caught");
        return EXIT_FAILURE;
    }

    return 0;
}
