import os
import json
import pandas as pd
from transformers import pipeline
from core.paths import PATH_SENTENCES_WITH_PHRASES, PATH_DATA_WITH_OFF, PATH_HYPERNUM_NLI

MIN_OOF_PROB = 0.1   # если (is_term_manual=0) и (oof_prob < MIN_OOF_PROB) — отбрасываем
REQUIRED_IS_TERM = None  # если =1, тогда берём только is_term_manual=1

# Threshold for "entailment"
HYPER_THRESHOLD = 0.85

# Minimum difference between entailment and contradiction
MIN_DIFF_ENT_CONTR = 0.1

# If True, ensures that neutral < entailment
CHECK_NEUTRAL = True

# Hypothesis templates used to check hypernym/hyponym relations
HYPOTHESIS_TEMPLATES = [
    "{hypo} — это вид {hyper}",
    "{hypo} — это тип {hyper}",
    "{hypo} — это разновидность {hyper}"
]

print("[INFO] Loading NLI (XLM-RoBERTa) as text-classification pipeline...")
nli_model = pipeline(
    task="text-classification",
    model="joeddav/xlm-roberta-large-xnli",
    return_all_scores=True
)
print("[INFO] Model loaded successfully.")


def load_key_mapping(xlsx_path: str):
    """
    Loads an Excel file containing a 'key' column (phrase), along with is_term_manual and oof_prob_class.
    Returns a dictionary in the format:
      {
         phrase_lower: {
             "phrase": str,
             "is_term_manual": int,
             "oof_prob_class": float
         },
         ...
      }
    """
    if not os.path.exists(xlsx_path):
        raise FileNotFoundError(f"Excel file not found: {xlsx_path}")
    df = pd.read_excel(xlsx_path)
    mapping = {}
    for _, row in df.iterrows():
        key_phrase = str(row.get('key', '')).strip()
        if not key_phrase:
            continue
        is_term = row.get('is_term_manual')
        oof_prob = row.get('oof_prob_class')

        mapping[key_phrase.lower()] = {
            "phrase": key_phrase,
            "is_term_manual": is_term,
            "oof_prob_class": oof_prob
        }
    return mapping


def get_key_info(key_str: str, mapping: dict):
    """
    Retrieves phrase information (key_str) from the mapping dictionary.
    Returns a default object with None values if the key is not found.
    """
    return mapping.get(key_str.lower(), {
        "phrase": key_str,
        "is_term_manual": None,
        "oof_prob_class": None
    })


def filter_key(key_obj: dict):
    """
    Filters a phrase object based on:
    - REQUIRED_IS_TERM
    - MIN_OOF_PROB
    Returns True if the key passes the filter criteria.
    """
    is_term = key_obj.get("is_term_manual", None)
    oof_prob = key_obj.get("oof_prob_class", None)

    if is_term == 0 and oof_prob is not None and oof_prob < MIN_OOF_PROB:
        return False
    return True

def tokenize_simple(phrase: str):
    """
    Simple tokenization (splitting by spaces).
    """
    return phrase.lower().split()

def tokens_have_two_or_more_common_words(phrase1: str, phrase2: str):
    """
    Returns True if two phrases (without lemmatization) share at least two common words.
    """
    s1 = set(tokenize_simple(phrase1))
    s2 = set(tokenize_simple(phrase2))
    inters = s1.intersection(s2)
    return len(inters) >= 2

def check_hypernym_nli(hyper: str, hypo: str, context: str = None):
    """
    Checks whether 'hyper' is a hypernym of 'hypo' using multiple hypothesis templates.
    Uses a pre-trained NLI model with text classification pipeline.
    """
    premise = hypo
    details = []

    for templ in HYPOTHESIS_TEMPLATES:
        hypothesis = templ.format(hypo=hypo, hyper=hyper)
        res_batch = nli_model([{"text": premise, "text_pair": hypothesis}], truncation=True)

        if not res_batch:
            continue

        result_for_example = res_batch[0]
        label_score_map = {}
        for item in result_for_example:
            lbl = item["label"].upper()
            sc = item["score"]
            label_score_map[lbl] = sc

        ent = label_score_map.get("ENTAILMENT", 0.0)
        cont = label_score_map.get("CONTRADICTION", 0.0)
        neu = label_score_map.get("NEUTRAL", 0.0)

        details.append({
            "template": hypothesis,
            "ent_score": ent,
            "cont_score": cont,
            "neutral_score": neu
        })

    if not details:
        return {
            "is_hyper": False,
            "avg_ent_score": 0.0,
            "avg_cont_score": 0.0,
            "avg_neutral_score": 0.0,
            "details": []
        }

    avg_ent = sum(d["ent_score"] for d in details) / len(details)
    avg_cont = sum(d["cont_score"] for d in details) / len(details)
    avg_neut = sum(d["neutral_score"] for d in details) / len(details)

    is_hyper = False
    if (avg_ent >= HYPER_THRESHOLD) and ((avg_ent - avg_cont) >= MIN_DIFF_ENT_CONTR):
        is_hyper = True

    if CHECK_NEUTRAL and is_hyper:
        if avg_neut > avg_ent:
            is_hyper = False

    return {
        "is_hyper": is_hyper,
        "avg_ent_score": avg_ent,
        "avg_cont_score": avg_cont,
        "avg_neutral_score": avg_neut,
        "details": details
    }

