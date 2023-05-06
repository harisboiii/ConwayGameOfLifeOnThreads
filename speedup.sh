#!/bin/bash

# compile the program
gcc -pthread -o cgwol_threads cgwol_threads.c

# set the input file name
input_file="Random"

# set the range of threads to use
min_threads=1
max_threads=8

# set the number of iterations to run for each number of threads
num_iterations=5

# create a directory to store the results
mkdir -p results

# iterate over the range of threads
for (( t=$min_threads; t<=$max_threads; t++ ))
do
    # set the number of threads
    export OMP_NUM_THREADS=$t

    # run the program multiple times and calculate the average execution time
    total_time=0
    for (( i=1; i<=$num_iterations; i++ ))
    do
        # measure the execution time
        time_start=$(date +%s%N)
        ./cgwol_threads $input_file > /dev/null
        time_end=$(date +%s%N)
        time_diff=$(( $time_end - $time_start ))

        # add the execution time to the total
        total_time=$(( $total_time + $time_diff ))
    done
    avg_time=$(( $total_time / $num_iterations ))

    # write the result to a file
    echo "$t $avg_time" >> results/speedup.txt
done

