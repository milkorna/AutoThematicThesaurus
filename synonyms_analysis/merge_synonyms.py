import json
import os

def should_include(main_item, synonyms_list):
    """
    Filtering: if the main item and all its "synonymous" objects
    have is_term_manual == 0 and oof_prob_class < 0.4, the item is not included.
    """
    if main_item.get("is_term_manual", 0) != 0 or main_item.get("oof_prob_class", 0) >= 0.4:
        return True
    for syn in synonyms_list:
        if syn.get("is_term_manual", 0) != 0 or syn.get("oof_prob_class", 0) >= 0.4:
            return True
    return False

def process_sbert(file_path):
    """
    Transformation:
      - usage_variants ← empty list,
      - synonyms ← list from the key field of each synonym,
      - similar_phrases ← empty list.
    """
    results = {}
    with open(file_path, encoding='utf-8') as f:
        data = json.load(f)
    for key, item in data.items():
        synonyms_candidates = item.get("synonyms", [])
        if not should_include(item, synonyms_candidates):
            continue

        unified = {
            "key": item["key"],
            "is_term_manual": item["is_term_manual"],
            "oof_prob_class": item["oof_prob_class"],
            "usage_variants": [],
            "synonyms": [syn.get("key") for syn in item.get("synonyms", [])],
            "similar_phrases": []
        }
        results[unified["key"]] = unified
    return results

def process_rut5(file_path):
    """
    Transformation:
      - usage_variants ← filtered top_paraphrases (only phrases containing spaces are kept),
      - synonyms ← list from found_in_data (key field),
      - similar_phrases ← empty list.
    """
    results = {}
    with open(file_path, encoding='utf-8') as f:
        data = json.load(f)
    for key, item in data.items():
        synonyms_candidates = item.get("found_in_data", [])
        if not should_include(item, synonyms_candidates):
            continue

        # Filter top_paraphrases: keep only phrases containing spaces
        usage_variants = [phrase for phrase in item.get("top_paraphrases", []) if " " in phrase]

        unified = {
            "key": item["key"],
            "is_term_manual": item["is_term_manual"],
            "oof_prob_class": item["oof_prob_class"],
            "usage_variants": usage_variants,
            "synonyms": [found.get("key") for found in item.get("found_in_data", [])],
            "similar_phrases": []
        }
        results[unified["key"]] = unified
    return results

def process_mask_deep(file_path):
    """
    Transformation:
      - usage_variants ← empty list,
      - For each synonym:
          if similarity_for_masked_word < 0.64 → add new_phrase to similar_phrases,
          otherwise → add new_phrase to synonyms.
    """
    results = {}
    with open(file_path, encoding='utf-8') as f:
        data = json.load(f)
    for key, item in data.items():
        synonyms_candidates = item.get("synonyms", [])
        if not should_include(item, synonyms_candidates):
            continue

        synonyms_list = []
        similar_phrases_list = []
        for syn in item.get("synonyms", []):
            if syn.get("similarity_for_masked_word", 0) < 0.64:
                similar_phrases_list.append(syn.get("new_phrase"))
            else:
                synonyms_list.append(syn.get("new_phrase"))
        unified = {
            "key": item["key"],
            "is_term_manual": item["is_term_manual"],
            "oof_prob_class": item["oof_prob_class"],
            "usage_variants": [],
            "synonyms": synonyms_list,
            "similar_phrases": similar_phrases_list
        }
        results[unified["key"]] = unified
    return results

def process_usage_variants(file_path):
    """
    Transformation:
      - The main phrase field becomes the key value,
      - usage_variants → list of strings (from the phrase field of each variant),
      - synonyms and similar_phrases remain empty.
    The file is assumed to be either an object or a list of objects.
    """
    results = {}
    with open(file_path, encoding='utf-8') as f:
        data = json.load(f)
    # If data is a dictionary, wrap it in a list for consistency
    if isinstance(data, dict):
        data = [data]
    for item in data:
        unified = {
            "key": item["phrase"],
            "is_term_manual": item["is_term_manual"],
            "oof_prob_class": item["oof_prob_class"],
            "usage_variants": [uv.get("phrase") for uv in item.get("usage_variants", [])],
            "synonyms": [],
            "similar_phrases": []
        }
        results[unified["key"]] = unified
    return results

def merge_results(*dicts):
    """
    Merge results from different sources by key.
    If the same key appears in multiple files:
      - usage_variants, synonyms, and similar_phrases arrays are merged without duplicates;
      - oof_prob_class takes the maximum value;
      - if is_term_manual == 1 in any case, the final value becomes 1.
    """
    merged = {}
    for d in dicts:
        for key, value in d.items():
            if key in merged:
                merged[key]["synonyms"] = list(set(merged[key]["synonyms"] + value["synonyms"]))
                merged[key]["usage_variants"] = list(set(merged[key]["usage_variants"] + value["usage_variants"]))
                merged[key]["similar_phrases"] = list(set(merged[key]["similar_phrases"] + value["similar_phrases"]))
                merged[key]["oof_prob_class"] = max(merged[key]["oof_prob_class"], value["oof_prob_class"])
                merged[key]["is_term_manual"] = 1 if (merged[key]["is_term_manual"] or value["is_term_manual"]) else 0
            else:
                merged[key] = value
    return merged

def main():
    # Define paths to source files (relative to the working directory)
    sbert_file = os.path.join("synonyms_sbert.json")
    rut5_file = os.path.join("synonyms_rut5_base_paraphraser.json")
    mask_deep_file = os.path.join("synonyms_mask_deep_pavlov.json")
    usage_variants_file = os.path.join("usage_variants.json")

    # Process each file
    sbert_data = process_sbert(sbert_file)
    rut5_data = process_rut5(rut5_file)
    mask_deep_data = process_mask_deep(mask_deep_file)
    usage_variants_data = process_usage_variants(usage_variants_file)

    # Merge results from all sources
    merged_data = merge_results(sbert_data, rut5_data, mask_deep_data, usage_variants_data)

    # Convert final dictionary to list and save to final_synonyms.json
    result_list = list(merged_data.values())
    with open("final_synonyms.json", "w", encoding="utf-8") as f:
        json.dump(result_list, f, ensure_ascii=False, indent=4)

if __name__ == "__main__":
    main()
