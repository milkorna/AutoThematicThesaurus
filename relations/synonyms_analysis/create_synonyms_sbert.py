import os
import json
import pandas as pd
import numpy as np
from sentence_transformers import SentenceTransformer
from annoy import AnnoyIndex
from core.paths import PATH_FILTERED_DATA, SYNONYMS_DIR

# Path to the output JSON file
PATH_OUT_JSON = SYNONYMS_DIR / "synonyms_sbert.json"

# SBERT model identifier and embedding/search parameters
SBERT_MODEL_NAME = "sberbank-ai/sbert_large_nlu_ru"
DIM_EMB = 1024
TOP_K = 100          # number of nearest neighbors to retrieve from Annoy
SIM_THRESHOLD = 0.8  # cosine similarity threshold for considering synonyms

def load_data():
    print("[INFO] Reading data from:", PATH_FILTERED_DATA)
    df = pd.read_excel(PATH_FILTERED_DATA)

    # Verify that required columns are present
    required_cols = {'key', 'is_term_manual', 'oof_prob_class'}
    if not required_cols.issubset(df.columns):
        missing = required_cols - set(df.columns)
        raise ValueError(f"[ERROR] Missing columns in DataFrame: {missing}")

    # Remove empty or whitespace-only keys and ensure all keys are strings
    df['key'] = df['key'].astype(str).str.strip()
    df = df[df['key'] != ""].reset_index(drop=True)
    return df

def load_sbert_model(model_name=SBERT_MODEL_NAME):
    print("[INFO] Loading SBERT model:", model_name)
    return SentenceTransformer(model_name)

def build_annoy_index(embeddings: np.ndarray, dim: int = DIM_EMB, n_trees: int = 10) -> AnnoyIndex:
    print(f"[INFO] Building Annoy index with dimension {dim} and {n_trees} trees")
    # Use 'angular' distance, which corresponds to cosine similarity
    index = AnnoyIndex(dim, 'angular')
    for i, emb in enumerate(embeddings):
        index.add_item(i, emb)
    index.build(n_trees)
    return index

def main():
    # Load filtered dataset
    df = load_data()

    # Load SBERT model and compute embeddings for all keys
    model = load_sbert_model()
    phrases = df['key'].tolist()
    print("[INFO] Computing embeddings for all phrases...")
    embeddings = model.encode(phrases, show_progress_bar=True)
    embeddings = np.array(embeddings, dtype=np.float32)

    # Build Annoy index for fast nearest-neighbor search
    print(f"[INFO] Constructing Annoy index for {len(phrases)} items...")
    annoy_index = build_annoy_index(embeddings)

    # Prepare auxiliary info for each phrase
    info_records = df.to_dict(orient='records')

    # For each phrase, retrieve nearest neighbors and filter by similarity threshold
    results = {}
    for idx, row in df.iterrows():
        key = row['key']
        emb = embeddings[idx]

        # Retrieve TOP_K nearest neighbors (including distances)
        neighbor_ids, distances = annoy_index.get_nns_by_vector(emb, TOP_K, include_distances=True)

        synonyms = []
        for nbr_idx, dist in zip(neighbor_ids, distances):
            if nbr_idx == idx:
                continue  # skip the phrase itself
            # Convert angular distance to cosine similarity
            cos_sim = 1.0 - (dist / 2.0)
            if cos_sim >= SIM_THRESHOLD:
                nbr = info_records[nbr_idx]
                synonyms.append({
                    "key": nbr["key"],
                    "is_term_manual": int(nbr["is_term_manual"]),
                    "oof_prob_class": float(nbr["oof_prob_class"]),
                    "similarity": round(cos_sim, 4)
                })

        if synonyms:
            results[key] = {
                "key": key,
                "is_term_manual": int(row["is_term_manual"]),
                "oof_prob_class": float(row["oof_prob_class"]),
                "synonyms": synonyms
            }

    print(f"[INFO] Found {len(results)} phrases with similarity ≥ {SIM_THRESHOLD}")

    # Ensure output directory exists and save results to JSON
    print(f"[INFO] Saving results to {PATH_OUT_JSON}")
    os.makedirs(SYNONYMS_DIR, exist_ok=True)
    with open(PATH_OUT_JSON, "w", encoding="utf-8") as fout:
        json.dump(results, fout, ensure_ascii=False, indent=4)

    print("[INFO] Completed successfully.")

if __name__ == "__main__":
    main()
