import os
import numpy as np
import pandas as pd
from sklearn.model_selection import StratifiedKFold
from sklearn.preprocessing import LabelEncoder
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier, StackingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.neural_network import MLPClassifier

from imblearn.over_sampling import SMOTE
from imblearn.under_sampling import RandomUnderSampler

from scripts.core.functions import load_fasttext_model, get_phrase_average_embedding, get_weighted_context_embedding
from scripts.core.paths import PROJECT_ROOT

def create_feature_matrix(df, ft_model, label_encoders):
    """
    Creates a feature matrix by combining numeric, binary, categorical, and embedding-based features.
    """
    numeric_cols = ['frequency', 'topic_relevance', 'centrality_score', 'inverse_rarity']
    features = []

    for _, row in df.iterrows():
        # Extract numeric features
        numeric_vector = []
        for col in numeric_cols:
            val = row[col]
            numeric_vector.append(float(val) if not pd.isna(val) else 0.0)

        # Binary feature: tag_match
        tm_value = row['tag_match']
        numeric_vector.append(1.0 if tm_value == "True" else 0.0)

        # Categorical features: model_name and label (encoded with LabelEncoder)
        cat_vector = []
        for cat_col in ['model_name', 'label']:
            raw_val = row[cat_col]
            if pd.isna(raw_val):
                raw_val = "NaN_value"
            raw_val = str(raw_val)
            le = label_encoders[cat_col]
            cat_idx = le.transform([raw_val])[0]
            cat_vector.append(float(cat_idx))

        # Generate embeddings for 'key' and 'context'
        key_str = str(row['key']) if not pd.isna(row['key']) else ""
        emb_key = get_phrase_average_embedding(key_str, ft_model)

        context_str = str(row['context']) if not pd.isna(row['context']) else ""
        emb_context = get_weighted_context_embedding(context_str, ft_model)

        # Compute norms and cosine similarity between embeddings
        key_norm = np.linalg.norm(emb_key)
        context_norm = np.linalg.norm(emb_context)
        if key_norm == 0.0 or context_norm == 0.0:
            cos_key_context = 0.0
        else:
            cos_key_context = float(np.dot(emb_key, emb_context) / (key_norm * context_norm))

        emb_combined = np.concatenate([emb_key, emb_context])

        # Combine all features into a single vector
        combined = np.concatenate([
            numeric_vector,
            cat_vector,
            [key_norm, context_norm, cos_key_context],
            emb_combined
        ])
        features.append(combined)

    return np.array(features, dtype=np.float32)

def check_and_handle_small_classes(y, threshold=5):
    """
    Filters out classes with fewer samples than the specified threshold.
    Returns a mask to filter the dataset.
    """
    class_counts = pd.Series(y).value_counts()
    small_classes = class_counts[class_counts < threshold].index.tolist()
    if small_classes:
        print(f"[WARNING] Classes with fewer than {threshold} samples in train: {small_classes}. They will be removed.")
        mask = ~pd.Series(y).isin(small_classes)
        return mask
    else:
        return pd.Series([True] * len(y))

def balance_classes(X, y, method='smote'):
    """
    Balances class distribution using SMOTE or RandomUnderSampler.
    """
    if method == 'smote':
        smote = SMOTE(sampling_strategy='auto', random_state=42, k_neighbors=2)
        X_res, y_res = smote.fit_resample(X, y)
        return X_res, y_res
    elif method == 'undersample':
        rus = RandomUnderSampler(sampling_strategy='auto', random_state=42)
        X_res, y_res = rus.fit_resample(X, y)
        return X_res, y_res
    else:
        return X, y

