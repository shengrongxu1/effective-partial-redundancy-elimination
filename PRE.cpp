// LCM
//  Optimal Computation Points
// 1. Safe-Earliest Transformation: insert h = t at every entry of node n satisfying DSafe & Earliest and replace each t by h
// (1) down-safe (DSafe): computation of t at n does not introduce a new value on a terminating path starting in n.
// (2) Earliest: path from start to end, no node m prior to n is safe and delivers the same value as n when computing t.
// 
//  Suppress Unnecessary Code Motion
// 2. Latest Transformation: insert h = t at every entry of node n satisfying Delay & Latest and replace each t by h
// (3) Delay (from DSafe and Earliest) --> Latest: values
// 
//  LCM Transformatio
// 3. Optimal Transformation
// (4) Isolated & Latest --> OCP; RO --> insert h = t at every entry of node n in OCP and replace each t by h in RO.