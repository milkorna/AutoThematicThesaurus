#ifndef STRING_FILTERS_H
#define STRING_FILTERS_H

#include <boost/algorithm/string.hpp>
#include <regex>
#include <string>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/uscript.h>
#include <unicode/ustream.h>
#include <unicode/utypes.h>
#include <xmorphy/morph/WordForm.h>

namespace StringFilters {

    bool CheckForMisclassifications(const X::WordFormPtr& form);

    // This function checks whether a given text contains any unwanted characters.
    // The function uses ICU to handle Unicode strings and to check properties of each character.
    bool ContainsUnwantedCharacters(const std::string& str);

    // This function checks whether a given str should be filtered out based on various conditions
    bool ShouldFilterOut(const std::string& str);
}

#endif // STRING_FILTERS_H