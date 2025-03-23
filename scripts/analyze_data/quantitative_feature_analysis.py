import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
from core.paths import PATH_DATA

"""
Script Purpose:
This script performs an exploratory data analysis (EDA) on selected quantitative features from the dataset.

Key tasks include:
- Loading the dataset from an Excel file.
- Identifying relevant quantitative features for analysis.
- Creating boxplots and histograms to visualize the distribution of these features.
- Saving the generated plots for further examination.
- Displaying dataset information and summary statistics to check for missing values and understand feature distributions.

This analysis helps assess the statistical properties of key features, detect outliers, and evaluate data distribution for further modeling steps.
"""

# Load the dataset
df = pd.read_excel(PATH_DATA)

# Define quantitative features for analysis
quantitative_features = ["topic_relevance", "centrality_score"]

# Define the output directory for saving plots
output_dir = os.getcwd()

# Generate boxplots and histograms for each feature
for feature in quantitative_features:
    plt.figure(figsize=(12, 6))

    # Boxplot
    plt.subplot(1, 2, 1)
    sns.boxplot(x=df[feature], color='skyblue')
    plt.title(f'Boxplot of {feature}')
    plt.xlabel(feature)

    # Histogram with KDE
    plt.subplot(1, 2, 2)
    sns.histplot(df[feature], bins=30, kde=True, color='orange')
    plt.title(f'Histogram of {feature}')
    plt.xlabel(feature)
    plt.ylabel('Frequency')

    plt.tight_layout()

    # Save the generated plot
    output_file = os.path.join(output_dir, f'{feature}_analysis.png')
    plt.savefig(output_file)
    plt.close()

# Display dataset information and summary statistics
print("Dataset Info:")
print(df.info())
print("\nSummary Statistics:")
print(df[quantitative_features].describe())
