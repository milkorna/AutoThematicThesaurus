import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.decomposition import PCA

from scripts.core.functions import load_fasttext_model, get_phrase_average_embedding, get_weighted_context_embedding

"""
Script Purpose:
This script performs an in-depth analysis of embeddings extracted from a Fast Text model.

Key tasks include:
- Loading and preprocessing the dataset.
- Computing key embedding-related features (norms, cosine similarity).
- Identifying and saving outliers based on IQR analysis.
- Generating and saving visualizations, including:
  - Histograms of key embedding-related features split by class.
  - Boxplots to compare distributions of features across classes.
  - A heatmap of correlation values between embedding-related features and the target label.
  - A scatter plot of PCA-reduced embeddings to visualize clustering.
- Performing PCA to reduce the dimensionality of concatenated embeddings.
- Saving the processed dataset with computed embedding features.
- Logging detected outliers into separate Excel files for further review.
"""

def detect_outliers(data, column, threshold=1.5):
    """Detects outliers based on the interquartile range (IQR) method."""
    Q1 = data[column].quantile(0.25)
    Q3 = data[column].quantile(0.75)
    IQR = Q3 - Q1
    lower_bound = Q1 - threshold * IQR
    upper_bound = Q3 + threshold * IQR
    return data[(data[column] < lower_bound) | (data[column] > upper_bound)]

def main():
    # Define file paths
    excel_path = "/home/milkorna/Documents/AutoThematicThesaurus/data.xlsx"
    model_path = "/home/milkorna/Documents/AutoThematicThesaurus/my_custom_fasttext_model_finetuned.bin"
    out_dir = "/home/milkorna/Documents/AutoThematicThesaurus/analyze_data/embedding_analysis"

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    # Load and preprocess dataset (using only labeled rows)
    df = pd.read_excel(excel_path)
    df = df[~df['is_term_manual'].isna()].copy()
    print("[INFO] Labeled data shape:", df.shape)
    df['is_term_manual'] = df['is_term_manual'].astype(int)

    # Load FastText model
    ft_model = load_fasttext_model(model_path)

    # Compute key embedding-related features
    key_norm_list = []
    context_norm_list = []
    cos_list = []
    key_embs = []
    context_embs = []

    for idx, row in df.iterrows():
        key_str = str(row['key']) if pd.notna(row['key']) else ""
        emb_key = get_phrase_average_embedding(key_str, ft_model)

        context_str = str(row['context']) if pd.notna(row['context']) else ""
        emb_context = get_weighted_context_embedding(context_str, ft_model)

        knorm = np.linalg.norm(emb_key)
        cnorm = np.linalg.norm(emb_context)

        if knorm == 0.0 or cnorm == 0.0:
            cossim = 0.0
        else:
            cossim = float(np.dot(emb_key, emb_context) / (knorm * cnorm))

        key_norm_list.append(knorm)
        context_norm_list.append(cnorm)
        cos_list.append(cossim)
        key_embs.append(emb_key)
        context_embs.append(emb_context)

    df['key_norm'] = key_norm_list
    df['context_norm'] = context_norm_list
    df['key_context_cosine'] = cos_list

    feats = ['key_norm', 'context_norm', 'key_context_cosine']

    # Identify and save outliers
    outlier_dir = os.path.join(out_dir, "outliers")
    if not os.path.exists(outlier_dir):
        os.makedirs(outlier_dir)

    outliers_summary = {}
    for feat in feats:
        # Detect outliers using the IQR method
        outliers = detect_outliers(df, feat)
        outliers_summary[feat] = len(outliers)
        out_file = os.path.join(outlier_dir, f"{feat}_outliers.xlsx")
        outliers.to_excel(out_file, index=False)
        print(f"[INFO] Found {len(outliers)} outliers in {feat}. Saved to {out_file}.")

    # Save a summary of outliers
    summary_file = os.path.join(outlier_dir, "outliers_summary.xlsx")
    summary_df = pd.DataFrame(list(outliers_summary.items()), columns=["Feature", "Outlier Count"])
    summary_df.to_excel(summary_file, index=False)
    print(f"[INFO] Summary of outliers saved to {summary_file}.")

    # Generate histograms and boxplots for embedding-related features
    for feat in feats:
        # Histogram of feature distribution
        plt.figure(figsize=(7,5))
        sns.histplot(data=df, x=feat, hue='is_term_manual', kde=True, stat='density', common_norm=False)
        plt.title(f"{feat} distribution by class")
        plt.savefig(os.path.join(out_dir, f"hist_{feat}.png"), dpi=150)
        plt.close()

        # Boxplot of feature distribution across classes
        plt.figure(figsize=(5,6))
        sns.boxplot(data=df, x='is_term_manual', y=feat)
        plt.title(f"{feat} boxplot by class")
        plt.savefig(os.path.join(out_dir, f"box_{feat}.png"), dpi=150)
        plt.close()

    # Compute and visualize correlation matrix
    corr_df = df[feats + ['is_term_manual']].corr(method='pearson')
    plt.figure(figsize=(10,9))
    sns.heatmap(corr_df, annot=True, cmap='RdBu', center=0)
    plt.title("Correlation matrix (key_norm, context_norm, cos, is_term_manual)")
    plt.savefig(os.path.join(out_dir, "corr_matrix.png"), dpi=150)
    plt.close()

    # Generate pairplot for selected features (optional, can be heavy on large datasets)
    pairplot_data = df[feats + ['is_term_manual']].copy()
    pairplot_data['is_term_manual'] = pairplot_data['is_term_manual'].astype(str)
    sns.pairplot(pairplot_data, hue='is_term_manual', diag_kind='hist')
    plt.savefig(os.path.join(out_dir, "pairplot_features.png"), dpi=150)
    plt.close()

    # Scatter plot of key_norm vs context_norm
    plt.figure(figsize=(6,5))
    sns.scatterplot(data=df, x='key_norm', y='context_norm', hue='is_term_manual', alpha=0.4)
    plt.title("Scatter: key_norm vs context_norm")
    plt.savefig(os.path.join(out_dir, "scatter_key_context_norm.png"), dpi=150)
    plt.close()

    # PCA 2D projection of concatenated key and context embeddings
    key_embs = np.array(key_embs)
    context_embs = np.array(context_embs)
    concat_embs = np.concatenate([key_embs, context_embs], axis=1)
    print("[INFO] concat_embs shape:", concat_embs.shape)

    pca = PCA(n_components=2, random_state=42)
    pca_2d = pca.fit_transform(concat_embs)
    df['pca_x'] = pca_2d[:,0]
    df['pca_y'] = pca_2d[:,1]

    # Scatter plot of PCA-transformed embeddings
    plt.figure(figsize=(6,5))
    sns.scatterplot(data=df, x='pca_x', y='pca_y', hue='is_term_manual', alpha=0.5)
    plt.title("PCA on concatenated embeddings (2D)")
    plt.savefig(os.path.join(out_dir, "pca_2d.png"), dpi=150)
    plt.close()

    # Save the processed dataset with computed features
    out_file = os.path.join(out_dir, "data_with_embed_analysis.xlsx")
    df.to_excel(out_file, index=False)
    print(f"[INFO] Saved results with embedding features to: {out_file}")
    print("[INFO] Analysis done. Plots saved in:", out_dir)

if __name__ == "__main__":
    main()
