#include <StringFilters.h>

namespace StringFilters {

    bool CheckForMisclassifications(const X::WordFormPtr& form)
    {
        std::unordered_set<char> punctuation = {'!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+',
                                                ',', '-',  '.', '/', ':', ';', '<',  '=', '>', '?', '@',
                                                '[', '\\', ']', '^', '_', '`', '{',  '|', '}', '~'};

        try {
            const auto str = form->getWordForm().getRawString();

            for (char c : str) {
                if (!std::isdigit(c) && punctuation.find(c) == punctuation.end())
                    return false;
            }
            return true;
        } catch (const std::exception& e) {
            return false;
        } catch (...) {
            return false;
        }

        return true;
    }

    // This function checks whether a given text contains any unwanted characters.
    // The function uses ICU to handle Unicode strings and to check properties of each character.
    bool ContainsUnwantedCharacters(const std::string& str)
    {
        // Convert the input UTF-8 string to an ICU UnicodeString for processing
        icu::UnicodeString unicodeText = icu::UnicodeString::fromUTF8(str);

        // Iterate through each character in the UnicodeString
        for (int32_t i = 0; i < unicodeText.length(); ++i) {
            UChar32 codepoint = unicodeText.char32At(i);

            // Check if the character is a digit
            if (u_isdigit(codepoint)) {
                return true;
            }

            // Check if the character is an emoji using the extended pictographic property
            if (u_hasBinaryProperty(codepoint, UCHAR_EXTENDED_PICTOGRAPHIC)) {
                return true;
            }

            // Check if the character is a Chinese ideograph (Han script)
            if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_HAN) {
                return true;
            }

            // Check if the character is part of the Devanagari script
            if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_DEVANAGARI) {
                return true;
            }

            // Check if the character is part of the Arabic script
            if (u_getIntPropertyValue(codepoint, UCHAR_SCRIPT) == USCRIPT_ARABIC) {
                return true;
            }

            // Check if the character is a mathematical or technical symbol
            if (u_charType(codepoint) == U_MATH_SYMBOL || u_charType(codepoint) == U_OTHER_SYMBOL) {
                return true;
            }
        }

        return false;
    }

    // This function checks whether a given str should be filtered out based on various conditions
    bool ShouldFilterOut(const std::string& str)
    {
        // Elements that contain %, *, _, #, or $
        if (str.find('%') != std::string::npos || str.find('*') != std::string::npos ||
            str.find('_') != std::string::npos || str.find('#') != std::string::npos ||
            str.find('$') != std::string::npos) {
            return true;
        }

        // Elements that consist entirely of punctuation or non-alphabetic symbols, excluding Russian letters
        if (std::regex_match(str, std::regex("^[^\\wа-яА-ЯёЁa-zA-Z¨]+$"))) {

            return true;
        }

        // Elements consisting of only English letters, punctuation, and digits and are longer than 25 characters
        if (str.size() > 25 && std::regex_match(str, std::regex("^[a-zA-Z0-9[:punct:]]+$"))) {
            return true;
        }

        // Use the ContainsUnwantedCharacters function to check for other unwanted characters
        if (ContainsUnwantedCharacters(str)) {
            return true;
        }

        // If none of the conditions match, the str is not filtered out
        return false;
    }
}
