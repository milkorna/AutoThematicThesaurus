#include "PhrasesCollectorUtils.h"
#include "GrammarPatternManager.h"
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

const std::string GetLowerCase(const std::string& line) {
    // Convert to lowercase using ICU
    icu::UnicodeString ustr(line.c_str(), "UTF-8");
    ustr.toLower(icu::Locale("ru_RU"));
    std::string lowerLine;
    ustr.toUTF8String(lowerLine);
    return lowerLine;
}

void OutputResults(const std::vector<WordComplexPtr>& collection, Process& process) {
    if (collection.empty())
        return;

    auto& morphAnalyzer = MorphAnalyzer::getInstance();

    for (const auto& wc : collection) {
        std::string key;
        for (const auto& w : wc->words) {
            key.append(morphAnalyzer.getLemma(w) + " ");
        }
        if (!key.empty()) {
            key.pop_back();
        }

        json lemmas_json = json::array();
        for (size_t i = 0; i < wc->lemmas.size(); i++) {
            lemmas_json.push_back(std::to_string(i) + "_" + wc->lemmas[i]);
        }

        json j = json::object();
        j["0_key"] = key;
        j["1_textForm"] = wc->textForm;
        j["2_modelName"] = wc->modelName;
        j["3_docId"] = process.docId;
        j["4_sentNum"] = process.sentNum;
        j["5_start_ind"] = wc->pos.start;
        j["6_end_ind"] = wc->pos.end;
        j["7_lemmas"] = lemmas_json;

        process.addJsonObject(j);
    }
    Logger::log("OutputResults", LogLevel::Info, "Appended results to JSON.");
}
