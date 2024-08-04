#ifndef PHRASES_COLLECTOR_UTILS_H
#define PHRASES_COLLECTOR_UTILS_H

#include <xmorphy/morph/WordForm.h>

#include <ModelComponent.h>
#include <PatternParser.h>
#include <PhrasesCollectorUtils.h>

#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace PhrasesCollectorUtils {

    MorphInfo GetMostProbableMorphInfo(const std::unordered_set<X::MorphInfo>& morphSet);
    bool MorphAnanlysisError(const WordFormPtr& token);
    bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo);
    const std::unordered_set<std::string> GetTopics();
    const std::unordered_set<std::string> GetStopWords();

    struct Position {
        size_t start;
        size_t end;
        size_t docNum;
        size_t sentNum;
    };

    struct WordComplex {
        std::deque<X::WordFormPtr> words = {};
        std::string textForm = "";
        Position pos;
        std::string modelName;

        bool operator==(const WordComplex& other) const
        {
            if (modelName != other.modelName) {
                return false;
            }

            if (words.size() != other.words.size()) {
                return false;
            }

            for (size_t i = 0; i < words.size(); ++i) {

                if (GetMostProbableMorphInfo(words[i]->getMorphInfo()).normalForm !=
                    GetMostProbableMorphInfo(other.words[i]->getMorphInfo()).normalForm) {
                    return false;
                }
            }

            return true;
        }

        std::string GetKey()
        {
            std::string key;
            for (const auto& w : words) {
                key.append(GetMostProbableMorphInfo(w->getMorphInfo()).normalForm.getRawString());
            }
            return key;
        }
    };

    using WordComplexPtr = std::shared_ptr<WordComplex>;

    struct CurrentPhraseStatus {
        size_t correct = 0;
        bool headIsMatched = false;
        bool headIsChecked = false;
        bool foundLex = false;
    };

    void LogCurrentSimplePhrase(const WordComplexPtr& curSimplePhr);
    void LogCurrentComplexModel(const std::string& name);

    void UpdateWordComplex(const WordComplexPtr& wc, const X::WordFormPtr& form, const std::string& formFromText,
                           bool isLeft);
    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase);
    void UpdatePhraseStatus(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase,
                            CurrentPhraseStatus& curPhrStatus, bool isLeft);

    WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName);
    WordComplexPtr InicializeWordComplex(const size_t tokenInd, const WordFormPtr token, const std::string modelName,
                                         const Process& process);

    bool CheckForMisclassifications(const X::WordFormPtr& form);

    void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);
}

namespace PHUtils = PhrasesCollectorUtils;

#endif // PHRASES_COLLECTOR_UTILS_H