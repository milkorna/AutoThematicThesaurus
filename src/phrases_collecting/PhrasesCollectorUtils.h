#pragma once

#include "WordComplex.h"

#include "xmorphy/morph/WordForm.h"

#include <unordered_set>

// \struct CurrentPhraseStatus
// \brief This structure holds the status of the current phrase during processing.
struct CurrentPhraseStatus {
    size_t correct = 0;         ///< Number of correctly identified components in the phrase.
    bool headIsMatched = false; ///< Indicates if the head of the phrase is matched.
    bool headIsChecked = false; ///< Indicates if the head of the phrase is checked.
    bool foundLex = false;      ///< Indicates if a lexical item was found.
};

// \brief Checks if there is an error in morphological analysis.
// \param token         The WordFormPtr token to check.
// \return              True if there is an error, false otherwise.
bool MorphAnanlysisError(const X::WordFormPtr& token);

// \brief Checks if the current form has a specific morphological property.
// \param currFormMorphInfo A set of morphological information of the current form.
// \return                  True if the property is found, false otherwise.
bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo);

const std::string GetLowerCase(const std::string& line);

// \brief Outputs the results of the phrase collection process.
// \param collection    A vector of collected word complexes.
// \param process       The process associated with the phrase collection.
void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process);
