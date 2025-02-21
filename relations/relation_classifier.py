import json
import pandas as pd
import numpy as np
from gensim.models import fasttext
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
from xgboost import XGBClassifier, plot_importance
import matplotlib.pyplot as plt
from numpy.linalg import norm
from sklearn.ensemble import StackingClassifier
from sklearn.linear_model import LogisticRegression
from catboost import CatBoostClassifier
import joblib

# Paths to necessary data files
PATH_FINAL_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/relations/marked_relations.json"
PATH_NEED_TO_MARK_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/relations/need_to_mark.json"
PATH_DATA_XLSX = "/home/milkorna/Documents/AutoThematicThesaurus/data_with_oof.xlsx"
PATH_FASTTEXT = "/home/milkorna/Documents/AutoThematicThesaurus/my_custom_fasttext_model_finetuned.bin"
PATH_MODEL = "/home/milkorna/Documents/AutoThematicThesaurus/relations/relation_classifier.pkl"

PATH_NEED_TO_MARK_MARKED_JSON = "/home/milkorna/Documents/AutoThematicThesaurus/relations/need_to_mark_marked.json"

# Feature columns
NUMERIC_COLS = ["phrase_size", "frequency", "topic_relevance",
                "centrality_score", "inverse_rarity", "oof_prob_class"]
CAT_COLS = ["tag_match", "model_name", "label"]

# Label columns representing different types of relations
LABEL_COLS = ["is_synonym", "is_usage_variant", "is_related", "is_hypernym", "is_hyponym"]

# Functions for fastText Loading and Embeddings
def load_fasttext_model(path):
    """Loads a pre-trained fastText model from the specified path."""
    print(f"[INFO] Loading fastText model from: {path}")
    ft = fasttext.load_facebook_model(path)
    print("[INFO] fastText loaded.")
    return ft

def get_phrase_embedding(phrase, ft_model):
    """Computes the average word embedding for a given phrase."""
    if not phrase or not isinstance(phrase, str):
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    words = phrase.split()
    vectors = []
    for w in words:
        if w in ft_model.wv.key_to_index:
            vectors.append(ft_model.wv[w])
    if len(vectors) == 0:
        return np.zeros(ft_model.wv.vector_size, dtype=np.float32)
    else:
        return np.mean(vectors, axis=0)

def get_weighted_context_embedding(context_str, ft_model):
    """Computes a weighted embedding for a context string where weights are based on token count."""
    if not context_str or not isinstance(context_str, str):
        return np.zeros(ft_model.vector_size, dtype=np.float32)
    parts = context_str.split('|')
    vectors = []
    weights = []
    for part in parts:
        part = part.strip()
        if part:
            emb = get_phrase_embedding(part, ft_model)
            if np.any(emb):
                vectors.append(emb)
                weights.append(len(part.split()))
    if len(vectors) == 0:
        return np.zeros(ft_model.wv.vector_size, dtype=np.float32)
    else:
        weights = np.array(weights, dtype=np.float32)
        return np.sum([v * w for v, w in zip(vectors, weights)], axis=0) / weights.sum()

def build_feature_vector(row, ft_model):
    """
    Constructs a feature vector by combining phrase embeddings, context embeddings,
    norms, cosine similarities, and numeric/categorical features.
    """
    # Generate embeddings for the phrase and its main counterpart
    phrase_key = str(row['key'])
    emb_phrase = get_phrase_embedding(phrase_key, ft_model)

    # Generate embeddings for context
    phrase_context = row.get("context", "")
    emb_context = get_weighted_context_embedding(phrase_context, ft_model)

    main_key_str = str(row.get("key_main_main", ""))
    emb_phrase_main = get_phrase_embedding(main_key_str, ft_model)
    main_context = row.get("context_main", "")
    emb_context_main = get_weighted_context_embedding(main_context, ft_model)

    norm_phrase       = norm(emb_phrase) if np.any(emb_phrase) else 0.0
    norm_phrase_main  = norm(emb_phrase_main) if np.any(emb_phrase_main) else 0.0
    norm_context      = norm(emb_context) if np.any(emb_context) else 0.0
    norm_context_main = norm(emb_context_main) if np.any(emb_context_main) else 0.0

    def safe_cosine(u, v):
        """Computes cosine similarity safely, handling division by zero."""
        nu = norm(u)
        nv = norm(v)
        if nu < 1e-9 or nv < 1e-9:
            return 0.0
        return float(np.dot(u, v) / (nu * nv))

    cos_phrase   = safe_cosine(emb_phrase, emb_phrase_main)
    cos_context  = safe_cosine(emb_context, emb_context_main)

    # Extract numerical and categorical features
    numeric_part = []
    for col in NUMERIC_COLS:
        val_phrase = row.get(col, 0.0)
        val_main   = row.get(col+"_main", 0.0)
        numeric_part.append(float(val_phrase))
        numeric_part.append(float(val_main))

    cat_part = []
    for col in CAT_COLS:
        val = row.get(col, None)
        if pd.isna(val) or val is None:
            cat_part.append(0.0)
        else:
            cat_val = hash(str(val)) % 1000
            cat_part.append(float(cat_val))

    # Concatenate all features into a single vector
    return np.concatenate([
        emb_phrase,
        emb_context,
        emb_phrase_main,
        emb_context_main,
        [norm_phrase, norm_phrase_main, cos_phrase,
         norm_context, norm_context_main, cos_context],
        numeric_part,
        cat_part
    ])


