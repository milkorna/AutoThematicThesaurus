#pragma once

#include <string>

namespace StringFilters {
/**
 * @brief Checks if a string consists only of digits or punctuation
 * @param text Input string
 * @return true if the string contains only digits or punctuation, false otherwise
 */
[[nodiscard]] bool isOnlyPunctuationOrDigits(const std::string& text);

/**
 * @brief Checks if the given string contains characters not suitable for Russian text
 *
 * Flags as unwanted:
 *  - Digits
 *  - Emoji / pictographic symbols
 *  - Han (Chinese), Devanagari, or Arabic scripts
 *  - Mathematical or other technical symbols
 *
 * @param str Input UTF-8 string
 * @return true if unwanted characters are present
 */
[[nodiscard]] bool hasNonCyrillicOrSpecialUnicode(const std::string& str);

/**
 * @brief Determines if a string should be filtered out as invalid
 *
 * Filter criteria:
 *  - Contains forbidden symbols: %, *, _, #, $
 *  - Consists only of punctuation or non-alphabetic characters
 *  - Is a long (>25 chars) English/digit/punctuation sequence
 *  - Contains foreign scripts, digits, emoji, or math symbols
 *
 * @param str Input string
 * @return true if the string should be filtered out
 */
[[nodiscard]] bool shouldBeFiltered(const std::string& str);

} // namespace StringFilters
