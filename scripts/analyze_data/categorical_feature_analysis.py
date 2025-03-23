import pandas as pd
import matplotlib.pyplot as plt
import os
from core.paths import PATH_DATA

"""
Script Purpose:
This script is designed to analyze and visualize the distribution of key features in a dataset. Specifically, it generates bar charts for the columns 'model_name' and 'label' to understand their frequency distributions.

The visualizations created here can guide data preprocessing and feature engineering steps in machine learning pipelines.
"""

# Load the dataset
df = pd.read_excel(PATH_DATA)

# Define the directory for saving plots
output_dir = os.getcwd()

# Generate and save a bar chart for 'model_name' distribution
plt.figure(figsize=(10, 6))
df['model_name'].value_counts().plot(kind='bar', color='skyblue', edgecolor='black')
plt.title('Distribution of model_name', fontsize=16)
plt.xlabel('Model Name', fontsize=12)
plt.ylabel('Frequency', fontsize=12)
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
model_name_chart_path = os.path.join(output_dir, 'model_name_distribution.png')
plt.savefig(model_name_chart_path)
plt.close()

# Generate and save a bar chart for 'label' distribution
plt.figure(figsize=(10, 6))
df['label'].value_counts().plot(kind='bar', color='lightgreen', edgecolor='black')
plt.title('Distribution of label', fontsize=16)
plt.xlabel('Label', fontsize=12)
plt.ylabel('Frequency', fontsize=12)
plt.xticks(rotation=45, ha='right')
plt.tight_layout()
label_chart_path = os.path.join(output_dir, 'label_distribution.png')
plt.savefig(label_chart_path)
plt.close()
