#!/usr/bin/env python3
import os
import json
import pandas as pd
import numpy as np
from gensim.models import fasttext
from numpy.linalg import norm

from core.functions import load_fasttext_model, get_phrase_average_embedding, cosine_similarity
from core.paths import PROJECT_ROOT, PATH_FASTTEXT

FILTERED_DATA_XLSX = PROJECT_ROOT / "filtered_data.xlsx"

# Output JSON file name (will be saved in the current directory)
OUTPUT_JSON_FILENAME = "hypernym_voting_results.json"

NEIGHBORS_TOP_N = 100
SECOND_ORDER_WEIGHT = 0.5
OOF_PROB_THRESHOLD = 0.05 # If oof_prob_class < 0.05 and is_term_manual == 0, the candidate is discarded

def load_taxonomy(xlsx_path):
    """
    Loads the taxonomy from the filtered_data.xlsx file.
    Returns:
      taxonomy — a dictionary { phrase: {"is_term_manual": ..., "oof_prob_class": ...} },
      taxonomy_set — a set of all phrases (for quick lookup).
    """
    df = pd.read_excel(xlsx_path)
    taxonomy = {}
    for idx, row in df.iterrows():
        phrase = str(row.get("key", "")).strip()
        if not phrase:
            continue
        taxonomy[phrase] = {
            "is_term_manual": row.get("is_term_manual", 0),
            "oof_prob_class": row.get("oof_prob_class", 0.0)
        }
    return taxonomy, set(taxonomy.keys())

def precompute_taxonomy_embeddings(taxonomy, ft_model):
    """
    Computes embeddings for all phrases in the taxonomy.
    Returns a dictionary: { phrase: embedding (L2-normalized numpy.ndarray) }
    """
    embeddings = {}
    for phrase in taxonomy:
        emb = get_phrase_average_embedding(phrase, ft_model)
        if emb is not None:
            embeddings[phrase] = emb
    return embeddings

def get_nearest_neighbors(query_emb, taxonomy_embeddings, top_n=NEIGHBORS_TOP_N):
    """
    Finds the top-N nearest neighbors based on cosine similarity among precomputed taxonomy embeddings.
    Returns a list of tuples (phrase, similarity).
    """
    similarities = []
    for phrase, emb in taxonomy_embeddings.items():
        sim = cosine_similarity(query_emb, emb)
        similarities.append((phrase, sim))
    similarities.sort(key=lambda x: x[1], reverse=True)
    return similarities[:top_n]

def generate_candidate_hypernyms(phrase, taxonomy_set):
    """
    Generates all non-empty subphrases of the given phrase that exist in taxonomy_set and are strictly shorter.
    """
    tokens = phrase.split()
    n = len(tokens)
    candidates = set()

    for i in range(n):
        for j in range(i+1, n+1):
            subphrase = " ".join(tokens[i:j]).strip()
            if subphrase and len(subphrase.split()) < n and subphrase in taxonomy_set:
                candidates.add(subphrase)
    return candidates


def vote_for_hypernyms(neighbor_phrase, neighbor_sim, taxonomy_set):
    """
    Generates hypernym candidates of first and second order for a given neighboring phrase.
    Second-order candidates have reduced weighting (SECOND_ORDER_WEIGHT).
    Returns a dictionary: { candidate_phrase: vote_weight }.
    """
    votes = {}
    first_order = generate_candidate_hypernyms(neighbor_phrase, taxonomy_set)
    for cand in first_order:
        votes[cand] = votes.get(cand, 0) + neighbor_sim
        second_order = generate_candidate_hypernyms(cand, taxonomy_set)
        for cand2 in second_order:
            votes[cand2] = votes.get(cand2, 0) + neighbor_sim * SECOND_ORDER_WEIGHT
    return votes

def hypernym_voting(query_phrase, taxonomy, taxonomy_embeddings, ft_model):
    """
    Implements the hypernym voting algorithm:
      - Computes embedding for query_phrase.
      - Finds top-N nearest neighbors.
      - Each neighbor votes for its hypernyms (first and second order).
      - Votes are aggregated.
      - Candidates are filtered based on is_term_manual and oof_prob_class threshold.
      - Returns a ranked list of top-10 hypernyms.
    """
    query_emb = get_phrase_average_embedding(query_phrase, ft_model)
    if query_emb is None:
        return []

    neighbors = get_nearest_neighbors(query_emb, taxonomy_embeddings, top_n=NEIGHBORS_TOP_N)

    total_votes = {}
    for neighbor_phrase, sim in neighbors:
        votes = vote_for_hypernyms(neighbor_phrase, sim, set(taxonomy.keys()))
        for candidate, vote_val in votes.items():
            total_votes[candidate] = total_votes.get(candidate, 0) + vote_val

    filtered_votes = {}
    for cand, vote in total_votes.items():
        cand_info = taxonomy.get(cand, {})
        is_term_manual = cand_info.get("is_term_manual", 0)
        oof_prob_class = cand_info.get("oof_prob_class", 0.0)
        if is_term_manual == 0 and oof_prob_class < OOF_PROB_THRESHOLD:
            continue
        filtered_votes[cand] = vote

    ranked = sorted(filtered_votes.items(), key=lambda x: x[1], reverse=True)[:10]

    hypernyms_info = []
    for cand, vote_val in ranked:
        cand_info = taxonomy.get(cand, {})
        hypernyms_info.append({
            "hypernym": cand,
            "vote": vote_val,
            "is_term_manual": cand_info.get("is_term_manual", 0),
            "oof_prob_class": cand_info.get("oof_prob_class", 0.0)
        })

    return hypernyms_info

def main():
    print("[INFO] Loading taxonomy...")
    taxonomy, taxonomy_set = load_taxonomy(FILTERED_DATA_XLSX)
    print(f"[INFO] Loaded {len(taxonomy)} phrases from taxonomy.")

    print("[INFO] Loading fastText model...")
    ft_model = load_fasttext_model(PATH_FASTTEXT)

    print("[INFO] Precomputing embeddings for taxonomy...")
    taxonomy_embeddings = precompute_taxonomy_embeddings(taxonomy, ft_model)

    results = []
    for idx, phrase in enumerate(taxonomy.keys(), 1):
        if phrase not in taxonomy_embeddings:
            hypernyms = []
        else:
            hypernyms = hypernym_voting(phrase, taxonomy, taxonomy_embeddings, ft_model)

        result = {
            "query": phrase,
            "hypernyms": hypernyms
        }

        results.append(result)

    output_path = os.path.join(os.getcwd(), OUTPUT_JSON_FILENAME)
    print(f"[INFO] Saving results to {output_path} ...")
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump({"results": results}, f, ensure_ascii=False, indent=4)

    print("[INFO] Done.")

if __name__ == "__main__":
    main()
