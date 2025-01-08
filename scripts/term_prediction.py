import pandas as pd
import numpy as np
from gensim.models import KeyedVectors
from gensim.models import fasttext
from sklearn.model_selection import train_test_split, StratifiedKFold
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier, StackingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report
from sklearn.preprocessing import LabelEncoder
from imblearn.over_sampling import SMOTE
from imblearn.under_sampling import RandomUnderSampler
import os

def load_fasttext_model(model_path):
    """
    Attempts to load a FastText model via gensim.
    """
    print(f"[INFO] Loading fastText model from: {model_path}")
    model = fasttext.load_facebook_model(model_path)
    # model = KeyedVectors.load_facebook_vectors(model_path)
    print("[INFO] fastText model loaded successfully.")
    return model

def get_phrase_embedding(phrase, ft_model):
    """
    Given a phrase (string) and a FastText model, return the average embedding
    of the words in the phrase. If a word is unknown to the model, skip it.
    If phrase is empty or all words are unknown, returns a zero vector.
    """
    if not phrase or not isinstance(phrase, str):
        # If phrase is None or not a string, return a zero-vector
        return np.zeros(ft_model.vector_size, dtype=np.float32)

    words = phrase.split()
    vectors = []
    for w in words:
        if w in ft_model.wv.key_to_index:
            vectors.append(ft_model.wv[w])
    if len(vectors) == 0:
        # If no known words, return a zero-vector of the same dimension
        return np.zeros(ft_model.wv.vector_size, dtype=np.float32)
    else:
        return np.mean(vectors, axis=0)

def vector_norm(vec):
    """
    Returns the Euclidean norm (length) of the vector.
    """
    return np.linalg.norm(vec)

def cosine_similarity(vec1, vec2):
    """
    Returns the cosine similarity between two vectors.
    If one of them is zero, returns 0.0 to avoid division by zero.
    """
    norm1 = np.linalg.norm(vec1)
    norm2 = np.linalg.norm(vec2)
    if norm1 == 0.0 or norm2 == 0.0:
        return 0.0
    return float(np.dot(vec1, vec2) / (norm1 * norm2))

def create_feature_matrix(df, ft_model, label_encoders):
    """
    Build a feature matrix using:
      - numeric cols: frequency, topic_relevance, centrality_score
      - binary col: tag_match (True/False -> 1/0)
      - binary col: is_term_auto (0/1)
      - categorical cols: model_name, label (using LabelEncoder)
      - embeddings: key + context (both from fastText, concatenated)
      - key_vector_norm
      - context_vector_norm
      - key_context_cosine (cosine similarity between key and context embeddings)
    """
    print("[INFO] Creating feature matrix...")
    features = []

    # Numeric columns
    numeric_cols = [
        'frequency',
        'topic_relevance',
        'centrality_score',
        'inverse_rarity'
    ]

    for idx, row in df.iterrows():
        # --- 1) Numeric features ---
        numeric_vector = []
        for col in numeric_cols:
            val = row[col]
            if pd.isna(val):
                numeric_vector.append(0.0)
            else:
                numeric_vector.append(float(val))

        # --- 2) Binary feature: tag_match (True/False -> 1.0/0.0) ---
        tm_value = row['tag_match']
        numeric_vector.append(1.0 if tm_value == "True" else 0.0)

        # --- 3) Categorical features (model_name, label) via label_encoders ---
        cat_vector = []
        for cat_col in ['model_name', 'label']:
            raw_val = row[cat_col]
            if pd.isna(raw_val):
                raw_val = "NaN_value"
            raw_val = str(raw_val)
            le = label_encoders[cat_col]
            cat_idx = le.transform([raw_val])[0]
            cat_vector.append(float(cat_idx))

        # --- 4) Embeddings: key + context ---
        phrase_key = str(row['key']) if not pd.isna(row['key']) else ""
        emb_key = get_phrase_embedding(phrase_key, ft_model)

        context_str = str(row['context']) if not pd.isna(row['context']) else ""
        emb_context = get_phrase_embedding(context_str, ft_model)

        key_norm = vector_norm(emb_key)          # length of key embedding
        context_norm = vector_norm(emb_context)  # length of context embedding
        cos_key_context = cosine_similarity(emb_key, emb_context)

        emb_combined = np.concatenate([emb_key, emb_context])

        # --- 5) Combine everything into one vector ---
        combined = np.concatenate([
            numeric_vector,       # numeric + binary
            cat_vector,           # categorical
            [key_norm, context_norm, cos_key_context],
            emb_combined          # original 2 * embedding_dim
        ])

        features.append(combined)

    features = np.array(features)
    print(f"[INFO] Feature matrix created. Shape: {features.shape}")
    return features

