#include "Logger.h"
#include "MorphAnalyzer.h"

#include "xmorphy/graphem/SentenceSplitter.h"
#include "xmorphy/graphem/Tokenizer.h"
#include "xmorphy/ml/SingleWordDisambiguate.h"
#include "xmorphy/ml/TFJoinedModel.h"
#include "xmorphy/ml/TFMorphemicSplitter.h"
#include "xmorphy/morph/Processor.h"
#include "xmorphy/utils/UniString.h"
#include <gtest/gtest.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace test_utils {

inline std::string load(const std::string& filename) {
    // Используем макрос, определенный в CMake
    std::filesystem::path fixture_dir(TEST_DATA_DIR);
    std::filesystem::path filepath = fixture_dir / filename;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open fixture file: " + filepath.string());
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return content;
}

inline bool dataExists(const std::string& filename) {
    std::filesystem::path fixture_dir(TEST_DATA_DIR);
    std::filesystem::path filepath = fixture_dir / filename;
    return std::filesystem::exists(filepath);
}

} // namespace test_utils

class SentenceProcessingTest : public ::testing::Test {
  protected:
    X::Tokenizer tokenizer;
    X::Processor analyzer;
    X::TFMorphemicSplitter morphemicSplitter;
    X::SingleWordDisambiguate disambiguater;
    X::TFJoinedModel joiner;
    MorphAnalyzer& morphAnalyzer = MorphAnalyzer::getInstance();

    struct ProcessedSentence {
        std::string raw;
        std::string normalized;
        size_t wordCount;
        std::vector<std::pair<std::string, std::string>> wordLemmaPairs; // (word, lemma)
    };

