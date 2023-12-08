#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Utilizare: bash $0 <character>"
    exit 1
fi

character=$1
counter=0

function is_valid() {
    local line=$1
    [[ $line =~ ^[A-Z] ]] &&                 
    [[ $line =~ [a-zA-Z0-9\ \!\?\.]+$ ]] &&       
    [[ $line =~ (\.|\!|\?)$ ]] &&                  
    [[ ! $line =~ ,\ și ]] &&                      
    [[ $line == *"$character"* ]]                   
}

while IFS= read -r line || [[ -n "$line" ]]; do
    if is_valid "$line"; then
        ((counter++))
    fi
done

echo "$counter"