def build_stacking_classifier():
    """
    Build a Stacking ensemble:
      - Level-0: RandomForest, GradientBoosting
      - Meta-learner (final_estimator): LogisticRegression
      - Stratified K-Fold cross-validation
    """
    base_estimators = [
        ('rf', RandomForestClassifier(n_estimators=100, random_state=42, class_weight='balanced')),
        ('gb', GradientBoostingClassifier(random_state=42))
    ]
    meta_learner = LogisticRegression(random_state=42)
    stratified_kfold = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)

    stack_clf = StackingClassifier(
        estimators=base_estimators,
        final_estimator=meta_learner,
        passthrough=False,
        cv=stratified_kfold
    )
    return stack_clf

def main():
    # Paths
    excel_path = "/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx"
    model_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_custom_fasttext_model_finetuned.bin"

    print("[INFO] Starting the script...")

    # Check dataset file
    print(f"[INFO] Checking dataset file: {excel_path}")
    if not os.path.exists(excel_path):
        print(f"[ERROR] Dataset file not found: {excel_path}")
        return

    # Check fastText model file
    print(f"[INFO] Checking fastText model file: {model_path}")
    if not os.path.exists(model_path):
        print(f"[ERROR] fastText model file not found: {model_path}")
        return

    # 1) Load dataset
    print("[INFO] Loading dataset...")
    df = pd.read_excel(excel_path)
    print(f"[INFO] Dataset loaded. Shape: {df.shape}")

    # 2) Split into labeled vs. unlabeled by 'is_term_manual'
    print("[INFO] Splitting data into labeled/unlabeled...")
    df_labeled = df[~df['is_term_manual'].isna()].copy()
    df_unlabeled = df[df['is_term_manual'].isna()].copy()

    print(f"[INFO] Labeled examples: {df_labeled.shape[0]}")
    print(f"[INFO] Unlabeled examples: {df_unlabeled.shape[0]}")

    if df_labeled.shape[0] < 10:
        print("[WARNING] Too few labeled examples. Exiting.")
        return

    # 3) Load fastText model
    print("[INFO] Loading fastText model...")
    ft_model = load_fasttext_model(model_path)

    # 4) Prepare LabelEncoders for categorical columns (model_name, label)
    print("[INFO] Preparing LabelEncoders for categorical columns...")
    cat_columns = ['model_name', 'label']
    label_encoders = {}
    for cat_col in cat_columns:
        le = LabelEncoder()
        all_values = df[cat_col].fillna("NaN_value").astype(str).tolist()
        le.fit(all_values)
        label_encoders[cat_col] = le
    print("[INFO] LabelEncoders fitted.")

    # 5) Build features for labeled data
    print("[INFO] Building features for labeled data...")
    X_labeled = create_feature_matrix(df_labeled, ft_model, label_encoders)
    y_labeled = df_labeled['is_term_manual'].values  # e.g. 0/1 or "term"/"not_term"
    print("[INFO] Labeled features built.")

    # 6) Split labeled data into train and validation sets
    print("[INFO] Splitting labeled data into train and validation sets...")
    X_train, X_val, y_train, y_val = train_test_split(
        X_labeled, y_labeled, test_size=0.2, random_state=42
    )
    print(f"[INFO] Train set size: {X_train.shape[0]}, Validation set size: {X_val.shape[0]}")

    # 7.1) Проверка и удаление малых классов
    print("[INFO] Checking class distribution in training set...")
    mask = check_and_handle_small_classes(y_train, threshold=2)  # Установим threshold=2, так как SMOTE требует минимум 2
    if not mask.all():
        X_train = X_train[mask]
        y_train = y_train[mask]
        print(f"[INFO] Updated training set size: {X_train.shape}")
        class_dist = dict(zip(*np.unique(y_train, return_counts=True)))
        print(f"[INFO] Updated class distribution: {class_dist}")

    # 7.2) Балансировка классов
    print("[INFO] Balancing classes in the training set...")
    try:
        X_train_resampled, y_train_resampled = balance_classes(X_train, y_train, method='smote')
    except ValueError as ve:
        print(f"[ERROR] Error during SMOTE: {ve}")
        print("[INFO] Attempting to balance classes with k_neighbors=1...")
        try:
            smote = SMOTE(sampling_strategy='auto', random_state=42, k_neighbors=1)
            X_train_resampled, y_train_resampled = smote.fit_resample(X_train, y_train)
            print(f"[INFO] Data after SMOTE with k_neighbors=1: {X_train_resampled.shape[0]} samples.")
            class_dist = dict(zip(*np.unique(y_train_resampled, return_counts=True)))
            print(f"[INFO] Class distribution after SMOTE: {class_dist}")
        except Exception as e:
            print(f"[ERROR] Failed to apply SMOTE even with k_neighbors=1: {e}")
            print(f"[INFO] Proceeding without applying SMOTE.")
            X_train_resampled, y_train_resampled = X_train, y_train

    # 8) Build and train stacking classifier
    print("[INFO] Building stacking classifier...")
    clf = build_stacking_classifier()

    print("[INFO] Training stacking classifier on resampled data...")
    clf.fit(X_train_resampled, y_train_resampled)
    print("[INFO] Stacking model trained.")

    # 9) Evaluate the model on validation set
    print("[INFO] Evaluating on validation set...")
    preds_val = clf.predict(X_val)
    print("[RESULT] Validation classification report:")
    print(classification_report(y_val, preds_val))

    # 10) Build features for unlabeled data
    print("[INFO] Building features for unlabeled data...")
    X_unlabeled = create_feature_matrix(df_unlabeled, ft_model, label_encoders)
    print("[INFO] Unlabeled features built.")

    # 11) Predict on unlabeled and get probabilities (for Active Learning)
    if X_unlabeled.shape[0] > 0:
        print("[INFO] Predicting on unlabeled data for Active Learning...")

        preds = clf.predict(X_unlabeled)
        unique_vals, counts = np.unique(preds, return_counts=True)
        print("[INFO] Predicted labels distribution on unlabeled:", dict(zip(unique_vals, counts)))

        probs = clf.predict_proba(X_unlabeled)

        df_unlabeled['predicted_label'] = preds
        df_unlabeled['predicted_prob_class1'] = probs[:, 1]

        uncertainty = np.abs(probs[:, 1] - 0.5)
        sorted_indices = np.argsort(uncertainty)

        K = 100
        top_k_indices = sorted_indices[:K]
        df_most_uncertain = df_unlabeled.iloc[top_k_indices]

        out_path_uncertain = "/home/milkorna/Documents/AutoThematicThesaurus/active_learning_candidates.xlsx"
        df_most_uncertain.to_excel(out_path_uncertain, index=False)
        print(f"[INFO] Saved {K} most uncertain examples to: {out_path_uncertain}")

        threshold = 0.9
        is_very_confident_mask = (probs[:, 1] >= threshold) | (probs[:, 1] <= (1 - threshold))

        df_very_confident = df_unlabeled.iloc[np.where(is_very_confident_mask)[0]]
        df_very_confident.loc[:, 'is_term_manual'] = df_very_confident['predicted_label']

        out_path_confident = "/home/milkorna/Documents/AutoThematicThesaurus/certain_candidates.xlsx"
        df_very_confident.to_excel(out_path_confident, index=False)
        print(f"[INFO] Saved {df_very_confident.shape[0]} very confident examples to: {out_path_confident}")
    else:
        print("[INFO] No unlabeled data found, nothing to do for Active Learning.")
    print("[INFO] Script finished successfully.")

