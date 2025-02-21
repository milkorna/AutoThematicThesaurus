import os
import json
import pandas as pd

# File paths
FILTERED_DATA_XLSX = r"/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
ENRICHED_SENTENCES_PATH = r"/home/milkorna/Documents/AutoThematicThesaurus/my_data/enriched_sentences.json"
OUTPUT_JSON_PATH = r"/home/milkorna/Documents/AutoThematicThesaurus/hyponym_hyponym_analysis/extract_relations_by_triggers.json"

# List of triggers (lowercase)
TRIGGERS = ["это метод", "это алгоритм", "это процесс", "это техника", "это концепция", "относится к классу"]

def load_key_mapping(xlsx_path):
    """
    Loads an Excel file and returns a dictionary:
      { key_phrase_lower: {"phrase": key_phrase, "is_term_manual": ..., "oof_prob_class": ...}, ... }
    """
    df = pd.read_excel(xlsx_path)
    mapping = {}
    for _, row in df.iterrows():
        key_phrase = str(row.get('key')).strip()
        if not key_phrase:
            continue
        mapping[key_phrase.lower()] = {
            "phrase": key_phrase,
            "is_term_manual": row.get('is_term_manual'),
            "oof_prob_class": row.get('oof_prob_class')
        }
    return mapping

def get_key_info(key, key_mapping):
    """
    Returns key information: phrase, is_term_manual, oof_prob_class.
    If the key is not found in the mapping, returns an object with the key itself and None values.
    """
    return key_mapping.get(key.lower(), {"phrase": key, "is_term_manual": None, "oof_prob_class": None})

def extract_within_sentence(sentence, key_mapping, trigger, trigger_pos):
    """
    Extracts relations if the trigger is not at the beginning of the sentence.
    Logic:
      - Subject: key phrases whose last occurrence (rfind) is before the trigger.
      - Predicate: key phrases whose first occurrence is after the trigger,
        ensuring no more than two words between the trigger and phrase start.
    Returns a list of relation objects:
      { "subject": {...}, "predicate": {...} }
    """
    relations = []
    normalized = sentence.get("normalizedStr", "")
    lower_sentence = normalized.lower()
    subject_range_end = trigger_pos
    predicate_range_start = trigger_pos + len(trigger)

    left_candidates = []   # (key, pos)
    right_candidates = []  # (key, pos)

    for key in sentence.get("keys", []):
        key_lower = key.lower()
        # Find last occurrence before trigger for subject
        pos_left = lower_sentence.rfind(key_lower, 0, subject_range_end)
        if pos_left != -1:
            candidate_end = pos_left + len(key_lower)
            gap_left = lower_sentence[candidate_end: subject_range_end]
            if len(gap_left.split()) <= 2:
                left_candidates.append((key, pos_left))
        # Find first occurrence after trigger for predicate
        pos_after = lower_sentence.find(key_lower, predicate_range_start)
        if pos_after != -1:
            gap = lower_sentence[predicate_range_start: pos_after]
            if len(gap.split()) <= 2:
                right_candidates.append((key, pos_after))
        else:
            pos_full = lower_sentence.find(key_lower)
            if pos_full != -1 and pos_full < predicate_range_start and (pos_full + len(key_lower)) > predicate_range_start:
                right_candidates.append((key, pos_full))

    if left_candidates and right_candidates:
        for left, _ in left_candidates:
            for right, _ in right_candidates:
                subject_info = get_key_info(left, key_mapping)
                predicate_info = get_key_info(right, key_mapping)
                if ((subject_info.get("is_term_manual") == 0 and subject_info.get("oof_prob_class", 0) < 0.05) or
                    (predicate_info.get("is_term_manual") == 0 and predicate_info.get("oof_prob_class", 0) < 0.05)):
                    continue
                relation = {
                    "subject": subject_info,
                    "predicate": predicate_info
                }
                relations.append(relation)
    return relations

def extract_left_candidates_from_prev(sentence, key_mapping):
    """
    Extracts subject candidates from the previous sentence.
    For each key, calculates a tuple: (key, end_position, key_length),
    where end_position = position of the last occurrence of the key + key length.
    Candidates with the maximum end_position are selected, and among them,
    only those with the maximum length are retained.
    Returns a list of objects containing key information.
    """
    normalized = sentence.get("normalizedStr", "")
    lower_sentence = normalized.lower()
    candidate_tuples = []
    for key in sentence.get("keys", []):
        key_lower = key.lower()
        pos = lower_sentence.rfind(key_lower)
        if pos != -1:
            end_pos = pos + len(key_lower)
            candidate_tuples.append((key, end_pos, len(key)))
    if not candidate_tuples:
        return []
    max_end = max(t[1] for t in candidate_tuples)
    candidates_at_max = [t for t in candidate_tuples if t[1] == max_end]
    max_length = max(t[2] for t in candidates_at_max)
    final_keys = [t[0] for t in candidates_at_max if t[2] == max_length]
    return [get_key_info(k, key_mapping) for k in set(final_keys)]

import re

