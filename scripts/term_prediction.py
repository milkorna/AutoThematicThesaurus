import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split, StratifiedKFold
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier, StackingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report
from sklearn.preprocessing import LabelEncoder
from imblearn.over_sampling import SMOTE
from imblearn.under_sampling import RandomUnderSampler
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout
from sklearn.neural_network import MLPClassifier
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier, StackingClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.neural_network import MLPClassifier
from sklearn.model_selection import StratifiedKFold
import matplotlib.pyplot as plt

from core.functions import load_fasttext_model, get_phrase_average_embedding, get_weighted_context_embedding
from core.functions import cosine_similarity
from core.paths import PATH_DATA, PATH_FASTTEXT

def vector_norm(vec):
    """
    Computes the Euclidean norm (length) of a vector.
    """
    return np.linalg.norm(vec)

def create_feature_matrix(df, ft_model, label_encoders):
    """
    Creates a feature matrix combining numeric, binary, categorical, and embedding-based features.
    """
    print("[INFO] Creating feature matrix...")
    features = []

    # Columns with numeric data
    numeric_cols = [
        'frequency',
        'topic_relevance',
        'centrality_score',
        'inverse_rarity'
    ]

    for idx, row in df.iterrows():
        # Numeric features
        numeric_vector = []
        for col in numeric_cols:
            val = row[col]
            if pd.isna(val):
                numeric_vector.append(0.0)
            else:
                numeric_vector.append(float(val))

        # Binary feature: tag_match (True/False -> 1.0/0.0)
        tm_value = row['tag_match']
        numeric_vector.append(1.0 if tm_value == "True" else 0.0)

        # Categorical features (model_name, label) encoded using LabelEncoder
        cat_vector = []
        for cat_col in ['model_name', 'label']:
            raw_val = row[cat_col]
            if pd.isna(raw_val):
                raw_val = "NaN_value"
            raw_val = str(raw_val)
            le = label_encoders[cat_col]
            cat_idx = le.transform([raw_val])[0]
            cat_vector.append(float(cat_idx))

        # Embedding features: key and context
        phrase_key = str(row['key']) if not pd.isna(row['key']) else ""
        emb_key = get_phrase_average_embedding(phrase_key, ft_model)

        context_str = str(row['context']) if not pd.isna(row['context']) else ""
        emb_context_aggregated = get_weighted_context_embedding(context_str, ft_model)

        # Additional features derived from embeddings
        key_norm = vector_norm(emb_key)
        context_norm = vector_norm(emb_context_aggregated)
        cos_key_context = cosine_similarity(emb_key, emb_context_aggregated)

        emb_combined = np.concatenate([emb_key, emb_context_aggregated])

        combined = np.concatenate([
            numeric_vector,
            cat_vector,
            [key_norm, context_norm, cos_key_context],
            emb_combined
        ])

        features.append(combined)

    features = np.array(features)
    print(f"[INFO] Feature matrix created. Shape: {features.shape}")
    return features

def build_stacking_classifier(input_dim):
    """
    Builds a stacking classifier using RandomForest, GradientBoosting, and MLPClassifier as base estimators.
    LogisticRegression is used as the final estimator.
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

    stratified_kfold = StratifiedKFold(n_splits=5, shuffle=True, random_state=42)

    stack_clf = StackingClassifier(
        estimators=base_estimators,
        final_estimator=meta_learner,
        passthrough=False,
        cv=stratified_kfold
    )
    return stack_clf

def check_and_handle_small_classes(y, threshold=5):
    """
    Checks class distribution and removes classes with fewer samples than the threshold.
    Returns a mask to filter out samples belonging to small classes.
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
    Balances classes using the specified method. Supports SMOTE and RandomUnderSampler.
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

