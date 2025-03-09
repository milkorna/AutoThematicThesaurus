import os
import json
import pandas as pd
import numpy as np

from sentence_transformers import SentenceTransformer
from annoy import AnnoyIndex

PATH_BIGDATA = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
PATH_FILTERED = "/home/milkorna/Documents/AutoThematicThesaurus/filtered_data.xlsx"
PATH_OUT_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/synonyms_analysis/synonyms_sbert.json"

SBERT_MODEL_NAME = "sberbank-ai/sbert_large_nlu_ru"
DIM_EMB = 1024
TOP_K = 100     # number of nearest neighbors extracted from Annoy
SIM_THRESHOLD = 0.8  # cosine similarity threshold

def load_dataframes():
    print("[INFO] Reading big data from:", PATH_BIGDATA)
    df_big = pd.read_excel(PATH_BIGDATA)

    # Ensure required columns are present
    required_cols = {'key', 'is_term_manual', 'oof_prob_class'}
    if not required_cols.issubset(df_big.columns):
        missing = required_cols - set(df_big.columns)
        raise ValueError(f"[ERROR] Missing columns in df_big: {missing}")

    print("[INFO] Reading filtered data from:", PATH_FILTERED)
    df_filtered = pd.read_excel(PATH_FILTERED)
    # Assume df_filtered contains at least the 'key' column
    if 'key' not in df_filtered.columns:
        raise ValueError("[ERROR] 'key' column is missing in df_filtered.")

    return df_big, df_filtered

def load_sbert_model(model_name=SBERT_MODEL_NAME):
    print("[INFO] Loading SBERT model:", model_name)
    model = SentenceTransformer(model_name)
    return model

def build_annoy_index(embeddings, dim=DIM_EMB, n_trees=10):
    """
    embeddings: np.array of shape [N, dim]
    Build an AnnoyIndex (angular=False => uses Euclidean distance,
    but we need cosine similarity => see nuances).
    For cosine similarity, Annoy recommends setting:
      AnnoyIndex(dim, 'angular')
    """
    print("[INFO] Building Annoy index, dimension:", dim)
    t = AnnoyIndex(dim, 'angular')
    # 'angular' distance = 2*(1 - cosine_similarity)

    for i, emb in enumerate(embeddings):
        t.add_item(i, emb)
    t.build(n_trees)
    return t

def cosine_similarity(vec1, vec2):
    """
    Classic cosine similarity calculation
    """
    num = np.dot(vec1, vec2)
    denom = np.linalg.norm(vec1) * np.linalg.norm(vec2)
    if denom == 0.0:
        return 0.0
    return float(num / denom)

def main():
    df_big, df_filtered = load_dataframes()
    model = load_sbert_model(SBERT_MODEL_NAME)

    # Convert df_big to a list of strings (keys) + map {idx -> (phrase, is_term, prob)}
    big_phrases = df_big['key'].astype(str).tolist()

    # Create an array (is_term_manual, oof_prob_class) for convenience
    big_info = []
    for i, row in df_big.iterrows():
        big_info.append({
            "key": str(row['key']),
            "is_term_manual": int(row['is_term_manual']),
            "oof_prob_class": float(row['oof_prob_class'])
        })

    # Generate embeddings for data_with_oof
    print("[INFO] Embedding data_with_oof phrases...")
    big_embeddings = model.encode(big_phrases, show_progress_bar=True)
    big_embeddings = np.array(big_embeddings, dtype=np.float32)  # shape [N, DIM_EMB]

    # Build AnnoyIndex for fast kNN search
    print("[INFO] Building Annoy index for big data, size:", len(big_phrases))
    annoy_index = build_annoy_index(big_embeddings, DIM_EMB, n_trees=10)

    # Process filtered_data: embed each phrase and find nearest neighbors
    results = {}

    for idx_f, row_f in df_filtered.iterrows():
        phrase_key = str(row_f['key']).strip()
        if not phrase_key:
            continue

        # Check if the phrase exists in df_big to retrieve is_term_manual / prob
        # It may not always exist. If missing, assign defaults 0 / 0.0
        mask = (df_big['key'] == phrase_key)
        if not mask.any():
            # optionally skip or set defaults
            is_term_f = int(row_f.get('is_term_manual', 0))
            prob_f = 0.0
        else:
            # take the first matching row
            row_match = df_big[mask].iloc[0]
            is_term_f = int(row_match['is_term_manual'])
            prob_f = float(row_match['oof_prob_class'])

        # Compute embedding
        emb_f = model.encode(phrase_key)
        emb_f = np.array(emb_f, dtype=np.float32)

        # Find top_k nearest neighbors in Annoy
        nn_indices = annoy_index.get_nns_by_vector(emb_f, TOP_K, include_distances=True)
        neighbor_ids, dists = nn_indices

        # Convert angular distance to cosine similarity
        synonyms_found = []

        for i_n, dist_n in zip(neighbor_ids, dists):
            cos_sim = 1.0 - (dist_n / 2.0)
            cos_sim = max(min(cos_sim, 1.0), -1.0)  # clamp for safety

            # Exclude exact match
            if big_info[i_n]["key"] == phrase_key:
                continue

            if cos_sim >= SIM_THRESHOLD:
                # Add to results
                synonyms_found.append({
                    "key": big_info[i_n]["key"],
                    "is_term_manual": big_info[i_n]["is_term_manual"],
                    "oof_prob_class": big_info[i_n]["oof_prob_class"],
                    "similarity": round(cos_sim, 4)
                })

        if synonyms_found:
            results[phrase_key] = {
                "key": phrase_key,
                "is_term_manual": is_term_f,
                "oof_prob_class": prob_f,
                "synonyms": synonyms_found
            }

    print(f"[INFO] Found {len(results)} phrases with synonyms above threshold {SIM_THRESHOLD}.")

    print(f"[INFO] Saving results to {PATH_OUT_JSON}")
    with open(PATH_OUT_JSON, "w", encoding="utf-8") as f_out:
        json.dump(results, f_out, ensure_ascii=False, indent=4)

    print("[INFO] Done.")

if __name__ == "__main__":
    main()