def build_X_matrix(df, ft_model):
    """Constructs a feature matrix by applying build_feature_vector to each row."""
    X_list = []
    for idx, row in df.iterrows():
        fv = build_feature_vector(row, ft_model)
        X_list.append(fv)
    return np.array(X_list)


# Data Loading Functions
def load_labeled_relations(path_json):
    """Loads labeled relations from a JSON file and converts them into a DataFrame."""
    with open(path_json, 'r', encoding='utf-8') as f:
        data = json.load(f)
    rows = []
    for block in data:
        main_key = block["key"]
        for pinfo in block["phrases"]:
            rowdict = {
                "key_main": main_key,
                "key": pinfo["phrase"],
                "is_synonym": bool(pinfo["is_synonym"]),
                "is_usage_variant": bool(pinfo["is_usage_variant"]),
                "is_related": bool(pinfo["is_related"]),
                "is_hypernym": bool(pinfo["is_hypernym"]),
                "is_hyponym": bool(pinfo["is_hyponym"])
            }
            rows.append(rowdict)
    return pd.DataFrame(rows)

def load_unlabeled_relations(path_json):
    """Loads the unlabeled dataset and returns the original JSON along with a DataFrame."""
    with open(path_json, 'r', encoding='utf-8') as f:
        data = json.load(f)
    rows_for_df = []
    for block in data:
        main_key = block["key"]
        for pinfo in block["phrases"]:
            phrase = pinfo["phrase"]
            rows_for_df.append({"key_main": main_key, "key": phrase})
    return data, pd.DataFrame(rows_for_df)

