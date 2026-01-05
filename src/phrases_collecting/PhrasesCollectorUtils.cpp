#include "PhrasesCollectorUtils.h"
#include "GrammarPatternManager.h"
#include "Logger.h"
#include "MorphAnalyzer.h"

#include "xmorphy/utils/UniString.h"
#include <boost/program_options.hpp>
#include <cctype>
#include <nlohmann/json.hpp>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>

using json = nlohmann::json;

bool MorphAnanlysisError(const X::WordFormPtr& token) {
    auto isDesiredPOS = [](const X::UniSPTag& tag) -> bool {
        static const std::unordered_set<std::string> desiredPOS = {"ADJ", "NOUN", "PROPN", "VERB"};
        return desiredPOS.find(tag.toString()) != desiredPOS.end();
    };

    return token->getWordForm().length() == 1 && token->getMorphInfo().size() == 1 &&
           isDesiredPOS(token->getMorphInfo().begin()->sp);
}

bool HaveSp(const std::unordered_set<X::MorphInfo>& currFormMorphInfo) {
    const auto& manager = GrammarPatternManager::GetManager();

    for (const auto& morphForm : currFormMorphInfo) {
        const auto& spSet = manager.getUsedSp();
        if (spSet.find(morphForm.sp.toString()) != spSet.end())
            return true;
    }
    return false;
}