    std::vector<ProcessedSentence> processMultipleSentences(const std::string& inputText) {

        Logger::enableLogging(true);
        Logger::setGlobalLogLevel(LogLevel::Debug);
        fs::path testDataDir = fs::current_path() / "tests" / "data";
        std::string logFilePath = (testDataDir / "test_sentence_processing.txt").string();
        Logger::initializeLogFile(logFilePath);

        std::vector<ProcessedSentence> results;

        // Создаём поток из входного текста
        std::istringstream inputStream(inputText);

        X::SentenceSplitter sentenceSplitter(inputStream);
        size_t sentenceNum = 0;

        while (!sentenceSplitter.eof()) {
            std::string rawSentence;
            sentenceSplitter.readSentence(rawSentence);

            if (rawSentence.empty())
                continue;

            Logger::log("SentenceProcessing", LogLevel::Info,
                        "=== Processing Sentence #" + std::to_string(sentenceNum) + " ===");
            Logger::log("SentenceProcessing", LogLevel::Info, "Raw: " + rawSentence);

            // Токенизация
            std::vector<X::TokenPtr> tokens = tokenizer.analyze(X::UniString(rawSentence));

            // Анализ предложения
            X::Sentence sentence = analyzer.analyze(tokens);

            // Дизамбигуация
            disambiguater.disambiguate(sentence);

            // Mорфемное разбиение и дизамбигуация
            joiner.disambiguateAndMorphemicSplit(sentence);

            // Шаг 6: Нормализация (лемматизация)
            std::string normalizedSentence;
            std::vector<std::pair<std::string, std::string>> wordLemmaPairs;

            for (size_t wordIdx = 0; auto& wordForm : sentence) {
                morphemicSplitter.split(wordForm);

                // Логирование основной информации
                Logger::log("SentenceProcessing", LogLevel::Debug,
                            "  [Word #" + std::to_string(wordIdx) + "] " + wordForm->getWordForm().getRawString());
                Logger::log("SentenceProcessing", LogLevel::Debug,
                            "    Token type: " + wordForm->getTokenType().toString());

                // Логирование всей морфологической информации
                const auto& morphInfos = wordForm->getMorphInfo();
                Logger::log("SentenceProcessing", LogLevel::Debug,
                            "    MorphInfo entries: " + std::to_string(morphInfos.size()));

                for (const auto& morphInfo : morphInfos) {
                    std::string morphLog = "      Lemma: " + morphInfo.normalForm.getRawString() +
                                           " | Gender: " + morphInfo.tag.getGender().toString() +
                                           " | Number: " + morphInfo.tag.getNumber().toString() +
                                           " | Case: " + morphInfo.tag.getCase().toString() +
                                           " | Tense: " + morphInfo.tag.getTense().toString() +
                                           " | Animacy: " + morphInfo.tag.getAnimacy().toString() +
                                           " | VerbForm: " + morphInfo.tag.getVerbForm().toString() +
                                           " | Mood: " + morphInfo.tag.getMood().toString() +
                                           " | Person: " + morphInfo.tag.getPerson().toString() +
                                           " | Voice: " + morphInfo.tag.getVoice().toString() +
                                           " | Aspect: " + morphInfo.tag.getAspect().toString() +
                                           " | Cmp: " + morphInfo.tag.getCmp().toString() +
                                           " | Variance: " + morphInfo.tag.getVariance().toString() +
                                           " | SP: " + morphInfo.sp.toString() +
                                           " | Probability: " + std::to_string(morphInfo.probability) +
                                           " | AnalyzerTag: " + morphInfo.at.toString() +
                                           " | StemLen: " + std::to_string(morphInfo.stemLen);
                    Logger::log("SentenceProcessing", LogLevel::Debug, morphLog);
                }

                // Логирование фонемной информации
                const auto& phemInfo = wordForm->getPhemInfo();
                if (!phemInfo.empty()) {
                    const X::UniString& uniWord = wordForm->getWordForm();

                    Logger::log("SentenceProcessing", LogLevel::Debug,
                                "    PhemTags count: " + std::to_string(phemInfo.size()));

                    // Визуальный разбор
                    std::string breakdown = "    Morpheme breakdown: ";
                    for (size_t i = 0; i < phemInfo.size() && i < uniWord.length(); ++i) {
                        breakdown += uniWord.charAtAsString(i);

                        if (i < phemInfo.size() - 1 && phemInfo[i] != phemInfo[i + 1]) {
                            breakdown += "| ";
                        }
                    }
                    Logger::log("SentenceProcessing", LogLevel::Debug, breakdown);

                    // Детальный список
                    Logger::log("SentenceProcessing", LogLevel::Debug, "    Detailed PhemTag analysis:");
                    std::string detailedLog = "";
                    for (size_t i = 0; i < phemInfo.size() && i < uniWord.length(); ++i) {
                        std::string charLog = "      [" + std::to_string(i) + "] '" + uniWord.charAtAsString(i) +
                                              "' -> " + phemInfo[i].toString() + "\t";
                        detailedLog.append(charLog);
                    }
                    Logger::log("SentenceProcessing", LogLevel::Debug, detailedLog);
                }

                if (wordForm->getTokenType() != X::TokenTypeTag::WORD) {
                    ++wordIdx;
                    continue;
                }

                std::string lemma = morphAnalyzer.getLemma(wordForm);
                normalizedSentence.append(lemma + " ");

                // Сохраняем пару (слово, лемма)
                wordLemmaPairs.push_back({wordForm->getWordForm().getRawString(), lemma});
                ++wordIdx;
            }

            if (!normalizedSentence.empty()) {
                normalizedSentence.pop_back(); // Удалить последний пробел
            }

            Logger::log("SentenceProcessing", LogLevel::Info, "Normalized: " + normalizedSentence);

            results.push_back({rawSentence, normalizedSentence, wordLemmaPairs.size(), wordLemmaPairs});

            sentenceNum++;
        }

        return results;
    }
};

// Тест обработки нескольких предложений
TEST_F(SentenceProcessingTest, MultipleSimpleSentences) {
    std::string inputText = test_utils::load("complex_text.txt");

    auto results = processMultipleSentences(inputText);

    // Проверяем, что обработано несколько предложений
    EXPECT_GE(results.size(), 3);

    // Проверяем каждое предложение
    for (size_t i = 0; i < results.size(); ++i) {
        EXPECT_FALSE(results[i].raw.empty());
        EXPECT_FALSE(results[i].normalized.empty());
        EXPECT_GT(results[i].wordCount, 0);

        std::cout << "\n--- Sentence #" << i << " ---" << std::endl;
        std::cout << "Raw: " << results[i].raw << std::endl;
        std::cout << "Normalized: " << results[i].normalized << std::endl;
        std::cout << "Word count: " << results[i].wordCount << std::endl;
        std::cout << "Word-Lemma pairs:" << std::endl;
        for (const auto& [word, lemma] : results[i].wordLemmaPairs) {
            std::cout << "  " << word << " -> " << lemma << std::endl;
        }
    }
}