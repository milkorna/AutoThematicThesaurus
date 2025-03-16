import json
import pandas as pd
import os

HEAD_NOUNS_JSON_PATH = "/home/milkorna/Documents/AutoThematicThesaurus/hyponym_hyponym_analysis/head_nouns.json"
FILTERED_DATA_XLSX = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
OUTPUT_JSON_PATH = "/home/milkorna/Documents/AutoThematicThesaurus/hyponym_hyponym_analysis/head_nouns_filtered.json"

def load_phrases(file_path):
    df = pd.read_excel(file_path)
    if "key" not in df.columns:
        raise ValueError("Column 'key' not found in filtered_data.xlsx")
    phrases = df["key"].astype(str).tolist()
    return phrases

def get_token_set(phrases):
    tokens = set()
    punctuation = ".,!?;:()\"'«»"
    for phrase in phrases:
        cleaned = phrase.lower()
        for ch in punctuation:
            cleaned = cleaned.replace(ch, "")
        words = cleaned.split()
        tokens.update(words)
    return tokens

def filter_head_nouns(head_nouns, valid_words):
    for head, data in head_nouns.items():
        for rel in ["hypernyms", "hyponyms", "synonyms"]:
            filtered_candidates = []
            for candidate in data.get(rel, []):
                word = candidate.get("word", "").lower()
                if word in valid_words:
                    filtered_candidates.append(candidate)
            data[rel] = filtered_candidates
    return head_nouns

def main():
    if not os.path.exists(HEAD_NOUNS_JSON_PATH):
        print(f"[ERROR] File {HEAD_NOUNS_JSON_PATH} not found.")
        return
    with open(HEAD_NOUNS_JSON_PATH, "r", encoding="utf-8") as f:
        head_nouns = json.load(f)

    if not os.path.exists(FILTERED_DATA_XLSX):
        print(f"[ERROR] File {FILTERED_DATA_XLSX} not found.")
        return
    phrases = load_phrases(FILTERED_DATA_XLSX)
    valid_words = get_token_set(phrases)
    print(f"[INFO] Collected {len(valid_words)} unique words from filtered_data.xlsx.")

    filtered_head_nouns = filter_head_nouns(head_nouns, valid_words)

    with open(OUTPUT_JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(filtered_head_nouns, f, ensure_ascii=False, indent=4)

    print(f"[INFO] Filtered head nouns saved to {OUTPUT_JSON_PATH}")

if __name__ == "__main__":
    main()
