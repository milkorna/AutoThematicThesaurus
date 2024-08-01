 
#!/bin/bash

if [ -z "$1" ]; then
  echo "use: $0 <folder>"
  exit 1
fi

DIR=$1

if [ ! -d "$DIR" ]; then
  echo "folder $DIR doesn't exist"
  exit 1
fi

find "$DIR" -type f -exec file -i {} \; | awk -F'charset=' '{print $2}' | sort | uniq

echo "Binary files:"
find "$DIR" -type f -exec file -i {} \; | grep 'charset=binary' | awk -F: '{print $1}'

echo "Ascii files:"
find "$DIR" -type f -exec file -i {} \; | grep 'charset=us-ascii' | awk -F: '{print $1}'
