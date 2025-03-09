import json
import os
import pandas as pd
import numpy as np
import torch
import inspect
import pymorphy2
import gensim
from gensim.models import fasttext

from transformers import AutoTokenizer, AutoModelForMaskedLM

# Patch pymorphy2 for compatibility with Python 3.12+
if not hasattr(inspect, "getargspec"):
    def getargspec_patched(func):
        spec = inspect.getfullargspec(func)
        return spec.args, spec.varargs, spec.varkw, spec.defaults  # Совместимый формат
    inspect.getargspec = getargspec_patched

# Search parameters
PATH_FILTERED = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
PATH_BIGDATA = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
PATH_OUT_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/synonyms_analysis/synonyms_mask_deep_pavlov.json"
PATH_FASTTEXT = "/home/milkorna/Documents/AutoThematicThesaurus/my_custom_fasttext_model_finetuned.bin"
MODEL_NAME = "DeepPavlov/rubert-base-cased"
TOP_N = 20  # Number of candidates the model generates for [MASK]
STOP_WORD = "данный"  # Synonyms containing this word will be removed

# Initialize tokenizer and model
print("[INFO] Loading morphological analyzer and model...")
morph = pymorphy2.MorphAnalyzer()
tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME)
model = AutoModelForMaskedLM.from_pretrained(MODEL_NAME)
model.eval()
print("[INFO] Model loaded successfully.")

def load_fasttext_model(model_path):
    """
    Loads a FastText model from the specified path (via Gensim).
    """
    print(f"[INFO] Loading fastText model from: {model_path}")
    ft_model = fasttext.load_facebook_model(model_path)
    print("[INFO] fastText model loaded successfully.")
    return ft_model

ft_model = load_fasttext_model(PATH_FASTTEXT)

def get_pos_tag(word):
    """
    Returns the part of speech (POS) of a word using pymorphy2, e.g., NOUN, ADJF, etc.
    """
    parsed_word = morph.parse(word)[0]
    return parsed_word.tag.POS

def lemmatize_word(word):
    """
    Lemmatizes a word using pymorphy2.
    """
    parsed_word = morph.parse(word)[0]
    return parsed_word.normal_form

def get_word_embedding(word, ft_model):
    """
    Returns the word vector from the fastText model.
    If the word is not in the vocabulary, returns a zero vector.
    """
    if word in ft_model.wv.key_to_index:
        return ft_model.wv[word]
    else:
        # If the word is unknown, return a zero vector
        return np.zeros(ft_model.vector_size, dtype=np.float32)

def cosine_similarity(vec1, vec2):
    """
    Returns the cosine similarity between two vectors.
    """
    denom = (np.linalg.norm(vec1) * np.linalg.norm(vec2))
    if denom == 0.0:
        return 0.0
    return float(np.dot(vec1, vec2) / denom)

def find_synonyms_in_set(phrase, phrases_set, top_n=TOP_N):
    """
    - For each word in the phrase, replace it with [MASK], the model generates top_n candidates
    - Filter by part of speech
    - Create new phrase variations
    - Keep only those variations that exist in phrases_set (exact match)
    Returns a list (strings) of potential "synonyms" from the set.
    """
    words = phrase.split()

    # POS for each word
    pos_tags = [get_pos_tag(w) for w in words]

    found_candidates = []

    for i, (original_word, original_pos) in enumerate(zip(words, pos_tags)):
        # Save the embedding of original_word to calculate similarity
        emb_original = get_word_embedding(original_word, ft_model)

        # Mask the i-th token
        masked_sentence = words.copy()
        masked_sentence[i] = "[MASK]"
        # Create fake context for the model (can be modified)
        input_text = f"{' '.join(masked_sentence)} может быть заменено на [MASK]."

        # Tokenization
        input_ids = tokenizer.encode(input_text, return_tensors="pt")
        mask_token_index = torch.where(input_ids == tokenizer.mask_token_id)[1]
        # Prediction
        with torch.no_grad():
            output = model(input_ids)
            logits = output.logits

        mask_token_logits = logits[0, mask_token_index, :]
        top_tokens = torch.topk(mask_token_logits, top_n, dim=1).indices[0].tolist()

        # Generate candidates
        for token_id in top_tokens:
            cand_word = tokenizer.decode([token_id]).strip().lower()
            cand_lemma = lemmatize_word(cand_word)
            # Filter by POS
            if get_pos_tag(cand_lemma) == original_pos:
                new_words = words.copy()
                new_words[i] = cand_lemma
                new_phrase = " ".join(new_words)
                if new_phrase != phrase and new_phrase in phrases_set:
                    # Calculate similarity_for_masked_word
                    emb_candidate = get_word_embedding(cand_lemma, ft_model)
                    sim_score = cosine_similarity(emb_original, emb_candidate)
                    found_candidates.append((new_phrase, i, original_word, cand_lemma, sim_score))

    # Remove duplicate new_phrases and construct the final list
    unique_candidates = {}
    for (new_ph, i_word, orig_w, cand_w, sim_val) in found_candidates:
        if new_ph not in unique_candidates:
            unique_candidates[new_ph] = (i_word, orig_w, cand_w, sim_val)
        else:
            _, _, _, old_sim = unique_candidates[new_ph]
            if sim_val > old_sim:
                unique_candidates[new_ph] = (i_word, orig_w, cand_w, sim_val)

    # Convert to a list of dictionaries
    results = []
    for new_phrase, (i_word, orig_w, cand_w, sim_val) in unique_candidates.items():
        results.append({
            "new_phrase": new_phrase,
            "masked_word": orig_w,
            "candidate_word": cand_w,
            "similarity_for_masked_word": round(sim_val, 5)
        })

    return results

