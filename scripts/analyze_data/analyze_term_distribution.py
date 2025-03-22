import pandas as pd
import matplotlib.pyplot as plt
import os
from scripts.core.paths import PATH_DATA

# Load the dataset
df = pd.read_excel(PATH_DATA)

# Ensure the required column exists in the dataset
if 'is_term_manual' not in df.columns:
    raise ValueError("Dataset does not contain the required column 'is_term_manual'.")

# Function to plot the distribution of 'is_term_manual' labels
def plot_is_term_distribution(data):
    plt.figure(figsize=(8, 6))
    data['is_term_manual'].value_counts().sort_index().plot(kind='bar', color=['skyblue', 'salmon'], alpha=0.7)
    plt.title('Distribution of labels', fontsize=14)
    plt.xlabel('is_term_manual', fontsize=12)
    plt.ylabel('Count', fontsize=12)
    plt.xticks([0, 1], labels=['Non-Term (0)', 'Term (1)'], rotation=0)
    plt.grid(axis='y', linestyle='--', alpha=0.6)

    # Save the plot to the current working directory
    output_path = os.path.join(os.getcwd(), 'is_term_manual_distribution.png')
    plt.savefig(output_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved as: {output_path}")
    plt.show()

plot_is_term_distribution(df)
