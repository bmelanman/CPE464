#!/bin/bash

export DEBUG=0
NUM_DIFFS=0

# Ensure program is up to date
make trace -C ../

# Clear folders, or create them if they don't exist
find ./diff -type f -delete > /dev/null || mkdir diff
find ./results -type f -delete > /dev/null || mkdir results

# Grab out test files' names
cd ../ref_pcap/ || exit 1
TEST_FILES=(*.pcap)

cd ../testing || exit 1

clear

echo "-----"

# Iterate through each file
for FILE in "${TEST_FILES[@]}"; do

    # Run teh file through the program
    ../trace "../ref_pcap/$FILE" > "./results/$FILE.txt"

    # Check for differences between the program output and the corresponding reference file
    diff "../ref_output/$FILE.out" "./results/$FILE.txt" > "./diff/$FILE.diff"

    # If the file is empty, delete it, otherwise increase the diff count
    if [ -s "./diff/$FILE.diff" ]; then
        # The file is not-empty
        NUM_DIFFS=$((NUM_DIFFS + 1))
    else
        rm -f "./diff/$FILE.diff"
    fi
done

# Report results to the user
if [ $NUM_DIFFS -eq 0 ]; then
    printf "There are no errors!\n"
else
    printf "There where %d diff files :( \n" $NUM_DIFFS
fi

# Clean up after ourselves
make clean -C ../ > /dev/null
