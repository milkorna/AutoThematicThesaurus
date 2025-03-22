import os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from scripts.core.paths import PATH_DATA

"""
Script Purpose:
This script performs an analysis of numeric features in a dataset by calculating and visualizing their correlations.

Key tasks include:
1. Loading data from an Excel file.
2. Identifying numeric features for analysis.
3. Computing the correlation matrix for the selected numeric features.
4. Visualizing the correlation matrix using a heatmap.
"""

def main():
    # Load data from an Excel file
    df = pd.read_excel(PATH_DATA)

    # Define the list of numeric features to analyze
    # Adjust the list to include or exclude specific columns as needed.
    numeric_columns = [
        'is_term_manual',
        'phrase_size',
        'frequency',
        'topic_relevance',
        'centrality_score',
        'phrases_count'
    ]

    # Ensure all specified columns exist in the dataset
    numeric_columns = [col for col in numeric_columns if col in df.columns]

    if not numeric_columns:
        print("No numeric features available for analysis.")
        return

    # Compute the correlation matrix for numeric features
    corr_matrix = df[numeric_columns].corr()

    # Visualize the correlation matrix using a heatmap
    plt.figure(figsize=(8, 6))
    sns.heatmap(corr_matrix, annot=True, cmap='coolwarm', fmt=".2f")
    plt.title("Correlation Matrix of Numeric Features")
    plt.tight_layout()

    # Save the heatmap to a file
    output_filename = "correlation_heatmap.png"
    plt.savefig(output_filename, dpi=300)
    plt.close()

    print(f"Done! Correlation heatmap saved to '{output_filename}'")

if __name__ == "__main__":
    main()
