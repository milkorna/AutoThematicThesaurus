#include <PhrasesCollectorUtils.h>
#include <WordComplex.h>

namespace PhrasesCollectorUtils {

    bool WordComplex::operator==(const WordComplex& other) const
    {
        if (modelName != other.modelName || words.size() != other.words.size())
            return false;

        for (size_t i = 0; i < words.size(); ++i) {
            if (GetMostProbableMorphInfo(words[i]->getMorphInfo()).normalForm !=
                GetMostProbableMorphInfo(other.words[i]->getMorphInfo()).normalForm) {
                return false;
            }
            if (lemmas[i] != other.lemmas[i])
                return false;
        }

        return true;
    }

    const std::string WordComplex::GetKey() const
    {
        std::string key;
        for (const auto& l : lemmas) {
            key.append(l + " ");
        }
        key.pop_back();
        return key;
    }

    WordComplexPtr InicializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName)
    {
        WordComplexPtr wc = std::make_shared<WordComplex>();
        wc->words = curSimplePhr->words;
        wc->lemmas = curSimplePhr->lemmas;
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
        wc->lemmas.push_back(GetLemma(token));
        wc->textForm = token->getWordForm().getRawString();
        wc->pos = {tokenInd, tokenInd, process.m_docNum, process.m_sentNum};
        wc->modelName = modelName;

        return wc;
    }

    void UpdateWordComplex(const WordComplexPtr& wc, const WordFormPtr& form, const std::string& formFromText,
                           bool isLeft)
    {
        if (isLeft) {
            wc->words.push_front(form);
            wc->lemmas.push_back(GetLemma(form));
            wc->pos.start--;
            wc->textForm.insert(0, formFromText + " ");
        } else {
            wc->words.push_back(form);
            wc->lemmas.push_back(GetLemma(form));
            wc->pos.end++;
            wc->textForm.append(" " + formFromText);
        }
    }

    void AddWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (auto rit = asidePhrase->words.rbegin(); rit != asidePhrase->words.rend(); ++rit) {
            wc->words.push_front(*rit);
            wc->lemmas.push_front(GetLemma(*rit));
        }
    }

    void AddWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase)
    {
        for (const auto& word : asidePhrase->words) {
            wc->words.push_back(word);
            wc->lemmas.push_back(GetLemma(word));
        }
    }
}