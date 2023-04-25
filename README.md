# effective-partial-redundancy-elimination

This is final project for EECS 583.

For testing, take the example of a simple ifElse case (test_3.c). First, we need to comment out all the functions **except** these following lines in `runOnFunction` of `PRE.cpp` in `mypass` folder to just print out the dynamic operation count of the unoptimized code:
```
BlockFrequencyInfo &bfi = getAnalysis<BlockFrequencyInfoWrapperPass>().getBFI();
BranchProbabilityInfo &bpi = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();

printDynInstcount(&F, &bfi, &bpi);
```

and then build via `sh building.sh` in the `benchmarks` folder, one can get the printed information after `sh run.sh test_3`:
```
main,block count : 1
block count : 0
block count : 1
block count : 1
10
-e 
=== Correctness Check ===
diff: fplicm_output: No such file or directory
-e >> PASS

-e 1. Performance of unoptimized code
Command exited with non-zero status 6
0.00user 0.00system 0:00.00elapsed 84%CPU (0avgtext+0avgdata 1592maxresident)k
0inputs+0outputs (0major+71minor)pagefaults 0swaps
-e 


-e 2. Performance of optimized code
Command exited with non-zero status 6
0.00user 0.00system 0:00.00elapsed 91%CPU (0avgtext+0avgdata 1580maxresident)k
0inputs+0outputs (0major+71minor)pagefaults 0swaps
-e
```

so 10 for the unoptimized version. Then by applying our implementations of forward proparation, just uncomment all lines in `runOnFunction` and rebuild via `sh building.sh` and again `sh run.sh test_3`, one can get:
```
main,block count : 1
block count : 0
block count : 0
block count : 1
block count : 0
block count : 1
11
-e 
=== Correctness Check ===
diff: fplicm_output: No such file or directory
-e >> PASS

-e 1. Performance of unoptimized code
Command exited with non-zero status 6
0.00user 0.00system 0:00.00elapsed 90%CPU (0avgtext+0avgdata 1612maxresident)k
0inputs+0outputs (0major+73minor)pagefaults 0swaps
-e 


-e 2. Performance of optimized code
Command exited with non-zero status 6
0.00user 0.00system 0:00.00elapsed 91%CPU (0avgtext+0avgdata 1588maxresident)k
0inputs+0outputs (0major+72minor)pagefaults 0swaps
-e
```
and now we turn to 11. After this, again comment out all the functional lines in `runOnFunction` of `PRE.cpp` in `mypass` folder to just print out the dynamic operation count of the optimized code and type `sh run_2.sh test_3` for printing the information of the final version:
```
main,block count : 1
block count : 0
block count : 1
block count : 1
8
```
and now it comes to 8.
When you run all the `sh xxx.sh` lines, stay in the `benchmarks` folder.

Plus: please specify your own `PATH2LIB` in `run.sh` and `run_2.sh` before sh anything!
