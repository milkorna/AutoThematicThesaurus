#pragma once

#include <cstddef>

/**
 * @brief Status tracking for phrase components during grammar pattern matching
 */
struct PhraseMatchStatus {
    size_t matchedComponents = 0;
    bool headMatched = false;
    bool headValidated = false;
    bool lexFound = false;

    // Ясная валидация состояния
    bool isValid() const {
        return headValidated && headMatched && lexFound;
    }

    void reset() {
        matchedComponents = 0;
        headMatched = false;
        headValidated = false;
        lexFound = false;
    }
};
