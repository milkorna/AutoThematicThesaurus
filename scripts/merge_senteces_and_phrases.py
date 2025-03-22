import os
import json
import pandas as pd

from scripts.core.paths import PATH_DATA, PATH_SENTENCES, PATH_SENTENCES_WITH_PHRASES

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
    if not os.path.exists(PATH_DATA):
        print(f"[ERROR] Excel file {PATH_DATA} not found.")
        return
    if not os.path.exists(PATH_SENTENCES):
        print(f"[ERROR] JSON file {PATH_SENTENCES} not found.")
        return

    # Build a mapping: sentence -> list of key phrases
    sentence_to_keys = build_sentence_key_mapping(PATH_DATA)
    # Enrich sentences with key phrases
    enriched_data = enrich_sentences(PATH_SENTENCES, sentence_to_keys)

    # Save the result
    with open(PATH_SENTENCES_WITH_PHRASES, "w", encoding="utf-8") as f:
        json.dump(enriched_data, f, ensure_ascii=False, indent=4)

    print(f"Enriched sentences saved to {PATH_SENTENCES_WITH_PHRASES}")

if __name__ == "__main__":
    main()