def check_and_handle_small_classes(y, threshold=5):
    """
    Checks class distribution and removes classes with fewer samples than the threshold.
    """
    class_counts = pd.Series(y).value_counts()
    small_classes = class_counts[class_counts < threshold].index.tolist()
    if small_classes:
        print(f"[WARNING] Classes with fewer than {threshold} samples: {small_classes}. They will be removed.")
        mask = ~pd.Series(y).isin(small_classes)
        return mask
    else:
        print(f"[INFO] All classes have sufficient number of samples.")
        return pd.Series([True] * len(y))

def balance_classes(X, y, method='smote'):
    """
    Balances classes using the specified method.
    Supported methods: 'smote', 'undersample', 'none'
    """
    if method == 'smote':
        print("[INFO] Applying SMOTE for class balancing...")
        smote = SMOTE(sampling_strategy='auto', random_state=42, k_neighbors=2)
        X_res, y_res = smote.fit_resample(X, y)
        print(f"[INFO] Data after SMOTE: {X_res.shape[0]} samples.")
        class_distribution = dict(zip(*np.unique(y_res, return_counts=True)))
        print(f"[INFO] Class distribution after SMOTE: {class_distribution}")
        return X_res, y_res
    elif method == 'undersample':
        print("[INFO] Applying Random UnderSampling for class balancing...")
        rus = RandomUnderSampler(sampling_strategy='auto', random_state=42)
        X_res, y_res = rus.fit_resample(X, y)
        print(f"[INFO] Data after UnderSampling: {X_res.shape[0]} samples.")
        class_distribution = dict(zip(*np.unique(y_res, return_counts=True)))
        print(f"[INFO] Class distribution after UnderSampling: {class_distribution}")
        return X_res, y_res
    else:
        print("[INFO] No class balancing applied.")
        return X, y


if __name__ == "__main__":
    main()
