#include "StringFilters.h"

namespace StringFilters
{

    namespace
    {
        constexpr std::string_view PUNCTUATION = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)";
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

        // Forbidden ASCII symbols
        static const std::string forbidden = "%*_$#";
        if (str.find_first_of(forbidden) != std::string::npos)
            return true;

        // Only punctuation / non-alphabetic (excluding Russian letters)
        if (std::regex_match(str, std::regex(R"(^[^\wа-яА-ЯёЁa-zA-Z¨]+$)")))
            return true;

        // Long Latin/digit/punct-only sequences
        if (str.size() > 25 &&
            std::regex_match(str, std::regex(R"(^[a-zA-Z0-9[:punct:]]+$)")))
            return true;

        // Unicode-level unwanted characters
        if (HasNonCyrillicOrSpecialUnicode(str))
            return true;

        return false;
    }

} // namespace StringFilters
