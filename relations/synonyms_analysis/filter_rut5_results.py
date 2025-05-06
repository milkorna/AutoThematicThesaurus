import json
import re
import pymorphy2
from pathlib import Path
from core.paths import SYNONYMS_DIR

import inspect

# Patch for pymorphy2 compatibility with Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults
    inspect.getargspec = getargspec_patched


# Paths
INPUT_PATH = SYNONYMS_DIR / "rut5_base_paraphraser.json"
OUTPUT_PATH = SYNONYMS_DIR / "synonyms_rut5_base_paraphraser.json"

# Initialize pymorphy2
morph = pymorphy2.MorphAnalyzer()

def get_pos(word):
    return morph.parse(word)[0].tag.POS

def is_adjective(word):
    return get_pos(word) in {"ADJF", "ADJS"}

def is_noun(word):
    return get_pos(word) == "NOUN"

def should_filter_pair(original_phrase, candidate_phrase):
    orig_tokens = original_phrase.lower().strip().split()
    cand_tokens = candidate_phrase.lower().strip().split()

    if len(orig_tokens) != 2 or len(cand_tokens) != 2:
        return False

    orig_pos = [get_pos(w) for w in orig_tokens]
    cand_pos = [get_pos(w) for w in cand_tokens]

    # Look for NOUN + NOUN -> ADJ + NOUN
    if orig_pos[0] == "NOUN" and orig_pos[1] == "NOUN" and \
       cand_pos[0] in {"ADJF", "ADJS"} and cand_pos[1] == "NOUN":

        noun_lemma = morph.parse(orig_tokens[0])[0].normal_form
        adj_lemma = morph.parse(cand_tokens[0])[0].normal_form

        # Heuristic: adjective lemma starts with noun lemma root
        if adj_lemma.startswith(noun_lemma[:5]):  # Allow some prefix flexibility
            print(f"[FILTER] '{original_phrase}' -> '{candidate_phrase}' | noun='{noun_lemma}' ~ adj='{adj_lemma}'")
            return True

    return False


def filter_rut5_file(input_path, output_path):
    with open(input_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    new_data = {}

    for phrase, content in data.items():
        orig_phrase = phrase.lower().strip()

        # For top_paraphrases: keep all, just filter the values
        top_filtered = [
            p for p in content.get("top_paraphrases", [])
            if not should_filter_pair(orig_phrase, p)
        ]

        # For found_in_data: if any should be filtered — skip whole entry
        filtered_out = any(
            should_filter_pair(orig_phrase, d.get("key", ""))
            for d in content.get("found_in_data", [])
        )

        if filtered_out:
            print(f"[REMOVE] Full entry for '{phrase}' removed due to bad found_in_data.")
            continue

        # Always keep if at least one of the lists is non-empty
        if top_filtered or content.get("found_in_data", []):
            new_entry = content.copy()
            new_entry["top_paraphrases"] = top_filtered
            new_data[phrase] = new_entry

    # Save the filtered results
    with open(output_path, "w", encoding="utf-8") as out:
        json.dump(new_data, out, ensure_ascii=False, indent=4)

    print(f"[INFO] Filtered data saved to {output_path}")

if __name__ == "__main__":
    filter_rut5_file(INPUT_PATH, OUTPUT_PATH)
