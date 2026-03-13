#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <cmath>
#include <iomanip>

using namespace std;

// --- Data Structures ---

struct Assignment
{
    int id;
    int prompts;
    vector<int> dependencies;
    string llm_type; // "ChatGPT" or "Gemini"
    int cost_type;   // 1 for ChatGPT, 2 for Gemini
};

// Represents a state in the search space
struct State
{
    vector<bool> completed;       // Mask of completed assignments (index 0 = A1)
    int g;                        // Cost so far (Days)
    double h;                     // Heuristic estimate
    double f;                     // f = g + h
    vector<vector<int>> schedule; // Log of schedule: [[1, 7], [2, 4]...]

    // For priority queue (Min-Heap based on f)
    bool operator>(const State &other) const
    {
        return f > other.f;
    }
};

// --- Global Variables (Populated from Input) ---
map<int, Assignment> assignments; // ID -> Assignment
int total_assignments = 0;
int max_id = 0;

// --- Helper Functions ---

// Determine LLM type based on ID (Even=ChatGPT, Odd=Gemini)
string get_llm_type(int id)
{
    return (id % 2 == 0) ? "ChatGPT" : "Gemini";
}

// Parse Input File
void parse_input(string filename)
{
    ifstream infile(filename);
    if (!infile.is_open())
    {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }

    string line;
    while (getline(infile, line))
    {
        if (line.empty() || line[0] == '%' || line[0] == '$')
            continue;

        stringstream ss(line);
        string type;
        ss >> type;

        // Ignore N and K lines inside file as per instructions
        if (type == "N" || type == "K")
            continue;

        if (type == "A")
        {
            Assignment a;
            ss >> a.id >> a.prompts;

            int dep;
            while (ss >> dep && dep != 0)
            {
                a.dependencies.push_back(dep);
            }

            a.llm_type = get_llm_type(a.id);
            a.cost_type = (a.id % 2 == 0) ? 1 : 2;

            assignments[a.id] = a;
            if (a.id > max_id)
                max_id = a.id;
        }
    }
    total_assignments = assignments.size();
    infile.close();
}

// Check if a task's dependencies are met
bool are_dependencies_met(int id, const vector<bool> &completed_mask)
{
    for (int dep : assignments[id].dependencies)
    {
        if (!completed_mask[dep])
            return false;
    }
    return true;
}

// --- Heuristics ---

double calculate_heuristic(const vector<bool> &completed, int K1, int K2)
{
    // H1: Resource Bound
    int rem_prompts_chat = 0;
    int rem_prompts_gem = 0;

    for (auto const &pair : assignments)
    {
        int id = pair.first;
        const Assignment &task = pair.second;

        if (!completed[id])
        {
            if (task.cost_type == 1)
                rem_prompts_chat += task.prompts;
            else
                rem_prompts_gem += task.prompts;
        }
    }

    double days_chat = (K1 > 0) ? ceil((double)rem_prompts_chat / K1) : (rem_prompts_chat > 0 ? 999 : 0);
    double days_gem = (K2 > 0) ? ceil((double)rem_prompts_gem / K2) : (rem_prompts_gem > 0 ? 999 : 0);

    return max(days_chat, days_gem);
}

// --- Solver (A*) ---

struct SolverResult
{
    int days;
    double cost;
    vector<vector<int>> schedule;
    long long nodes_expanded;
};

// Generates valid subsets of tasks for a single day
void generate_moves(
    vector<int> &candidates,
    int index,
    vector<int> &current_batch,
    int current_k1,
    int current_k2,
    int max_k1,
    int max_k2,
    int N_limit,
    vector<vector<int>> &valid_moves)
{
    // If we have considered all candidates
    if (index == candidates.size())
    {
        if (!current_batch.empty())
        {
            valid_moves.push_back(current_batch);
        }
        return;
    }

    int task_id = candidates[index];
    int p = assignments[task_id].prompts;
    int type = assignments[task_id].cost_type;

    // Option 1: Include this task (if resources allow)
    bool can_take = false;
    if (current_batch.size() < N_limit)
    {
        if (type == 1 && current_k1 + p <= max_k1)
            can_take = true;
        if (type == 2 && current_k2 + p <= max_k2)
            can_take = true;
    }

    if (can_take)
    {
        current_batch.push_back(task_id);
        int new_k1 = (type == 1) ? current_k1 + p : current_k1;
        int new_k2 = (type == 2) ? current_k2 + p : current_k2;

        generate_moves(candidates, index + 1, current_batch, new_k1, new_k2, max_k1, max_k2, N_limit, valid_moves);

        current_batch.pop_back(); // Backtrack
    }

    // Option 2: Skip this task
    generate_moves(candidates, index + 1, current_batch, current_k1, current_k2, max_k1, max_k2, N_limit, valid_moves);
}

