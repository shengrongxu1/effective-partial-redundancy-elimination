# effective-partial-redundancy-elimination

This is final project for EECS 583.

For the testings, first, in the `benchmarks` folder we need to comment out all the functions except these lines in runOnFunction **except** these following lines to just print out the dynamic operation count of the unoptimized code:
```
BlockFrequencyInfo &bfi = getAnalysis<BlockFrequencyInfoWrapperPass>().getBFI();
BranchProbabilityInfo &bpi = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();

printDynInstcount(&F, &bfi, &bpi);
```

and then build via `sh building.sh`, one can get the print information
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

so 10 for the unoptimized version.