def main():
    """
    Main script execution for data preprocessing, model training, and evaluation.
    """
    print("[INFO] Starting the script...")

    # Load dataset
    print("[INFO] Loading dataset...")
    df = pd.read_excel(PATH_DATA)
    print(f"[INFO] Dataset loaded. Shape: {df.shape}")

    # Split data into labeled and unlabeled
    print("[INFO] Splitting data into labeled/unlabeled...")
    df_labeled = df[~df['is_term_manual'].isna()].copy()
    df_unlabeled = df[df['is_term_manual'].isna()].copy()

    print(f"[INFO] Labeled examples: {df_labeled.shape[0]}")
    print(f"[INFO] Unlabeled examples: {df_unlabeled.shape[0]}")

    if df_labeled.shape[0] < 10:
        print("[WARNING] Too few labeled examples. Exiting.")
        return

    # Load fastText model
    print("[INFO] Loading fastText model...")
    ft_model = load_fasttext_model(PATH_FASTTEXT)

    # Prepare LabelEncoders for categorical columns
    cat_columns = ['model_name', 'label']
    label_encoders = {}
    for cat_col in cat_columns:
        le = LabelEncoder()
        all_values = df[cat_col].fillna("NaN_value").astype(str).tolist()
        le.fit(all_values)
        label_encoders[cat_col] = le
    print("[INFO] LabelEncoders fitted.")

    # Create feature matrix for labeled data
    print("[INFO] Building features for labeled data...")
    X_labeled = create_feature_matrix(df_labeled, ft_model, label_encoders)
    y_labeled = df_labeled['is_term_manual'].values  # e.g. 0/1 or "term"/"not_term"
    print("[INFO] Labeled features built.")

    # Split labeled data into train and validation sets
    print("[INFO] Splitting labeled data into train and validation sets...")
    indices = np.arange(len(X_labeled))
    train_indices, val_indices = train_test_split(
        indices, test_size=0.2, random_state=42
    )

    X_train = X_labeled[train_indices]
    X_val = X_labeled[val_indices]
    y_train = y_labeled[train_indices]
    y_val = y_labeled[val_indices]
    print(f"[INFO] Train set size: {X_train.shape[0]}, Validation set size: {X_val.shape[0]}")

    # Handle small classes
    mask = check_and_handle_small_classes(y_train, threshold=2)  # Установим threshold=2, так как SMOTE требует минимум 2
    if not mask.all():
        X_train = X_train[mask]
        y_train = y_train[mask]
        print(f"[INFO] Updated training set size: {X_train.shape}")
        class_dist = dict(zip(*np.unique(y_train, return_counts=True)))
        print(f"[INFO] Updated class distribution: {class_dist}")

    # Balance classes in the training set
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

    # Build and train the stacking classifier
    input_dim = X_train_resampled.shape[1]
    clf = build_stacking_classifier(input_dim)
    print("[INFO] Training stacking classifier on resampled data...")
    clf.fit(X_train_resampled, y_train_resampled)
    print("[INFO] Stacking model trained.")

    # Save MLP training and validation graphs after training
    # Retrieve the MLP classifier by its name from base_estimators
    mlp_estimator = clf.named_estimators_['mlp']  # "mlp" из base_estimators

    # Building and saving a schedule loss_curve_
    if hasattr(mlp_estimator, 'loss_curve_'):
        plt.figure(figsize=(8, 5))
        plt.plot(mlp_estimator.loss_curve_, label='Training Loss')
        plt.title('MLP Training Loss Curve')
        plt.xlabel('Iteration')
        plt.ylabel('Loss')
        plt.legend()
        plt.savefig("mlp_loss_curve.png", dpi=150)  # Сохраняем в файл
        plt.close()
        print("[INFO] Saved MLP training loss curve to mlp_loss_curve.png")

    # Generate and save the validation accuracy curve if early stopping is enabled
    if hasattr(mlp_estimator, 'validation_scores_'):
        plt.figure(figsize=(8, 5))
        plt.plot(mlp_estimator.validation_scores_, label='Validation Accuracy')
        plt.title('MLP Validation Accuracy Curve')
        plt.xlabel('Iteration')
        plt.ylabel('Accuracy')
        plt.legend()
        plt.savefig("mlp_validation_accuracy.png", dpi=150)
        plt.close()
        print("[INFO] Saved MLP validation accuracy curve to mlp_validation_accuracy.png")

    # Evaluate the model on the validation set
    print("[INFO] Evaluating on validation set...")
    preds_val = clf.predict(X_val)
    print("[RESULT] Validation classification report:")
    print(classification_report(y_val, preds_val))

    # Create a copy of the relevant rows from df_labeled for validation
    df_val = df_labeled.iloc[val_indices].copy()

    # Save true and predicted labels
    df_val['true_label'] = y_val
    df_val['predicted_label'] = preds_val

    # Identify misclassified examples
    df_misclassified = df_val[df_val['true_label'] != df_val['predicted_label']]

    # Save misclassified examples to an Excel file
    out_path_misclassified = "/home/milkorna/Documents/AutoThematicThesaurus/misclassified_val_samples.xlsx"
    df_misclassified.to_excel(out_path_misclassified, index=False)
    print(f"[INFO] Saved misclassified validation examples to: {out_path_misclassified}")

    # Build features for unlabeled data
    print("[INFO] Building features for unlabeled data...")
    X_unlabeled = create_feature_matrix(df_unlabeled, ft_model, label_encoders)
    print("[INFO] Unlabeled features built.")

    # Predict on unlabeled data and prepare for Active Learning
    if X_unlabeled.shape[0] > 0:
        print("[INFO] Predicting on unlabeled data for Active Learning...")

        preds = clf.predict(X_unlabeled)
        unique_vals, counts = np.unique(preds, return_counts=True)
        print("[INFO] Predicted labels distribution on unlabeled:", dict(zip(unique_vals, counts)))

        probs = clf.predict_proba(X_unlabeled)

        df_unlabeled['predicted_label'] = preds
        df_unlabeled['predicted_prob_class1'] = probs[:, 1]

        # Calculate uncertainty and sort indices based on it
        uncertainty = np.abs(probs[:, 1] - 0.5)
        sorted_indices = np.argsort(uncertainty)

        # Save the top K most uncertain examples to Excel
        K = 200
        top_k_indices = sorted_indices[:K]
        df_most_uncertain = df_unlabeled.iloc[top_k_indices]

        out_path_uncertain = "/home/milkorna/Documents/AutoThematicThesaurus/active_learning_candidates.xlsx"
        df_most_uncertain.to_excel(out_path_uncertain, index=False)
        print(f"[INFO] Saved {K} most uncertain examples to: {out_path_uncertain}")

        # Identify very confident predictions based on a threshold
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

if __name__ == "__main__":
    main()
