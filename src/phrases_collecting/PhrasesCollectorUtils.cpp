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

#include <GrammarPatternManager.h>
#include <PatternPhrasesStorage.h>
#include <PhrasesCollectorUtils.h>

#include <cctype>
#include <nlohmann/json.hpp>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

using json = nlohmann::json;
using namespace X;

namespace PhrasesCollectorUtils {
    Options g_options;

    std::vector<fs::path> GetFilesToProcess()
    {
        fs::path repoPath = fs::current_path();
        fs::path inputDir = repoPath / "my_data/texts";
        std::vector<fs::path> files_to_process;
        files_to_process.reserve(g_options.textToProcessCount);
        for (const auto& entry : fs::directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("art") == 0 && filename.find("_text.txt") != std::string::npos) {
                    files_to_process.push_back(entry.path());
                }
            }
        }
        return files_to_process;
    }

    std::vector<fs::path> GetResFiles()
    {
        fs::path repoPath = fs::current_path();
        fs::path inputDir = repoPath / "res";
        std::vector<fs::path> files_to_process;
        files_to_process.reserve(g_options.textToProcessCount);
        for (const auto& entry : fs::directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("res") == 0 && filename.find("_text.json") != std::string::npos) {
                    files_to_process.push_back(entry.path());
                }
            }
        }
        return files_to_process;
    }

    void RemoveSeparatorTokens(std::vector<WordFormPtr>& forms)
    {
        forms.erase(std::remove_if(forms.begin(), forms.end(),
                                   [](const WordFormPtr& form) { return form->getTokenType() == TokenTypeTag::SEPR; }),
                    forms.end());
    }

    void ProcessFile(const fs::path& inputFile, const fs::path& outputDir, int& counter, std::mutex& counterMutex)
    {
        std::string filename = inputFile.filename().string();
        std::string outputFile = (outputDir / ("res_" + filename)).string();
        auto startProcessText = std::chrono::high_resolution_clock::now();

        if (g_options.multithreading) {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            std::string thread_id = oss.str();
            Logger::log("Thread", LogLevel::Info, thread_id + " starting file processing: " + inputFile.string());
        }
        {
            std::lock_guard<std::mutex> lock(counterMutex);
            ++counter;
        }

        Tokenizer tok;
        TFMorphemicSplitter morphemic_splitter;
        Process process(inputFile, outputFile);
        SentenceSplitter ssplitter(process.m_input);
        Processor analyzer;
        SingleWordDisambiguate disamb;
        TFJoinedModel joiner;

        auto& storage = PatternPhrasesStorage::GetStorage();
        auto& corpus = TextCorpus::GetCorpus();

        do {
            std::string sentence;
            ssplitter.readSentence(sentence);

            if (sentence.empty())
                continue;

            std::vector<TokenPtr> tokens = tok.analyze(UniString(sentence));
            std::vector<WordFormPtr> forms = analyzer.analyze(tokens);

            RemoveSeparatorTokens(forms);
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
        PatternPhrasesStorage::GetStorage().FinalizeDocumentProcessing();

        auto endProccesText = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = endProccesText - startProcessText;
        Logger::log("", LogLevel::Info,
                    "processText() for " + filename + " took " + std::to_string(duration.count()) + " seconds.");
    }

    void BuildPhraseStorage()
    {
        fs::path repoPath = fs::current_path();
        fs::path outputDir = repoPath / "res";
        fs::create_directories(outputDir);

        auto& storage = PatternPhrasesStorage::GetStorage();
        auto& corpus = TextCorpus::GetCorpus();
        try {
            int counter = 0;
            std::mutex counterMutex;

            std::vector<fs::path> files_to_process = GetFilesToProcess();
            if (g_options.multithreading) {

                const size_t batchSize = 15;
                for (size_t batchStart = 0; batchStart < g_options.textToProcessCount; batchStart += batchSize) {
                    std::vector<std::thread> threads;
                    size_t batchEnd =
                        std::min(batchStart + batchSize, static_cast<unsigned long>(g_options.textToProcessCount));
                    for (size_t i = batchStart; i < batchEnd; ++i) {
                        threads.emplace_back([&, i]() {
                            corpus.LoadTextsFromFile(files_to_process[i]);
                            ProcessFile(files_to_process[i], outputDir, counter, counterMutex);
                        });
                    }

                    for (auto& thread : threads) {
                        if (thread.joinable()) {
                            thread.join();
                        }
                    }
                }
                // storage.threadController.pauseUntilAllThreadsReach();

            } else {
                for (unsigned int i = 0; i < files_to_process.size(); ++i) {
                    corpus.LoadTextsFromFile(files_to_process[i]);
                    ProcessFile(files_to_process[i], outputDir, counter, counterMutex);
                }
            }

            TextCorpus::GetCorpus().SaveCorpusToFile((repoPath / "my_data" / "corpus").string());
            Logger::log("\n\nProcessed", LogLevel::Info, std::to_string(counter) + " files");

        } catch (const std::exception& e) {
            Logger::log("", LogLevel::Error, "Exception caught: " + std::string(e.what()));
        } catch (...) {
            Logger::log("", LogLevel::Error, "Unknown exception caught");
        }
    }

    MorphInfo GetMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet)
    {
        auto maxElement = *morphSet.begin();
        for (const auto& elem : morphSet) {
            if (elem.probability > maxElement.probability) {
                maxElement = elem;
            }
        }
        return maxElement;
    }

    bool MorphAnanlysisError(const WordFormPtr& token)
    {
        auto isDesiredPOS = [](const UniSPTag& tag) -> bool {
            static const std::unordered_set<std::string> desiredPOS = {"ADJ", "NOUN", "PROPN", "VERB"};
            return desiredPOS.find(tag.toString()) != desiredPOS.end();
        };

        return token->getWordForm().length() == 1 && token->getMorphInfo().size() == 1 &&
               isDesiredPOS(token->getMorphInfo().begin()->sp);
    }

    bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo)
    {
        for (const auto& morphForm : currFormMorphInfo) {
            const auto& spSet = GrammarPatternManager::GetManager()->getUsedSp();
            if (spSet.find(morphForm.sp.toString()) != spSet.end())
                return true;
        }
        return false;
    }

    const std::string GetLowerCase(const std::string& line)
    {
        // Convert to lowercase using ICU
        icu::UnicodeString ustr(line.c_str(), "UTF-8");
        ustr.toLower(icu::Locale("ru_RU"));
        std::string lowerLine;
        ustr.toUTF8String(lowerLine);
        return lowerLine;
    }

    const std::unordered_set<std::string> GetStopWords()
    {
        static std::unordered_set<std::string> stopWords;
        static bool initialized = false;

        if (initialized) {
            return stopWords;
        }

        std::filesystem::path repoPath = std::filesystem::current_path();
        std::filesystem::path inputPath = repoPath / "my_data/stop_words.txt";

        std::ifstream file(inputPath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file " + inputPath.string());
        }

        std::string line;
        while (std::getline(file, line)) {
            const auto lowerLine = GetLowerCase(line);
            stopWords.insert(lowerLine);
        }

        initialized = true;
        return stopWords;
    }

    bool contains_no_latin(const std::string& str)
    {
        for (char ch : str) {
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                return false;
            }
        }
        return true;
    }

    const std::unordered_set<std::string> GetTopics()
    {
        static std::unordered_set<std::string> topics;
        static bool initialized = false;

        if (initialized) {
            return topics;
        }

        std::filesystem::path repoPath = std::filesystem::current_path();
        std::filesystem::path inputPath = repoPath / "my_data/tags_and_hubs_line_counts.txt";

        std::ifstream file(inputPath);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file " + inputPath.string());
        }

        std::string line;
        while (std::getline(file, line)) {

            // Check if the line contains "Блог компании"
            if (line.find("Блог компании") != std::string::npos) {
                continue;
            }

            if (line.find(" 1") != std::string::npos) {
                continue;
            }

            if (!contains_no_latin(line))
                continue;

            // Remove trailing digits
            line.erase(
                std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isdigit(ch); }).base(),
                line.end());

            // Remove trailing spaces
            line.erase(
                std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                line.end());

            const auto lowerLine = GetLowerCase(line);
            if (lowerLine.size() > 3)
                topics.insert(lowerLine);
        }

        initialized = true;
        return topics;
    }

    const std::unordered_map<std::string, WordEmbeddingPtr>& GetTopicVectors()
    {
        static std::unordered_map<std::string, WordEmbeddingPtr> topicVectors;
        static bool initialized = false;

        if (!initialized) {
            std::unordered_set<std::string> topics = GetTopics();

            for (const auto& t : topics) {
                topicVectors[t] = std::make_shared<WordEmbedding>(t);
            }

            initialized = true;
        }

        return topicVectors;
    }

    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr)
    {
        Logger::log("CURRENT SIMPLE PHRASE", LogLevel::Debug,
                    curSimplePhr->textForm + " || " + curSimplePhr->modelName);
    }

    void LogCurrentComplexModel(const std::string& name)
    {
        Logger::log("CURRENT COMPLEX MODEL", LogLevel::Debug, name);
    }

    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft)
    {
        curPhrStatus.correct++;
        if (isLeft) {
            wc->pos.start = asidePhrase->pos.start;
            wc->textForm.insert(0, asidePhrase->textForm + " ");
        } else {
            wc->pos.end = asidePhrase->pos.end;
            wc->textForm.append(" " + asidePhrase->textForm);
        }
    }

    bool CheckForMisclassifications(const WordFormPtr& form)
    {
        std::unordered_set<char> punctuation = {'!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+',
                                                ',', '-',  '.', '/', ':', ';', '<',  '=', '>', '?', '@',
                                                '[', '\\', ']', '^', '_', '`', '{',  '|', '}', '~'};

        try {
            const auto str = form->getWordForm().getRawString();

            for (char c : str) {
                if (!std::isdigit(c) && punctuation.find(c) == punctuation.end())
                    return false;
            }
            return true;
        } catch (const std::exception& e) {
            return false;
        } catch (...) {
            return false;
        }

        return true;
    }

    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process)
    {
        for (const auto& wc : collection) {
            std::string key;
            for (const auto& w : wc->words) {
                key.append(GetLemma(w) + " ");
            }
            if (!key.empty()) {
                key.pop_back();
            }

            json lemmas_json = json::array();
            for (size_t i = 0; i < wc->lemmas.size(); i++) {
                lemmas_json.push_back(std::to_string(i) + "_" + wc->lemmas[i]);
            }

            json j = json::object();
            j["0_key"] = key;
            j["1_textForm"] = wc->textForm;
            j["2_modelName"] = wc->modelName;
            j["3_docNum"] = process.m_docNum;
            j["4_sentNum"] = process.m_sentNum;
            j["5_start_ind"] = wc->pos.start;
            j["6_end_ind"] = wc->pos.end;
            j["7_lemmas"] = lemmas_json;

            process.m_output << j.dump(4) << std::endl;
        }
    }

    const std::string GetLemma(const WordFormPtr& form)
    {
        return GetMostProbableMorphInfo(form->getMorphInfo()).normalForm.toLowerCase().getRawString();
    }
}