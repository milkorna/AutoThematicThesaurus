#pragma once

#include <string>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <xmorphy/morph/WordForm.h>

namespace StringFilters {
/**
 * @brief Checks if a string consists only of digits or punctuation
 * @param text Input string
 * @return true if the string contains only digits or punctuation, false otherwise
 */
bool IsOnlyPunctuationOrDigits(const std::string& text);

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
bool HasNonCyrillicOrSpecialUnicode(const std::string& str);

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
bool ShouldBeFiltered(const std::string& str);

} // namespace StringFilters
