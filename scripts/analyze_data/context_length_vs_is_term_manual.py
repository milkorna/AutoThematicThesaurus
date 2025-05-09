import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from core.paths import PATH_DATA

"""
Script Purpose:
This script performs an analysis of the relationship between the length of the context (in characters and words) and the target label 'is_term_manual'.

Key tasks include:
1. Adding new columns to the dataset to calculate context lengths.
2. Visualizing the distribution of context lengths for different values of 'is_term_manual'.
3. Calculating and displaying the correlation between context lengths and the target label.

This analysis provides insights into whether the length of the context has any influence on the classification labels, which could guide feature selection and engineering for machine learning models.
"""

# Load the dataset
df = pd.read_excel(PATH_DATA)

# Add new columns for context length analysis
# Context length in characters
df['context_length_chars'] = df['context'].astype(str).apply(len)
# Context length in words
df['context_length_words'] = df['context'].astype(str).apply(lambda x: len(x.split()))

# Visualize the distribution of context length (in characters) by 'is_term_manual'
plt.figure(figsize=(12, 6))
sns.boxplot(x='is_term_manual', y='context_length_chars', data=df, palette='Set2')
plt.title('Context Length (Characters) vs Is Term Manual', fontsize=16)
plt.xlabel('Is Term Manual', fontsize=12)
plt.ylabel('Context Length (Characters)', fontsize=12)
plt.tight_layout()
plt.savefig('context_length_chars_vs_is_term_manual.png')
plt.show()

# Visualize the distribution of context length (in words) by 'is_term_manual'
plt.figure(figsize=(12, 6))
sns.boxplot(x='is_term_manual', y='context_length_words', data=df, palette='Set3')
plt.title('Context Length (Words) vs Is Term Manual', fontsize=16)
plt.xlabel('Is Term Manual', fontsize=12)
plt.ylabel('Context Length (Words)', fontsize=12)
plt.tight_layout()
plt.savefig('context_length_words_vs_is_term_manual.png')
plt.close()

# Calculate the correlation between context length and the target label
correlation_chars = df[['context_length_chars', 'is_term_manual']].corr().iloc[0, 1]
correlation_words = df[['context_length_words', 'is_term_manual']].corr().iloc[0, 1]

# Display the calculated correlations
print("Correlation between context length (characters) and is_term_manual:", correlation_chars)
print("Correlation between context length (words) and is_term_manual:", correlation_words)