# Main Logic
def main():
    """Main function that loads data, trains models, and makes predictions."""
    ft_model = load_fasttext_model(PATH_FASTTEXT)

    df_oof = pd.read_excel(PATH_DATA_XLSX)
    df_lab = load_labeled_relations(PATH_FINAL_JSON)
    merged = pd.merge(df_lab, df_oof, on="key", how="inner")

    # Extract feature matrix and labels
    Y = merged[LABEL_COLS].values.astype(int)
    X = build_X_matrix(merged, ft_model)

    # Split into training and validation sets
    X_train, X_val, Y_train, Y_val = train_test_split(X, Y, test_size=0.2, random_state=42)

    # Train models using stacking
    xgb_models = {}
    base_estimators = [
        ('xgb', XGBClassifier(
            n_estimators=500,
            max_depth=12,
            learning_rate=0.1,
            subsample=0.8,
            colsample_bytree=0.8,
            random_state=42
        )),
        ('cat', CatBoostClassifier(
            iterations=300,
            depth=8,
            learning_rate=0.1,
            random_state=42,
            verbose=0
        ))
    ]
    for i, label_name in enumerate(LABEL_COLS):
        print(f"\n[INFO] Training label: {label_name}")
        y_train_i = Y_train[:, i]
        y_val_i = Y_val[:, i]

        final_estimator = LogisticRegression(random_state=42)

        stack_clf = StackingClassifier(
            estimators=base_estimators,
            final_estimator=final_estimator,
            passthrough=False
        )

        stack_clf.fit(X_train, y_train_i)
        y_val_pred = stack_clf.predict(X_val)
        print(f"[INFO] Validation report for label '{label_name}':")
        print(classification_report(y_val_i, y_val_pred))
        xgb_models[label_name] = stack_clf


    #  Prediction on the training set
    train_preds = []
    for i, label_name in enumerate(LABEL_COLS):
        preds_i = xgb_models[label_name].predict(X)
        train_preds.append(preds_i)
    # Convert predictions list to a NumPy array and transpose it to match the shape (n_samples, 5)
    train_preds = np.array(train_preds).T

    print("\n[INFO] Classification report (TRAIN) for all labels combined:")
    print(classification_report(Y, train_preds, target_names=LABEL_COLS))

    # Load the unlabeled dataset for prediction
    original_data, df_unl_raw = load_unlabeled_relations(PATH_NEED_TO_MARK_JSON)

    # Merge the unlabeled dataset with feature data (left join on 'key')
    df_unl = pd.merge(
        df_unl_raw,
        df_oof,
        on='key',
        how='left'
    )

    # Merge the dataset again, adding '_main' suffix for main key-related features
    df_unl = pd.merge(
        df_unl,
        df_oof.add_suffix('_main'),
        left_on='key_main',
        right_on='key_main',
        how='left'
    )
    print(df_unl.columns)
    for c in NUMERIC_COLS:
        df_unl[c] = df_unl[c].fillna(0.0)
    df_unl['context'] = df_unl['context'].fillna("")

    # Generate feature matrix for unlabeled data
    X_unl = build_X_matrix(df_unl, ft_model)

    # Predictions for each label separately
    all_preds_binary = {}  # Stores binary (0/1) predictions
    all_preds_proba = {}   # Stores probability distributions
    for i, label_name in enumerate(LABEL_COLS):
        model_i = xgb_models[label_name]
        all_preds_binary[label_name] = model_i.predict(X_unl)
        all_preds_proba[label_name] = model_i.predict_proba(X_unl)

    # Store predictions in result DataFrame
    result_df = df_unl.copy()
    for label_name in LABEL_COLS:
        result_df[label_name] = all_preds_binary[label_name].astype(bool)

    # Compute confidence score (e.g., average distance from 0.5 probability threshold)
    confs = []
    for i_row in range(len(result_df)):
        dist_list = []
        for label_name in LABEL_COLS:
            p_1 = all_preds_proba[label_name][i_row, 1]
            dist_list.append(abs(p_1 - 0.5))
        confs.append(np.mean(dist_list))
    result_df["confidence"] = confs

    # Create a mapping of (key_main, key) pairs to their predicted relationships
    pred_map = {}
    for idx, row in result_df.iterrows():
        km = row["key_main"]
        k = row["key"]
        pred_map[(km, k)] = {
            "is_synonym": bool(row["is_synonym"]),
            "is_usage_variant": bool(row["is_usage_variant"]),
            "is_related": bool(row["is_related"]),
            "is_hypernym": bool(row["is_hypernym"]),
            "is_hyponym": bool(row["is_hyponym"]),
            "confidence": float(row["confidence"])
        }

    # Get a list of all (key_main, key) pairs
    all_pairs = list(pred_map.keys())

    # (Optional) If multiple labels are predicted as True, keep only one based on priority
    priority_list = ["is_related", "is_synonym", "is_usage_variant", "is_hypernym", "is_hyponym"]
    for (A, B) in all_pairs:
        relAB = pred_map[(A, B)]
        true_fields = [f for f in priority_list if relAB[f]]
        if len(true_fields) > 1:
            if "is_related" in true_fields:
                for f in priority_list:
                    relAB[f] = (f == "is_related")
            else:
                first_true = true_fields[0]
                for f in priority_list:
                    relAB[f] = (f == first_true)

    # (Optional) Ensure consistency between hypernym-hyponym and synonym relationships
    for (A, B) in all_pairs:
        if (B, A) in pred_map:
            relAB = pred_map[(A, B)]
            relBA = pred_map[(B, A)]

            # Гиперним <-> Гипоним
            if relAB["is_hypernym"]:
                relBA["is_hyponym"] = True
                relBA["is_hypernym"] = False
            if relAB["is_hyponym"]:
                relBA["is_hypernym"] = True
                relBA["is_hyponym"] = False

            # Синонимия
            if relAB["is_synonym"]:
                relBA["is_synonym"] = True
                relAB["is_hypernym"] = relAB["is_hyponym"] = False
                relBA["is_hypernym"] = relBA["is_hyponym"] = False

    # Update original_data with predicted relationships
    for block in original_data:
        km = block["key"]
        for pinfo in block["phrases"]:
            phr = pinfo["phrase"]
            if (km, phr) in pred_map:
                preds = pred_map[(km, phr)]
                pinfo["is_synonym"] = preds["is_synonym"]
                pinfo["is_usage_variant"] = preds["is_usage_variant"]
                pinfo["is_related"] = preds["is_related"]
                pinfo["is_hypernym"] = preds["is_hypernym"]
                pinfo["is_hyponym"] = preds["is_hyponym"]
                pinfo["confidence"] = preds["confidence"]
            else:
                pinfo["is_synonym"] = False
                pinfo["is_usage_variant"] = False
                pinfo["is_related"] = False
                pinfo["is_hypernym"] = False
                pinfo["is_hyponym"] = False
                pinfo["confidence"] = 0.0

    # Save the updated data with predictions
    with open(PATH_NEED_TO_MARK_MARKED_JSON, 'w', encoding='utf-8') as f:
        json.dump(original_data, f, ensure_ascii=False, indent=2)

    print(f"[INFO] Done! Result saved to {PATH_NEED_TO_MARK_MARKED_JSON}")


if __name__ == "__main__":
    main()
