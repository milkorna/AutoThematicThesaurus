#ifndef PATTERN_PHRASES_STORAGE_H
#define PATTERN_PHRASES_STORAGE_H

#include <ComplexPhrasesCollector.h>

#include <iostream>
#include <mutex>

class PatternPhrasesStorage {
public:
    static PatternPhrasesStorage& GetStorage()
    {
        static PatternPhrasesStorage starage;
        return starage;
    }

    void Collect(const std::vector<WordFormPtr>& forms, Process& process)
    {
        SimplePhrasesCollector::GetCollector().Collect(forms, process);
        ComplexPhrasesCollector::GetCollector().Collect(forms, process);

        SimplePhrasesCollector::GetCollector().Clear();
        ComplexPhrasesCollector::GetCollector().Clear();
    }

    void addPhrase(const std::string& phrase)
    {
        phrases.push_back(phrase);
    }

    const std::vector<std::string>& getPhrases() const
    {
        return phrases;
    }

private:
    PatternPhrasesStorage()
    {
    }
    ~PatternPhrasesStorage()
    {
    }
    PatternPhrasesStorage(const PatternPhrasesStorage&) = delete;
    PatternPhrasesStorage& operator=(const PatternPhrasesStorage&) = delete;

    std::vector<std::string> phrases;
};

#endif