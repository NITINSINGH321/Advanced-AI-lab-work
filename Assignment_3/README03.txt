#   Command:
   $ g++ assg03.cpp -o assg03

# USAGE
   The program takes input parameters exclusively from the command line.

   Syntax:
   $ ./assg03 <CASE> <input_file> <N> <K_chat> <K_gem> <Cost_chat> <Cost_gem> <Days_limit>

   Parameters:
   <CASE>       : 'A' for Single-Task Mode (Case A)
                  'B' for Multi-Task/Delayed-Sharing Mode (Case B)
   <input_file> : Path to the input file containing assignment dependencies.
   <N>          : Number of students in the group.
   <K_chat>     : Daily prompt limit for ChatGPT (Even-indexed assignments).
   <K_gem>      : Daily prompt limit for Gemini (Odd-indexed assignments).
   <Cost_chat>  : Cost per prompt for ChatGPT.
   <Cost_gem>   : Cost per prompt for Gemini.
   <Days_limit> : Target deadline (m) for cost optimization analysis.

   Example (Case A):
   $ ./assg03 A input.txt 3 5 5 10 20 10

   Example (Case B):
   $ ./assg03 B input.txt 3 5 5 10 20 10
