import os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from core.paths import PATH_DATA

"""
Script Purpose:
This script visualizes the distribution of numeric features grouped by the target label 'is_term_manual'.

Key tasks include:
- Loading the dataset from an Excel file.
- Verifying the presence of specified numeric features in the dataset.
- Creating boxplots (or violin plots) for each numeric feature to show their distribution across the 'is_term_manual' groups.
- Saving the resulting plots as a single image file.

This type of analysis helps identify differences in numeric features between classes, which can provide insights for feature selection and modeling.
"""

def main():
    # Load the dataset
    df = pd.read_excel(PATH_DATA)

    # Define the list of numeric features to visualize
    numeric_columns = [
        'phrase_size',
        'frequency',
        'topic_relevance',
        'centrality_score',
        'phrases_count'
    ]

    # Ensure the specified columns exist in the dataset
    numeric_columns = [col for col in numeric_columns if col in df.columns]
    if not numeric_columns:
        print("No numeric features available for visualization.")
        return

    # Create subplots for visualizing each numeric feature
    fig, axes = plt.subplots(nrows=len(numeric_columns),
                             figsize=(7, 4*len(numeric_columns)))

    # Handle the case where only one feature is present
    if len(numeric_columns) == 1:
        axes = [axes]  # Convert to a list for consistency

    # Generate boxplots (or violin plots) for each numeric feature
    for i, col in enumerate(numeric_columns):
        ax = axes[i]
        # Create a boxplot to visualize the distribution of the feature
        sns.boxplot(data=df, x='is_term_manual', y=col, ax=ax)

        ax.set_title(f"{col} by is_term_manual Groups")
        ax.set_xlabel("is_term_manual")
        ax.set_ylabel(col)

    # Adjust layout and save the final plot
    plt.tight_layout()

    output_filename = "boxplots_by_is_term_manual.png"
    plt.savefig(output_filename, dpi=300)
    plt.close()

    print(f"Done! Plots saved to '{output_filename}'")

if __name__ == "__main__":
    main()
