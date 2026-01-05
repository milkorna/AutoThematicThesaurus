#include "unicode/uchar.h"
#include "unicode/unistr.h"
#include "unicode/uscript.h"

#include "StringUtils.h"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <regex>
#include <string>
#include <unicode/locid.h>

namespace StringUtils {

namespace {
constexpr std::string_view PUNCTUATION = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)";
constexpr std::string_view FORBIDDEN = "%*_$#";

// ------------------------------------------------------------
// Helpers for ShouldBeFiltered()
// ------------------------------------------------------------
bool containsForbiddenSymbols(const std::string& str) {
    return str.find_first_of(FORBIDDEN) != std::string::npos;
}

bool isOnlyPunctuationOrNonAlpha(const std::string& str) {
    // True if consists entirely of punctuation or non-alphabetic symbols
    static const std::regex re(R"(^[^\wа-яА-ЯёЁa-zA-Z¨]+$)");
    return std::regex_match(str, re);
}

bool isLongLatinGarbage(const std::string& str) {
    // Strings longer than 25 made only of English letters, digits and punctuation
    if (str.size() <= 25)
        return false;

    static const std::regex re(R"(^[a-zA-Z0-9[:punct:]]+$)");
    return std::regex_match(str, re);
}
} // namespace

// ------------------------------------------------------------
// 1. Basic character composition checks
// ------------------------------------------------------------
bool isOnlyPunctuationOrDigits(const std::string& text) {
    if (text.empty())
        return false;

    for (unsigned char c : text) {
        if (!std::isdigit(c) && PUNCTUATION.find(c) == std::string_view::npos) {
            return false;
        }
    }

    return true;
}

// ------------------------------------------------------------
// 2. Unicode character property checks (ICU)
// ------------------------------------------------------------
bool hasNonCyrillicOrSpecialUnicode(const std::string& str) {
    icu::UnicodeString utext = icu::UnicodeString::fromUTF8(str);

    for (int32_t i = 0; i < utext.length(); ++i) {
        UChar32 codepoint = utext.char32At(i);

        if (u_isdigit(codepoint))
            return true;

        if (u_hasBinaryProperty(codepoint, UCHAR_EXTENDED_PICTOGRAPHIC))
            return true;

        auto script = static_cast<UScriptCode>(u_getIntPropertyValue(codepoint, UCHAR_SCRIPT));

        switch (script) {
        case USCRIPT_HAN:
        case USCRIPT_DEVANAGARI:
        case USCRIPT_ARABIC:
            return true;
        default:
            break;
        }

        auto ctype = u_charType(codepoint);
        if (ctype == U_MATH_SYMBOL || ctype == U_OTHER_SYMBOL)
            return true;
    }

    return false;
}

// ------------------------------------------------------------
// 3. High-level filtering rules
// ------------------------------------------------------------
bool shouldBeFiltered(const std::string& str) {
    if (str.empty())
        return true;

    return containsForbiddenSymbols(str) || isOnlyPunctuationOrNonAlpha(str) || isLongLatinGarbage(str) ||
           hasNonCyrillicOrSpecialUnicode(str);
}

std::string toLowerCase(const std::string& line) {
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(line);
    ustr.toLower(icu::Locale("ru_RU"));
    std::string result;
    ustr.toUTF8String(result);
    return result; // RVO оптимизирует копию
}

bool containsNoLatin(const std::string& str) {
    for (char ch : str) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            return false;
        }
    }
    return true;
}

std::string trimTrailingDigitsAndSpaces(const std::string& line) {
    auto notDigitOrSpace = [](unsigned char ch) { return !std::isdigit(ch) && !std::isspace(ch); };

    auto it = std::ranges::find_if_not(line | std::views::reverse, notDigitOrSpace).base();

    return std::string(line.begin(), it);
}

} // namespace StringUtils