def extract_cross_sentence_relation(prev_sentence, cur_sentence, key_mapping, trigger):
    """
    If the trigger is at the beginning of the current sentence:
      - Extract subject candidates from the end of the previous sentence (prev_sentence).
      - Extract predicate candidates from the current sentence according to standard logic.
    Each relation includes information about the previous sentence.
    Additionally, checks that the originalStr of the current sentence does not start with an English word.
    """
    # Check if originalStr starts with an English word
    orig = cur_sentence.get("originalStr", "").strip()
    if orig:
        first_word = orig.split()[0]
        if re.match(r'^[A-Za-z]+', first_word):
            return []

    relations = []
    normalized = cur_sentence.get("normalizedStr", "")
    lower_sentence = normalized.lower()
    predicate_range_start = len(trigger)
    right_candidates = []

    # Extract predicate candidates from the current sentence
    for key in cur_sentence.get("keys", []):
        key_lower = key.lower()
        pos_after = lower_sentence.find(key_lower, predicate_range_start)
        if pos_after != -1:
            gap = lower_sentence[predicate_range_start: pos_after]
            if len(gap.split()) <= 2:
                right_candidates.append((key, pos_after))
        else:
            pos_full = lower_sentence.find(key_lower)
            if pos_full != -1 and pos_full < predicate_range_start and (pos_full + len(key_lower)) > predicate_range_start:
                right_candidates.append((key, pos_full))

    if right_candidates:
        # Extract subject candidates from the previous sentence
        left_candidates = extract_left_candidates_from_prev(prev_sentence, key_mapping)
        if left_candidates:
            for subj in left_candidates:
                for right, _ in right_candidates:
                    predicate_info = get_key_info(right, key_mapping)
                    # Skip relations if subject or predicate does not meet the required conditions
                    if ((subj.get("is_term_manual") == 0 and subj.get("oof_prob_class", 0) < 0.1) or
                        (predicate_info.get("is_term_manual") == 0 and predicate_info.get("oof_prob_class", 0) < 0.15)):
                        continue
                    relation = {
                        "subject": subj,
                        "predicate": get_key_info(right, key_mapping),
                        "prev_sentence": {
                            "docNum": prev_sentence.get("docNum"),
                            "sentNum": prev_sentence.get("sentNum"),
                            "normalizedStr": prev_sentence.get("normalizedStr"),
                            "originalStr": prev_sentence.get("originalStr")
                        }
                    }
                    relations.append(relation)
    return relations

def relation_score(relation):
    """
    Computes the sorting score for relations.
    Sorting is based first on the sum of oof_prob_class values,
    then on the sum of is_term_manual values.
    """
    subj = relation["subject"]
    pred = relation["predicate"]
    score_oof = (subj.get("oof_prob_class", 0) + pred.get("oof_prob_class", 0))
    score_manual = (subj.get("is_term_manual", 0) + pred.get("is_term_manual", 0))
    return (score_oof, score_manual)

def main():
    """
    Main function to process sentences and extract relations.
    Steps:
      1. Check for required files.
      2. Load key mappings from an Excel file.
      3. Load enriched sentences from a JSON file.
      4. Iterate through sentences to identify triggers and extract relations.
      5. Save results to an output JSON file.
    """
     # Check for file existence
    if not os.path.exists(FILTERED_DATA_XLSX):
        print(f"[ERROR] File {FILTERED_DATA_XLSX} not found.")
        return
    if not os.path.exists(ENRICHED_SENTENCES_PATH):
        print(f"[ERROR] File {ENRICHED_SENTENCES_PATH} not found.")
        return

    # Load key mappings from Excel file
    key_mapping = load_key_mapping(FILTERED_DATA_XLSX)

    # Load enriched sentences from JSON file
    with open(ENRICHED_SENTENCES_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    sentences = data.get("sentences", [])
    # Sort sentences by document number (docNum) and sentence number (sentNum)
    sentences.sort(key=lambda s: (s.get("docNum", 0), s.get("sentNum", 0)))

    enriched_sentences = []
    for idx, sentence in enumerate(sentences):
        normalized = sentence.get("normalizedStr", "")
        lower_sentence = normalized.lower()
        relations = []
        # Check for the presence of any trigger from TRIGGERS in the sentence
        for trig in TRIGGERS:
            trig_pos = lower_sentence.find(trig)
            if trig_pos != -1:
                if trig_pos > 3:
                    relations.extend(extract_within_sentence(sentence, key_mapping, trig, trig_pos))
                else:
                    # If the trigger is at the beginning, find the previous sentence with the same docNum
                    prev_sentence = None
                    for j in range(idx - 1, -1, -1):
                        if sentences[j].get("docNum") == sentence.get("docNum"):
                            prev_sentence = sentences[j]
                            break
                    if prev_sentence is not None:
                        relations.extend(extract_cross_sentence_relation(prev_sentence, sentence, key_mapping, trig))
                # Stop further search if at least one trigger is found.
                break

        if relations:
            # If there are more than two relations, keep only the top two with the highest ranking.
            if len(relations) > 2:
                relations = sorted(relations, key=relation_score, reverse=True)[:2]
            sentence["is-a"] = relations
            enriched_sentences.append(sentence)

    if not enriched_sentences:
        print("No relations found.")
    else:
        output = {"sentences": enriched_sentences}
        with open(OUTPUT_JSON_PATH, "w", encoding="utf-8") as f:
            json.dump(output, f, ensure_ascii=False, indent=4)
        print(f"Enriched {len(enriched_sentences)} sentences. Results saved to {OUTPUT_JSON_PATH}")

if __name__ == "__main__":
    main()
