#include "StringFilters.h"

namespace StringFilters
{

    namespace
    {
        constexpr std::string_view PUNCTUATION = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)";
        constexpr std::string_view FORBIDDEN = "%*_$#";

        // ------------------------------------------------------------
        // Helpers for ShouldBeFiltered()
        // ------------------------------------------------------------
        bool ContainsForbiddenSymbols(const std::string &str)
        {
            return str.find_first_of(FORBIDDEN) != std::string::npos;
        }

        bool IsOnlyPunctuationOrNonAlpha(const std::string &str)
        {
            // True if consists entirely of punctuation or non-alphabetic symbols
            static const std::regex re(R"(^[^\wа-яА-ЯёЁa-zA-Z¨]+$)");
            return std::regex_match(str, re);
        }

        bool IsLongLatinGarbage(const std::string &str)
        {
            // Strings longer than 25 made only of English letters, digits and punctuation
            if (str.size() <= 25)
                return false;

            static const std::regex re(R"(^[a-zA-Z0-9[:punct:]]+$)");
            return std::regex_match(str, re);
        }
    }

    // ------------------------------------------------------------
    // 1. Basic character composition checks
    // ------------------------------------------------------------
    bool IsOnlyPunctuationOrDigits(const X::WordFormPtr &form)
    {
        if (!form)
            return false;

        try
        {
            const auto &raw = form->getWordForm().getRawString();
            if (raw.empty())
                return false;

            for (char c : raw)
            {
                if (!std::isdigit(static_cast<unsigned char>(c)) &&
                    PUNCTUATION.find(c) == std::string_view::npos)
                {
                    return false;
                }
            }
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    // ------------------------------------------------------------
    // 2. Unicode character property checks (ICU)
    // ------------------------------------------------------------
    bool HasNonCyrillicOrSpecialUnicode(const std::string &str)
    {
        icu::UnicodeString utext = icu::UnicodeString::fromUTF8(str);

        for (int32_t i = 0; i < utext.length(); ++i)
        {
            UChar32 codepoint = utext.char32At(i);

            if (u_isdigit(codepoint))
                return true;

            if (u_hasBinaryProperty(codepoint, UCHAR_EXTENDED_PICTOGRAPHIC))
                return true;

            auto script = static_cast<UScriptCode>(
                u_getIntPropertyValue(codepoint, UCHAR_SCRIPT));

            switch (script)
            {
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
    bool ShouldBeFiltered(const std::string &str)
    {
        if (str.empty())
            return true;

        return ContainsForbiddenSymbols(str) ||
               IsOnlyPunctuationOrNonAlpha(str) ||
               IsLongLatinGarbage(str) ||
               HasNonCyrillicOrSpecialUnicode(str);
    }

} // namespace StringFilters
