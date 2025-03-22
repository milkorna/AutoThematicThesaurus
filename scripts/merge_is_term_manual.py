import pandas as pd
import os

from scripts.core.paths import PATH_DATA, PROJECT_ROOT

def merge_is_term_manual():
    """
    Reads 'data.xlsx' and 'active_learning_candidates.xlsx',
    merges 'is_term_manual' back into the main dataset using 'key' as a unique identifier.
    Saves result to 'data_merged.xlsx'.
    """
    candidates_path = PROJECT_ROOT / "active_learning_candidates.xlsx",
    key_column="key"

    print("[INFO] Loading main dataset...")
    if not os.path.exists(PATH_DATA):
        print(f"[ERROR] data file not found: {PATH_DATA}")
        return
    df_data = pd.read_excel(PATH_DATA)
    print(f"[INFO] Loaded data. Shape: {df_data.shape}")

    print("[INFO] Loading active learning candidates dataset...")
    if not os.path.exists(candidates_path):
        print(f"[ERROR] candidates file not found: {candidates_path}")
        return
    df_candidates = pd.read_excel(candidates_path)
    print(f"[INFO] Loaded candidates. Shape: {df_candidates.shape}")

    if key_column not in df_data.columns:
        print(f"[ERROR] Column '{key_column}' not found in main dataset.")
        return
    if key_column not in df_candidates.columns:
        print(f"[ERROR] Column '{key_column}' not found in candidates dataset.")
        return
    if "is_term_manual" not in df_candidates.columns:
        print(f"[ERROR] Column 'is_term_manual' not found in candidates dataset.")
        return

    print("[INFO] Preparing to merge 'is_term_manual' from candidates into data...")

    df_candidates_subset = df_candidates[[key_column, "is_term_manual"]].copy()
    df_candidates_subset.rename(columns={"is_term_manual": "updated_is_term_manual"}, inplace=True)

    print("[INFO] Merging on key_column =", key_column, "...")
    df_merged = pd.merge(
        df_data,
        df_candidates_subset,
        on=key_column,
        how="left"
    )

    mask = ~df_merged["updated_is_term_manual"].isna()
    print(f"[INFO] Will update is_term_manual for {mask.sum()} rows (non-NaN in updated_is_term_manual).")

    df_merged.loc[mask, "is_term_manual"] = df_merged.loc[mask, "updated_is_term_manual"]
    df_merged.drop(columns=["updated_is_term_manual"], inplace=True)

    print(f"[INFO] Saving merged results to: {PATH_DATA}")
    df_merged.to_excel(PATH_DATA, index=False)
    print("[INFO] Done.")

if __name__ == "__main__":
    merge_is_term_manual()
