import os
import json
import re
import torch
import inspect
import pandas as pd
import pymorphy2
from collections import Counter
from transformers import T5ForConditionalGeneration, T5Tokenizer
from core.paths import PATH_FILTERED_DATA, SYNONYMS_DIR

# Patch pymorphy2 for compatibility with Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched

# Search parameters
PATH_OUT_JSON = SYNONYMS_DIR / "synonyms_rut5_base_paraphraser.json"
MODEL_NAME = "cointegrated/rut5-base-paraphraser"
NUM_BEAMS = 20                # Number of beams for search
NUM_RETURN_SEQUENCES = 20     # Maximum number of paraphrases
TOP_3 = 3                     # Number of paraphrases to keep in "top_paraphrases"
MAX_LENGTH_FACTOR = 1.5       # Factor to compute max_length (relative to input length)
GRAMS = 4                     # encoder_no_repeat_ngram_size=4 (recommended)

DEVICE = "cuda" if torch.cuda.is_available() else "cpu"

# Initialize tokenizer and model
def load_paraphraser_model():
    print(f"[INFO] Loading paraphraser model: {MODEL_NAME}")
    model = T5ForConditionalGeneration.from_pretrained(MODEL_NAME).to(DEVICE)
    tokenizer = T5Tokenizer.from_pretrained(MODEL_NAME)
    model.eval()
    return model, tokenizer

model, tokenizer = load_paraphraser_model()
print(f"[INFO] Model loaded on device: {DEVICE}")

morph = pymorphy2.MorphAnalyzer()

def generate_paraphrases(text,
                         model,
                         tokenizer,
                         num_beams=NUM_BEAMS,
                         num_return_sequences=NUM_RETURN_SEQUENCES,
                         grams=GRAMS):
    """
    Generates up to num_return_sequences paraphrases of the input text
    using beam search. Returns a list of unique strings.
    """
    text = text.strip()
    if not text:
        return []

    # Prepare input for T5: usually "paraphrase: <text>"
    # However, the model author (cointegrated/rut5-base-paraphraser)
    # mentions that input text alone is sufficient.
    input_ids = tokenizer.encode(text, return_tensors="pt").to(DEVICE)

    # Estimate max_length
    max_size = int(input_ids.shape[1] * MAX_LENGTH_FACTOR + 10)

    # Generate paraphrases
    with torch.no_grad():
        outputs = model.generate(
            input_ids,
            encoder_no_repeat_ngram_size=grams,
            num_beams=num_beams,
            num_return_sequences=num_return_sequences,
            max_length=max_size,
            do_sample=False,
            early_stopping=True
        )

    # Decode outputs
    paraphrases = []
    for out in outputs:
        par = tokenizer.decode(out, skip_special_tokens=True).strip()
        paraphrases.append(par)

    # Remove duplicates
    paraphrases = list(dict.fromkeys(paraphrases))
    return paraphrases

ALLOWED_POS = {"NOUN", "PRTF", "PRTS", "ADJF", "ADJS", "PREP"}

def normalize_phrase(phrase: str) -> str:
    """
    Converts text to lowercase,
    removes punctuation,
    lemmatizes words,
    removes extra spaces,
    ensures that each token's part of speech is in ALLOWED_POS.
    Additionally, ensures that a preposition is not the first or last word.
    """
    # Convert to lowercase
    phrase = phrase.lower()

    # Remove punctuation (any symbols except letters, numbers, and spaces)
    phrase = re.sub(r"[^\w\s\d]+", "", phrase)

    # Split into words
    tokens = phrase.split()

    lemmas = []
    pos_tags = []

    # Lemmatize and check part of speech
    for t in tokens:
        p = morph.parse(t)[0]
        pos = p.tag.POS  # Часть речи
        if pos is None or pos in ALLOWED_POS: # If POS is undefined, keep it
            lemmas.append(p.normal_form)
            pos_tags.append(pos if pos else "UNKNOWN")

    # Ensure a preposition is not first or last
    if not pos_tags:
        return ""

    if pos_tags[0] == "PREP" or pos_tags[-1] == "PREP":
        return ""

    # Reassemble normalized text
    normalized = " ".join(lemmas).strip()
    return normalized

def main():
    print("[INFO] Reading filtered data from:", PATH_FILTERED_DATA)
    df_filtered = pd.read_excel(PATH_FILTERED_DATA)

    # Dictionary for quick access: phrase -> (is_term_manual, oof_prob_class)
    dict = {}
    for idx, row in df_filtered.iterrows():
        ph = str(row.get('key', '')).strip()
        is_term = int(row.get('is_term_manual', 0))
        prob = float(row.get('oof_prob_class', 0.0))
        dict[ph] = (is_term, prob)

    results = {}

    # Process each row from filtered_data
    for idx, row in df_filtered.iterrows():
        original_phrase = str(row.get('key', '')).strip()
        if not original_phrase:
            continue
        if original_phrase not in dict:
            continue

        is_term, prob = dict[original_phrase]

        all_paraphrases = generate_paraphrases(original_phrase, model, tokenizer)

        if not all_paraphrases:
            continue

        cleaned_phrases = []
        for p in all_paraphrases:
            cp = normalize_phrase(p)
            if cp and cp != original_phrase and cp not in cleaned_phrases:
                words = cp.split()
                word_counts = Counter(words)
                if all(count == 1 for count in word_counts.values()): # Ensure uniqueness
                    cleaned_phrases.append(cp)
        if not cleaned_phrases:
            continue

        top_paraphrases = cleaned_phrases[:TOP_3]

        found_in_data = []
        for par in cleaned_phrases:
            if par in dict:
                st, sp = dict[par]
                found_in_data.append({
                    "key": par,
                    "is_term_manual": st,
                    "oof_prob_class": sp
                })

        if not found_in_data:
            continue

        results[original_phrase] = {
            "key": original_phrase,
            "is_term_manual": is_term,
            "oof_prob_class": prob,
            "top_paraphrases": top_paraphrases,
            "found_in_data": found_in_data
        }

    if not results:
        print("[INFO] No paraphrases found in filtered data.")
    else:
        print(f"[INFO] {len(results)} phrases had paraphrases found in filtered data.")

    print(f"[INFO] Saving final JSON to {PATH_OUT_JSON}")
    with open(PATH_OUT_JSON, 'w', encoding='utf-8') as f:
        json.dump(results, f, ensure_ascii=False, indent=4)

    print("[INFO] Done.")

if __name__ == "__main__":
    main()
