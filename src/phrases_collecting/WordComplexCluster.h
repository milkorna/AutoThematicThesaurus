

#include "Embedding.h"
#include "LSA.h"
#include "PhrasesCollectorUtils.h"
#include "ThreadController.h"

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// \struct WordComplexCluster
// \brief This structure represents a cluster of word complexes, including their TF, IDF, and TF-IDF values, as well
// as FastText vectors.
struct WordComplexCluster {
    size_t phraseSize; ///< Size of the phrase.
    bool tagMatch;     ///< Indicates if the cluster matches a topic.
    double frequency;
    double topicRelevance;
    double centralityScore;
    std::string key;       ///< String with normalized words.
    std::string modelName; ///< Name of the model associated with the cluster.
    std::vector<std::string> lemmas;
    std::vector<WordComplexPtr> wordComplexes;                        ///< Vector of word complexes in the cluster.
    std::vector<double> tf;                                           ///< Vector of TF values for the words.
    std::vector<double> idf;                                          ///< Vector of IDF values for the words.
    std::vector<double> tfidf;                                        ///< Vector of TF-IDF values for the words.
    std::vector<WordEmbeddingPtr> wordVectors;                        ///< Vector of FastText vectors for the words.
    std::unordered_map<std::string, std::set<std::string>> hypernyms; ///< Hypernyms for each word in the phrase.
    std::unordered_map<std::string, std::set<std::string>> hyponyms;  ///< Hyponyms for each word in the phrase.
    std::unordered_set<std::string> synonyms;
    std::vector<TokenizedSentence> contexts;
    bool is_term;
};