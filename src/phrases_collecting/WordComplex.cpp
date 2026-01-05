#include "WordComplex.h"
#include "MorphAnalyzer.h"

bool WordComplex::operator==(const WordComplex& other) const {
    if (modelName != other.modelName || words.size() != other.words.size())
        return false;

    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (size_t i = 0; i < words.size(); ++i) {
        if (morphAnalyzer.getNormalForm(words[i]) != morphAnalyzer.getNormalForm(other.words[i])) {
            return false;
        }
        if (lemmas[i] != other.lemmas[i])
            return false;
    }

    return true;
}

const std::string WordComplex::getKey() const {
    std::string key;
    for (const auto& l : lemmas) {
        key.append(l + " ");
    }
    key.pop_back();
    return key;
}

WordComplexPtr initializeWordComplex(const WordComplexPtr& curSimplePhr, const std::string& modelName) {
    WordComplexPtr wc = std::make_shared<WordComplex>();
    wc->words = curSimplePhr->words;
    wc->lemmas = curSimplePhr->lemmas;
    wc->textForm = curSimplePhr->textForm;
    wc->pos = curSimplePhr->pos;
    wc->modelName = modelName;
    return wc;
}

WordComplexPtr initializeWordComplex(const size_t tokenInd, const X::WordFormPtr token, const std::string modelName,
                                     const Process& process) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();
    WordComplexPtr wc = std::make_shared<WordComplex>();
    wc->words.push_back(token);
    wc->lemmas.push_back(morphAnalyzer.getLemma(token));
    wc->textForm = token->getWordForm().getRawString();
    wc->pos = {tokenInd, tokenInd, process.docId, process.sentNum};
    wc->modelName = modelName;

    return wc;
}

void updateWordComplex(const WordComplexPtr& wc, const X::WordFormPtr& form, const std::string& formFromText,
                       bool isLeft) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    if (isLeft) {
        wc->words.push_front(form);
        wc->lemmas.push_front(morphAnalyzer.getLemma(form));
        wc->pos.start--;
        wc->textForm.insert(0, formFromText + " ");
    } else {
        wc->words.push_back(form);
        wc->lemmas.push_back(morphAnalyzer.getLemma(form));
        wc->pos.end++;
        wc->textForm.append(" " + formFromText);
    }
}

void addWordsToFront(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();
    for (auto rit = asidePhrase->words.rbegin(); rit != asidePhrase->words.rend(); ++rit) {
        wc->words.push_front(*rit);
        wc->lemmas.push_front(morphAnalyzer.getLemma(*rit));
    }
}

void addWordsToBack(const WordComplexPtr& wc, const WordComplexPtr& asidePhrase) {
    auto& morphAnalyzer = MorphAnalyzer::getInstance();
    for (const auto& word : asidePhrase->words) {
        wc->words.push_back(word);
        wc->lemmas.push_back(morphAnalyzer.getLemma(word));
    }
}