def ensure_mutual_synonyms_and_cleanup(results, stop_word=STOP_WORD):
    """
    - Remove elements from synonyms where "key" contains stop_word (e.g., "данный").
    - For each pair (A -> B), if B -> A does not exist, add it.
    - If A has an empty synonyms list, remove A from results (or leave it).
    """
    # Convert keys to a list (to iterate over a copy)
    all_phrases = list(results.keys())

    for phrase in all_phrases:
        item = results[phrase]
        synonyms_list = item.get("synonyms", [])
        filtered_list = []
        for syn_obj in synonyms_list:
            if stop_word not in syn_obj["new_phrase"]:
                filtered_list.append(syn_obj)
        item["synonyms"] = filtered_list

    # 2) Mutual completion: If A contains B, then B should also contain A
    for phrase in all_phrases:
        if phrase not in results:
            continue
        synonyms_list = results[phrase].get("synonyms", [])
        for syn_obj in synonyms_list:
            candidate_str = syn_obj["new_phrase"]
            if candidate_str in results:
                # Check if phrase exists in candidate_str
                cand_syns = results[candidate_str].get("synonyms", [])
                # Verify if phrase -> candidate_str already exists
                already = any( c["new_phrase"] == phrase for c in cand_syns )
                if not already:
                    # Add entry
                    cand_syns.append({
                        "new_phrase": phrase,
                        "masked_word": syn_obj["candidate_word"],  # условно
                        "candidate_word": syn_obj["masked_word"],  # swapped
                        "similarity_for_masked_word": syn_obj["similarity_for_masked_word"]
                    })
                    results[candidate_str]["synonyms"] = cand_syns


    # Remove entries from the dictionary where synonyms list is empty
    to_remove = []
    for phrase in results:
        if not results[phrase]["synonyms"]:
            to_remove.append(phrase)

    for phrase in to_remove:
        del results[phrase]

    return results

def main():
    print("[INFO] Reading filtered data from:", PATH_FILTERED)
    df_filtered = pd.read_excel(PATH_FILTERED)

    print("[INFO] Reading big data from:", PATH_BIGDATA)
    df_big = pd.read_excel(PATH_BIGDATA)

    # Ensure df_big contains the required columns: 'key', 'is_term_manual', 'oof_prob_class'
    required_cols = {'key', 'is_term_manual', 'oof_prob_class'}
    if not required_cols.issubset(df_big.columns):
        print(f"[ERROR] df_big missing required columns: {required_cols - set(df_big.columns)}")
        return

    # Convert df_big into a dictionary { phrase -> (is_term_manual, oof_prob_class) }
    # and also into a set for fast lookup
    big_dict = {}
    for idx, row in df_big.iterrows():
        phrase_key = str(row['key']).strip()
        is_term = int(row['is_term_manual'])  # assuming 0/1
        prob = float(row['oof_prob_class'])
        big_dict[phrase_key] = (is_term, prob)
    phrases_set = set(big_dict.keys())
    results = {}

    # For each row in df_filtered (containing key, is_term_manual, etc.)
    # Find synonyms
    for idx, row in df_filtered.iterrows():
        phrase_key = str(row['key']).strip()
        if phrase_key not in big_dict:
            continue
        is_term, prob = big_dict[phrase_key]

        synonyms_found = find_synonyms_in_set(phrase_key, phrases_set, top_n=TOP_N)

        if not synonyms_found:
            # If no candidates are found, we still include the item in results
            # but it may be removed after ensure_mutual_synonyms_and_cleanup
            results[phrase_key] = {
                "key": phrase_key,
                "is_term_manual": is_term,
                "oof_prob_class": prob,
                "synonyms": []
            }
        else:
            # For each new_phrase, find is_term, prob
            syn_list = []
            for syn_item in synonyms_found:
                new_ph = syn_item["new_phrase"]
                masked_w = syn_item["masked_word"]
                cand_w = syn_item["candidate_word"]
                sim_score = syn_item["similarity_for_masked_word"]

                # Retrieve is_term, prob from big_dict[new_ph]
                syn_is_term, syn_prob = big_dict[new_ph]
                syn_list.append({
                    "new_phrase": new_ph,
                    "masked_word": masked_w,
                    "candidate_word": cand_w,
                    "similarity_for_masked_word": sim_score,
                    "is_term_manual": syn_is_term,
                    "oof_prob_class": syn_prob
                })

            results[phrase_key] = {
                "key": phrase_key,
                "is_term_manual": is_term,
                "oof_prob_class": prob,
                "synonyms": syn_list
            }


    # Post-processing: remove stop_word, ensure mutual completion, remove empty entries
    results = ensure_mutual_synonyms_and_cleanup(results, stop_word=STOP_WORD)

    # Save final JSON
    if not results:
        print("[INFO] No items with synonyms found. The result is empty.")
    else:
        print(f"[INFO] Found {len(results)} phrases with synonyms after cleanup.")

    print(f"[INFO] Saving synonyms to {PATH_OUT_JSON}")
    with open(PATH_OUT_JSON, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=4)

    print("[INFO] Done.")


if __name__ == "__main__":
    main()
