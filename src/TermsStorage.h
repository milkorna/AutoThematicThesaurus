#ifndef TERMS_STORAGE_H
#define TERMS_STORAGE_H

#include <PatternPhrasesStorage.h>

struct TermCluster : public WordComplexCluster {
    std::vector<WordComplexCluster> variants;
};

class TermsStorage {
public:
    static TermsStorage& GetStorage()
    {
        static TermsStorage storage;
        return storage;
    }

    void CollectTerms(PatternPhrasesStorage& phrases);

private:
    // \brief Default constructor.
    TermsStorage()
    {
    }

    // \brief Default destructor.
    ~TermsStorage()
    {
    }

    // \brief Deleted copy constructor to enforce singleton pattern.
    TermsStorage(const TermsStorage&) = delete;

    // \brief Deleted assignment operator to enforce singleton pattern.
    TermsStorage& operator=(const TermsStorage&) = delete;
    std::unordered_map<std::string, TermCluster> clusters;
};

#endif // PATTERN_PHRASES_STORAGE_H