def build_stacking_classifier(input_dim):
    """
    Builds a stacking classifier with RandomForest, GradientBoosting, and MLPClassifier as base models,
    and LogisticRegression as the meta-learner.
    """
    mlp_clf = MLPClassifier(
        hidden_layer_sizes=(256, 128, 64),
        activation='relu',
        solver='adam',
        max_iter=400,
        early_stopping=True,
        validation_fraction=0.1,
        random_state=42
    )
    base_estimators = [
        ('rf', RandomForestClassifier(n_estimators=100, random_state=42, class_weight='balanced')),
        ('gb', GradientBoostingClassifier(random_state=42)),
        ('mlp', mlp_clf)
    ]
    meta_learner = LogisticRegression(random_state=42)
    stack_clf = StackingClassifier(
        estimators=base_estimators,
        final_estimator=meta_learner,
        passthrough=False,
        cv=5
    )
    return stack_clf

def main():
    """
    Main script to preprocess data, build models, and generate out-of-fold predictions.
    """
    # Define file paths
    excel_path = PROJECT_ROOT / "data.xlsx"
    model_path = PROJECT_ROOT / "my_custom_fasttext_model_finetuned.bin"
    out_path = PROJECT_ROOT / "data_with_oof.xlsx"

    # Check if input files exist
    print("[INFO] Checking dataset file:", excel_path)
    if not os.path.exists(excel_path):
        print(f"[ERROR] Dataset file not found: {excel_path}")
        return

    print("[INFO] Checking fastText model file:", model_path)
    if not os.path.exists(model_path):
        print(f"[ERROR] fastText model file not found: {model_path}")
        return

    # Load dataset
    df = pd.read_excel(excel_path)
    print("[INFO] Dataset loaded. Shape:", df.shape)

    # Filter out rows with missing labels
    df = df[~df['is_term_manual'].isna()].copy()
    print("[INFO] Using only labeled data. Shape:", df.shape)

    # Shuffle the dataset
    df = df.sample(frac=1, random_state=42).reset_index(drop=True)

    # Load FastText model
    ft_model = load_fasttext_model(model_path)

    # Encode categorical columns using LabelEncoder
    cat_columns = ['model_name', 'label']
    label_encoders = {}
    for col in cat_columns:
        le = LabelEncoder()
        df[col] = df[col].fillna("NaN_value").astype(str)
        le.fit(df[col].tolist())
        label_encoders[col] = le

    # Create features for the entire dataset
    X_all = create_feature_matrix(df, ft_model, label_encoders)
    y_all = df['is_term_manual'].values  # 0/1

    # Initialize an array to store out-of-fold predictions
    oof_probs = np.zeros(len(df), dtype=np.float32)

    # Perform 10-fold stratified cross-validation
    skf = StratifiedKFold(n_splits=10, shuffle=False)
    for fold_i, (train_idx, test_idx) in enumerate(skf.split(X_all, y_all)):
        print(f"\n[INFO] Fold {fold_i+1}/10: train={len(train_idx)}, test={len(test_idx)}")

        X_train, X_test = X_all[train_idx], X_all[test_idx]
        y_train, y_test = y_all[train_idx], y_all[test_idx]

        # Remove small classes from the training set
        mask = check_and_handle_small_classes(y_train, threshold=2)
        X_train = X_train[mask]
        y_train = y_train[mask]

        # Balance the training set using SMOTE
        X_train_res, y_train_res = balance_classes(X_train, y_train, method='smote')

        # Build and train the stacking classifier
        input_dim = X_train_res.shape[1]
        clf = build_stacking_classifier(input_dim)
        clf.fit(X_train_res, y_train_res)

        # Предсказываем вероятности на test
        test_probs = clf.predict_proba(X_test)[:, 1] # Probabilities for class "1"
        oof_probs[test_idx] = test_probs

    # Add out-of-fold predictions to the dataframe
    df['oof_prob_class1'] = oof_probs

    # Save the dataframe with predictions to an Excel file
    df.to_excel(out_path, index=False)
    print(f"[INFO] Out-of-fold predictions saved to: {out_path}")
    print("[INFO] Done.")

if __name__ == "__main__":
    main()
