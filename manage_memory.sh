 #!/bin/bash

# Script to increase swap size and free up memory

# Check current memory status
echo "Current memory status:"
free -h

# Turn off the current swap
echo "Turning off current swap..."
sudo swapoff -a

# Create a new swap file of 4 GiB (adjust the size as needed)
SWAP_SIZE="4G"
echo "Creating a new swap file of size $SWAP_SIZE..."
sudo fallocate -l $SWAP_SIZE /swapfile

# Set the correct permissions for the swap file
echo "Setting permissions for the swap file..."
sudo chmod 600 /swapfile

# Prepare the file to be used as swap
echo "Preparing the swap file..."
sudo mkswap /swapfile

# Turn on the new swap
echo "Turning on the new swap file..."
sudo swapon /swapfile

# Check the new memory and swap status
echo "New memory and swap status:"
free -h

echo "Script completed. Now try loading your model."
