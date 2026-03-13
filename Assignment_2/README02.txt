1. OVERVIEW
--------------------------------------------------------------------------------
This program solves the assignment scheduling problem by modeling dependencies 
as a Directed Acyclic Graph (DAG). It extends Assignment 1 by performing 
optimizations (Minimum Days or Minimum Prompts) and supporting a "Delayed Handoff" 
scenario where solutions are exchanged only on the next day.

The code supports four distinct modes of operation:
  Mode 1: Find Earliest Completion Time (Standard Handoff)
  Mode 2: Find Best Subscription/Min K (Standard Handoff)
  Mode 3: Find Earliest Completion Time (Delayed Handoff)
  Mode 4: Find Best Subscription/Min K (Delayed Handoff)

2. COMPILATION
--------------------------------------------------------------------------------
To compile the C++ code, use a standard compiler like g++:

    g++ -std=c++17 -O2 assg02.cpp -o assg02

This will create an executable named `assg02` (or `assg02.exe` on Windows).

3. USAGE & INPUT FORMAT
--------------------------------------------------------------------------------
The program ignores the `N` and `K` values inside the input text file. Instead, 
you must provide these parameters via command-line arguments.

Command Syntax:
    ./assg02 <input_filename> <N_students> <Value> <Mode>

Arguments:
  <input_filename> : Path to the text file containing assignment data.
  <N_students>     : Number of students in the group.
  <Value>          : Interpreted based on the Mode (see below).
                     - If Mode is 1 or 3, this is K (Prompts per day).
                     - If Mode is 2 or 4, this is D (Deadline in days).
  <Mode>           : Integer flag (1-4) selecting the operation.

4. MODES OF OPERATION
--------------------------------------------------------------------------------

[Mode 1] Minimum Days (Standard Handoff)
   * Goal: Calculate the fastest time to finish given fixed resources.
   * Logic: When a task is finished, dependent tasks unlock IMMEDIATELY 
     on the same day (if prompts remain).
   * Usage Example (3 Students, 5 Prompts/day):
     ./assg02 input.txt 3 5 1

[Mode 2] Minimum Prompts / Best Subscription (Standard Handoff)
   * Goal: Find the smallest K needed to finish within a Deadline.
   * Logic: Uses Binary Search to find the optimal K.
   * Usage Example (3 Students, Deadline 5 days):
     ./assg02 input.txt 3 5 2

[Mode 3] Minimum Days (Delayed Handoff / Scenario)
   * Goal: Fastest time under the "6am Exchange" rule.
   * Logic: When a task is finished, dependent tasks unlock ONLY on the 
     NEXT day.
   * Usage Example (3 Students, 5 Prompts/day):
     ./assg02 input.txt 3 5 3

[Mode 4] Minimum Prompts / Best Subscription (Delayed Handoff / Scenario)
   * Goal: Smallest K needed to finish within a Deadline under "6am Exchange".
   * Logic: Binary Search for K combined with delayed unlocking.
   * Usage Example (3 Students, Deadline 5 days):
     ./assg02 input.txt 3 5 4

5. ALGORITHM DETAILS
--------------------------------------------------------------------------------
* Graph Parsing: The code reads the input file into a DAG structure. 
  Dependencies are tracked using `indegree` counts.
* Scheduling Heuristic: A "First Fit Decreasing" greedy strategy is used. 
  Tasks are sorted by cost (highest first) to prioritize "heavy" assignments, 
  ensuring efficient packing of the daily prompt budget.
* Optimization: For Modes 2 and 4, Binary Search is used over the range of 
  possible K values (from max_task_cost to total_cost) to efficiently find 
  the minimum feasible K.

6. SAMPLE INPUT FILE FORMAT
--------------------------------------------------------------------------------
The input file should follow the format from Assignment 1:
    % Comments are ignored
    N 3   % Ignored by this code (passed via CLI)
    K 5   % Ignored by this code (passed via CLI)
    A 1 2 0
    A 2 4 1 0
    ...