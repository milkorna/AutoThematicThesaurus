#include <GrammarPatternManager.h>
#include <PhrasesCollectorUtils.h>

#include <cctype>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

#include <filesystem>

using namespace X;

namespace PhrasesCollectorUtils {

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

            // Remove trailing digits
            line.erase(
                std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isdigit(ch); }).base(),
                line.end());

            // Remove trailing spaces
            line.erase(
                std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                line.end());

            // Convert to lowercase using ICU
            icu::UnicodeString ustr(line.c_str(), "UTF-8");
            ustr.toLower(icu::Locale("ru_RU"));
            std::string lowerLine;
            ustr.toUTF8String(lowerLine);

            if (lowerLine.size() > 3)
                topics.insert(lowerLine);
        }

        initialized = true;
        return topics;
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

    void UpdateWordComplex(const WordComplexPtr& wc, const WordFormPtr& form, const std::string& formFromText,
                           bool isLeft)
    {
        if (isLeft) {
            wc->words.push_front(form);
            wc->pos.start--;
            wc->textForm.insert(0, formFromText + " ");
        } else {
            wc->words.push_back(form);
            wc->pos.end++;
            wc->textForm.append(" " + formFromText);
        }
    }

    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (auto rit = asidePhrase->words.rbegin(); rit != asidePhrase->words.rend(); ++rit) {
            wc->words.push_front(*rit);
        }
    }

    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (const auto& word : asidePhrase->words) {
            wc->words.push_back(word);
        }
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

    WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName)
    {
        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->words = curSimplePhr->words;
        wc->textForm = curSimplePhr->textForm;
        wc->pos = curSimplePhr->pos;
        wc->modelName = modelName;
        return wc;
    }

    WordComplexPtr InicializeWordComplex(const size_t tokenInd, const WordFormPtr token, const std::string modelName,
                                         const Process& process)
    {
        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->words.push_back(token);
        wc->textForm = token->getWordForm().getRawString();
        wc->pos = {tokenInd, tokenInd, process.m_docNum, process.m_sentNum};
        wc->modelName = modelName;

        return wc;
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
            process.m_output << process.m_docNum << " " << process.m_sentNum << " start_ind = " << wc->pos.start
                             << " end_ind = " << wc->pos.end << "\t||\t" << wc->textForm << "\t||\t" << wc->modelName
                             << std::endl;
        }
    }
}