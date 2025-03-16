import os
import json
import pandas as pd

# File paths
EXCEL_PATH = r"/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx"
SENTENCES_JSON_PATH = r"/home/milkorna/Documents/AutoThematicThesaurus/my_data/nlp_corpus/sentences.json"
OUTPUT_JSON_PATH = r"/home/milkorna/Documents/AutoThematicThesaurus/my_data/enriched_sentences.json"

def build_sentence_key_mapping(excel_path):
    """
    Reads an Excel file and creates a dictionary:
      Key: normalized sentence (context)
      Value: list of phrases (key) found in that sentence.
    """
    df = pd.read_excel(excel_path)
    sentence_to_keys = {}

    # Iterate through each row in the Excel file
    for idx, row in df.iterrows():
        key_phrase = row.get('key')
        context = row.get('context')
        if pd.isna(context):
            continue
        # Split the context by delimiter '|'
        contexts = [c.strip() for c in str(context).split('|') if c.strip()]
        for sent in contexts:
            sentence_to_keys.setdefault(sent, []).append(key_phrase)
    return sentence_to_keys

def enrich_sentences(sentences_json_path, sentence_to_keys):
    """
    Loads the original JSON with sentences, searches for phrases in each sentence,
    adds them to a new 'keys' field, and returns a dictionary with enriched sentences.
    Retains only sentences where the list of key phrases is not empty.
    """
    with open(sentences_json_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    enriched = []
    for sentence in data.get("sentences", []):
        normalized = sentence.get("normalizedStr", "").strip()
        keys = sentence_to_keys.get(normalized, [])
        if keys:
            sentence["keys"] = keys
            enriched.append(sentence)
    return {"sentences": enriched}

def main():
    # Check for file existence
    if not os.path.exists(EXCEL_PATH):
        print(f"[ERROR] Excel file {EXCEL_PATH} not found.")
        return
    if not os.path.exists(SENTENCES_JSON_PATH):
        print(f"[ERROR] JSON file {SENTENCES_JSON_PATH} not found.")
        return

    # Build a mapping: sentence -> list of key phrases
    sentence_to_keys = build_sentence_key_mapping(EXCEL_PATH)
    # Enrich sentences with key phrases
    enriched_data = enrich_sentences(SENTENCES_JSON_PATH, sentence_to_keys)

    # Save the result
    with open(OUTPUT_JSON_PATH, "w", encoding="utf-8") as f:
        json.dump(enriched_data, f, ensure_ascii=False, indent=4)

    print(f"Enriched sentences saved to {OUTPUT_JSON_PATH}")

if __name__ == "__main__":
    main()
