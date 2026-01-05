#include "TextCorpusFilter.h"

#include <algorithm>

void TextCorpusFilter::filterTextsByLength(TextCorpus& corpus, size_t minLength, bool requireSpaces) {
    Logger::log("TextCorpusFilter", LogLevel::Info,
                "Filtering texts by length (min: " + std::to_string(minLength) + ")...");

    size_t originalCount = corpus.getTextCount();
    auto& texts = corpus.getTextsForModification();

    for (auto& [filename, textList] : texts) {
        auto newEnd =
            std::remove_if(textList.begin(), textList.end(), [minLength, requireSpaces](const std::string& text) {
                bool tooShort = text.length() < minLength;
                bool missingSpaces = requireSpaces && text.find(' ') == std::string::npos;
                return tooShort || missingSpaces;
            });
        textList.erase(newEnd, textList.end());
    }

    recalculateStatistics(corpus);

    Logger::log("TextCorpusFilter", LogLevel::Info,
                "Texts filtered: " + std::to_string(originalCount) + " → " + std::to_string(corpus.getTextCount()));
}

void TextCorpusFilter::filterStopWords(TextCorpus& corpus) {
    Logger::log("TextCorpusFilter", LogLevel::Info, "Filtering stop words...");

    auto& wordFreq = corpus.getWordFrequenciesForModification();
    auto& docFreq = corpus.getDocumentFrequenciesForModification();

    size_t originalCount = wordFreq.size();

    auto it = wordFreq.begin();
    while (it != wordFreq.end()) {
        if (StringFilters::ShouldBeFiltered(it->first)) {
            docFreq.erase(it->first);
            it = wordFreq.erase(it);
        } else {
            ++it;
        }
    }

    recalculateStatistics(corpus);

    Logger::log("TextCorpusFilter", LogLevel::Info,
                "Stop words filtered: " + std::to_string(originalCount) + " → " + std::to_string(wordFreq.size()));
}

void TextCorpusFilter::recalculateStatistics(TextCorpus& corpus) {
    Logger::log("TextCorpusFilter", LogLevel::Debug, "Recalculating corpus statistics...");
    corpus.recalculateStatistics();
}