#ifndef STRING_FILTERS_H
#define STRING_FILTERS_H

#include <regex>
#include <string>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <xmorphy/morph/WordForm.h>

namespace StringFilters
{

    /**
     * @brief Checks if the given word form consists only of digits or punctuation symbols
     * @param form Morphological word form pointer
     * @return true if the string contains only digits or punctuation, false otherwise
     */
    bool IsOnlyPunctuationOrDigits(const X::WordFormPtr &form);

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
    bool HasNonCyrillicOrSpecialUnicode(const std::string &str);

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
    bool ShouldBeFiltered(const std::string &str);

} // namespace StringFilters

#endif // STRING_FILTERS_H
