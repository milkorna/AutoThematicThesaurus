import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scripts.core.paths import PATH_DATA_WITH_OFF, PROJECT_ROOT

from sklearn.metrics import (
    accuracy_score, precision_score, recall_score, f1_score,
    roc_auc_score, roc_curve, precision_recall_curve, average_precision_score
)

def main():
    # Define input and output file paths
    out_roc_path = PROJECT_ROOT / "roc_curve_oof.png"
    out_pr_path = PROJECT_ROOT / "pr_curve_oof.png"

    # Check if the input file exists
    if not os.path.exists(PATH_DATA_WITH_OFF):
        print(f"[ERROR] No file found: {PATH_DATA_WITH_OFF}")
        return

    # Load the data from the Excel file
    df = pd.read_excel(PATH_DATA_WITH_OFF)
    print("[INFO] Loaded data_with_oof. Shape:", df.shape)

    # Ensure required columns are present
    if 'is_term_manual' not in df.columns or 'oof_prob_class1' not in df.columns:
        print("[ERROR] Columns 'is_term_manual' or 'oof_prob_class1' missing.")
        return

    # Extract true labels and predicted probabilities
    y_true = df['is_term_manual'].values
    y_proba = df['oof_prob_class1'].values

    # Ensure labels are binary (0/1)
    unique_labels = np.unique(y_true)
    print("[INFO] Unique labels in is_term_manual:", unique_labels)

    # Calculate metrics for a default threshold of 0.5
    threshold = 0.5
    y_pred_05 = (y_proba >= threshold).astype(int)

    acc_05 = accuracy_score(y_true, y_pred_05)
    prec_05 = precision_score(y_true, y_pred_05, zero_division=0)
    rec_05 = recall_score(y_true, y_pred_05, zero_division=0)
    f1_05 = f1_score(y_true, y_pred_05, zero_division=0)

    # Display metrics for the default threshold
    print("\n[METRICS at threshold=0.5]")
    print(f"Accuracy = {acc_05:.3f}")
    print(f"Precision= {prec_05:.3f}")
    print(f"Recall   = {rec_05:.3f}")
    print(f"F1       = {f1_05:.3f}")

    # Calculate ROC-AUC score
    roc_auc = roc_auc_score(y_true, y_proba)
    print(f"\n[ROC AUC] = {roc_auc:.3f}")

    # Generate and save the ROC curve
    fpr, tpr, _ = roc_curve(y_true, y_proba)
    plt.figure(figsize=(6,5))
    plt.plot(fpr, tpr, label=f'ROC curve (AUC={roc_auc:.3f})')
    plt.plot([0,1],[0,1], 'r--')
    plt.xlabel("False Positive Rate")
    plt.ylabel("True Positive Rate")
    plt.title("ROC curve (OOF)")
    plt.legend()
    plt.savefig(out_roc_path, dpi=150)
    plt.close()
    print("[INFO] ROC curve saved to:", out_roc_path)

    # Generate and save the Precision-Recall curve
    prec, rec, thr = precision_recall_curve(y_true, y_proba)
    ap = average_precision_score(y_true, y_proba)
    print(f"[Average Precision] = {ap:.3f}")

    plt.figure(figsize=(6,5))
    plt.plot(rec, prec, label=f'PR curve (AP={ap:.3f})')
    plt.xlabel("Recall")
    plt.ylabel("Precision")
    plt.title("Precision-Recall curve (OOF)")
    plt.legend()
    plt.savefig(out_pr_path, dpi=150)
    plt.close()
    print("[INFO] PR curve saved to:", out_pr_path)

    # Find the best threshold based on F1 score
    best_thr = 0.5
    best_f1 = 0.0
    for thr_i in np.linspace(0.0, 1.0, 101):
        y_pred_thr = (y_proba >= thr_i).astype(int)
        f1_thr = f1_score(y_true, y_pred_thr, zero_division=0)
        if f1_thr > best_f1:
            best_f1 = f1_thr
            best_thr = thr_i
    print(f"\n[Best threshold by F1] = {best_thr:.2f} -> F1={best_f1:.3f}")

    # Calculate precision and recall for the best threshold
    y_pred_best = (y_proba >= best_thr).astype(int)
    prec_b = precision_score(y_true, y_pred_best, zero_division=0)
    rec_b = recall_score(y_true, y_pred_best, zero_division=0)
    print(f"Precision={prec_b:.3f}, Recall={rec_b:.3f}")

    print("\n[INFO] Analysis done.")

if __name__ == "__main__":
    main()
