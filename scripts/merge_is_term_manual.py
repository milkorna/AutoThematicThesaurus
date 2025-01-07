import pandas as pd
import os

def merge_is_term_manual(
    data_path="/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx",
    candidates_path="/home/milkorna/Documents/AutoThematicThesaurus/active_learning_candidates.xlsx",
    output_path="/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx",
    key_column="key"
):
    """
    Reads 'data.xlsx' and 'active_learning_candidates.xlsx',
    merges 'is_term_manual' back into the main dataset using 'key' as a unique identifier.
    Saves result to 'data_merged.xlsx'.
    """

    print("[INFO] Loading main dataset...")
    if not os.path.exists(data_path):
        print(f"[ERROR] data file not found: {data_path}")
        return
    df_data = pd.read_excel(data_path)
    print(f"[INFO] Loaded data. Shape: {df_data.shape}")

    print("[INFO] Loading active learning candidates dataset...")
    if not os.path.exists(candidates_path):
        print(f"[ERROR] candidates file not found: {candidates_path}")
        return
    df_candidates = pd.read_excel(candidates_path)
    print(f"[INFO] Loaded candidates. Shape: {df_candidates.shape}")

    # Ïðîâåðèì, ÷òî ñòîëáåö key_column åñòü â îáîèõ äàòàñåòàõ
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

    # Âûáèðàåì òîëüêî íóæíûå ñòîëáöû èç df_candidates, ÷òîáû èçáåæàòü ïóòàíèöû
    df_candidates_subset = df_candidates[[key_column, "is_term_manual"]].copy()

    # Äëÿ óäîáñòâà ïåðåèìåíóåì is_term_manual âî ÷òî-òî âðîäå updated_is_term_manual,
    # ÷òîáû ÿâíî âèäåòü, ÷òî ýòî ïðèøëî èç candidates.
    df_candidates_subset.rename(columns={"is_term_manual": "updated_is_term_manual"}, inplace=True)

    print("[INFO] Merging on key_column =", key_column, "...")
    # Ñäåëàåì merge ñ òèïîì "left" (÷òîáû ñîõðàíèòü ïîðÿäîê/ñòðîêè îñíîâíîãî df_data)
    df_merged = pd.merge(
        df_data,
        df_candidates_subset,
        on=key_column,
        how="left"
    )

    # Òåïåðü â df_merged åñòü ñòîëáåö updated_is_term_manual, êîòîðûé ìîæåò áûòü NaN,
    # åñëè â candidates íå áûëî ñîîòâåòñòâóþùåé ñòðîêè. Åñëè åñòü çíà÷åíèå, ìû õîòèì
    # çàïèñàòü åãî â is_term_manual.

    # Ëîãèêà: òîëüêî îáíîâëÿåì is_term_manual òàì, ãäå updated_is_term_manual íå NaN
    # (è ïðè íåîáõîäèìîñòè, åñëè õîòèì ïåðåçàïèñûâàòü âñå çíà÷åíèÿ, ìîæíî ïî-äðóãîìó)
    mask = ~df_merged["updated_is_term_manual"].isna()
    print(f"[INFO] Will update is_term_manual for {mask.sum()} rows (non-NaN in updated_is_term_manual).")

    # Îáíîâèì is_term_manual òîëüêî òàì, ãäå äåéñòâèòåëüíî åñòü íîâûå äàííûå
    df_merged.loc[mask, "is_term_manual"] = df_merged.loc[mask, "updated_is_term_manual"]

    # Óäàëèì âñïîìîãàòåëüíûé ñòîëáåö (íåîáÿçàòåëüíî)
    df_merged.drop(columns=["updated_is_term_manual"], inplace=True)

    print(f"[INFO] Saving merged results to: {output_path}")
    df_merged.to_excel(output_path, index=False)
    print("[INFO] Done.")

if __name__ == "__main__":
    merge_is_term_manual()