def main():
    key_mapping = load_key_mapping(PATH_DATA_WITH_OFF)

    if not os.path.exists(PATH_SENTENCES_WITH_PHRASES):
        raise FileNotFoundError(f"File not found: {PATH_SENTENCES_WITH_PHRASES}")

    with open(PATH_SENTENCES_WITH_PHRASES, "r", encoding="utf-8") as f:
        data = json.load(f)

    final_relations = []
    seen_pairs = set()

    for sentence_item in data.get("sentences", []):
        keys_in_sent = sentence_item.get("keys", [])
        if len(keys_in_sent) < 2:
            continue

        context_text = sentence_item.get("normalizedStr", "")
        doc_num = sentence_item.get("docNum")
        sent_num = sentence_item.get("sentNum")

        n = len(keys_in_sent)
        for i in range(n):
            for j in range(i+1, n):
                k1_info = get_key_info(keys_in_sent[i], key_mapping)
                k2_info = get_key_info(keys_in_sent[j], key_mapping)

                phr1 = k1_info["phrase"]
                phr2 = k2_info["phrase"]

                if not phr1 or not phr2 or (phr1 == phr2):
                    continue

                if not filter_key(k1_info):
                    continue
                if not filter_key(k2_info):
                    continue

                both_not_term = (k1_info["is_term_manual"] == 0 and k2_info["is_term_manual"] == 0)

                both_oof_low = (
                    k1_info["oof_prob_class"] is not None and
                    k2_info["oof_prob_class"] is not None and
                    k1_info["oof_prob_class"] < 0.4 and
                    k2_info["oof_prob_class"] < 0.4
                )

                if both_not_term or both_oof_low:
                    continue

                one_not_term_low = (
                    (k1_info["is_term_manual"] == 0 and
                    k1_info["oof_prob_class"] is not None and
                    k1_info["oof_prob_class"] < 0.3)
                    or
                    (k2_info["is_term_manual"] == 0 and
                    k2_info["oof_prob_class"] is not None and
                    k2_info["oof_prob_class"] < 0.3)
                )
                if one_not_term_low:
                    continue

                if tokens_have_two_or_more_common_words(phr1, phr2):
                    continue

                len1 = len(tokenize_simple(phr1))
                len2 = len(tokenize_simple(phr2))

                if len1 <= len2:
                    hyper_obj, hypo_obj = k1_info, k2_info
                else:
                    hyper_obj, hypo_obj = k2_info, k1_info

                hyper = hyper_obj["phrase"]
                hypo = hypo_obj["phrase"]

                check_res = check_hypernym_nli(hyper=hyper, hypo=hypo, context=context_text)
                if check_res["is_hyper"]:
                    pair_key = (
                        doc_num,
                        sent_num,
                        hyper_obj["phrase"],
                        hypo_obj["phrase"]
                    )

                    if pair_key in seen_pairs:
                        continue
                    seen_pairs.add(pair_key)

                    final_relations.append({
                        "docNum": doc_num,
                        "sentNum": sent_num,
                        "sentence": context_text,
                        "hyper": hyper_obj,
                        "hypo": hypo_obj,
                        "avg_ent_score": check_res["avg_ent_score"],
                        "avg_cont_score": check_res["avg_cont_score"],
                        "avg_neutral_score": check_res["avg_neutral_score"]
                    })

    output_data = {"relations": final_relations}
    with open(PATH_HYPERNUM_NLI, "w", encoding="utf-8") as f:
        json.dump(output_data, f, ensure_ascii=False, indent=4)

    print(f"[INFO] Всего найдено {len(final_relations)} отношений гипероним/гипоним.")
    print(f"[INFO] Результаты сохранены в {PATH_HYPERNUM_NLI}")

if __name__ == "__main__":
    main()
