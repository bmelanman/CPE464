#!/bin/zsh

clear

echo "-----"

find ./diff -type f -delete
find ./results -type f -delete

cd ../ref_pcap/ || exit 1
TEST_FILES=(*.pcap)

cd ../testing || exit 1

NUM_DIFFS=0

for FILE in "${TEST_FILES[@]}"; do

    ../cmake-build-debug/trace "../ref_pcap/$FILE" > "./results/$FILE.out"

    diff "../ref_output/$FILE.out" "./results/$FILE.out" > "./diff/$FILE.diff"

    if [ -s "./diff/$FILE.diff" ]; then
            # The file is not-empty
            NUM_DIFFS=$((NUM_DIFFS + 1))
    fi
done

if [ $NUM_DIFFS -eq 0 ]; then
    printf "There are no errors!\n"
else
    printf "There where %d diff files :( \n" $NUM_DIFFS
fi