SolverResult solve_scenario(int N, int K1, int K2, char case_type)
{
    priority_queue<State, vector<State>, greater<State>> pq;
    map<vector<bool>, int> visited; // State -> Min g-score

    // Initial State
    vector<bool> initial_completed(max_id + 1, false);
    initial_completed[0] = true; // Dummy node

    State start;
    start.completed = initial_completed;
    start.g = 0;
    start.h = calculate_heuristic(initial_completed, K1, K2);
    start.f = start.g + start.h;

    pq.push(start);
    long long nodes = 0;

    while (!pq.empty())
    {
        State current = pq.top();
        pq.pop();
        nodes++;

        // Check Goal
        bool all_done = true;
        for (auto const &pair : assignments)
        {
            int id = pair.first;
            if (!current.completed[id])
            {
                all_done = false;
                break;
            }
        }
        if (all_done)
        {
            return {current.g, 0.0, current.schedule, nodes};
        }

        // Pruning (Closed List)
        if (visited.count(current.completed) && visited[current.completed] <= current.g)
        {
            continue;
        }
        visited[current.completed] = current.g;

        // 1. Identify Candidates (Ready tasks)
        vector<int> candidates;
        for (auto const &pair : assignments)
        {
            int id = pair.first;
            if (!current.completed[id])
            {
                if (are_dependencies_met(id, current.completed))
                {
                    candidates.push_back(id);
                }
            }
        }

        if (candidates.empty())
            continue;

        // 2. Generate Valid Moves (Subsets)
        vector<vector<int>> moves;
        vector<int> batch;

        // CASE A: Limit = N (one per student)
        // CASE B: Limit = Unlimited (bound by prompts)
        int batch_limit = (case_type == 'A') ? N : 100;

        generate_moves(candidates, 0, batch, 0, 0, K1, K2, batch_limit, moves);

        for (const auto &move : moves)
        {
            if (move.empty())
                continue;

            vector<bool> next_completed = current.completed;
            for (int id : move)
                next_completed[id] = true;

            int new_g = current.g + 1;

            if (visited.count(next_completed) && visited[next_completed] <= new_g)
                continue;

            State next_state;
            next_state.completed = next_completed;
            next_state.g = new_g;
            next_state.h = calculate_heuristic(next_completed, K1, K2);
            next_state.f = next_state.g + next_state.h;
            next_state.schedule = current.schedule;
            next_state.schedule.push_back(move);

            pq.push(next_state);
        }
    }

    return {-1, 0, {}, nodes}; // Fail
}

// --- Main ---

int main(int argc, char *argv[])
{
    // Check if arguments are sufficient
    if (argc < 9)
    {
        cout << "Usage: " << argv[0] << " <CASE> <input_file> <N> <K_chat> <K_gem> <Cost_chat> <Cost_gem> <Days_limit_M>" << endl;
        cout << "   <CASE>: 'A' or 'B'" << endl;
        cout << "Example: " << argv[0] << " A input.txt 3 5 5 10 20 5" << endl;
        return 1;
    }

    // Parse Command Line Arguments
    char case_selector = argv[1][0]; // First char of first argument
    string input_file = argv[2];
    int N = stoi(argv[3]);
    int K1 = stoi(argv[4]);    // ChatGPT Limit
    int K2 = stoi(argv[5]);    // Gemini Limit
    double C1 = stod(argv[6]); // Cost ChatGPT
    double C2 = stod(argv[7]); // Cost Gemini
    int M = stoi(argv[8]);     // Deadline

    // Validate Case Selector
    if (case_selector != 'A' && case_selector != 'B')
    {
        cerr << "Error: Invalid Case specified. Use 'A' or 'B'." << endl;
        return 1;
    }

    parse_input(input_file);

    cout << "--- Solving for CASE " << case_selector << " ---" << endl;
    cout << "Input File: " << input_file << endl;
    cout << "Assignments: " << total_assignments << endl;
    cout << "Group Size (N): " << N << endl;
    cout << "Limits (K): ChatGPT=" << K1 << ", Gemini=" << K2 << endl;
    cout << "Costs (C): ChatGPT=" << C1 << ", Gemini=" << C2 << endl;
    cout << "---------------------\n"
         << endl;

    SolverResult res = solve_scenario(N, K1, K2, case_selector);

    if (res.days != -1)
    {
        cout << "Earliest Completion: " << res.days << " days." << endl;
        cout << "Nodes Expanded: " << res.nodes_expanded << endl;
        cout << "Schedule:" << endl;
        double total_cost = 0;
        for (size_t i = 0; i < res.schedule.size(); ++i)
        {
            cout << "  Day " << i + 1 << ": ";
            for (int id : res.schedule[i])
            {
                cout << "A" << id << "(" << get_llm_type(id) << ") ";
                total_cost += (assignments[id].cost_type == 1 ? C1 * assignments[id].prompts : C2 * assignments[id].prompts);
            }
            cout << endl;
        }
        cout << "Total Cost: " << total_cost << endl;
    }
    else
    {
        cout << "No feasible solution found." << endl;
    }

    return 0;
}