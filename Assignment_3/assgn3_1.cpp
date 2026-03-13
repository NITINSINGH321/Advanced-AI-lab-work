/*
 * CS5205: Advanced AI Lab - Assignment 3 Solver
 * Solves scheduling for ChatGPT/Gemini under Group Subscription constraints.
 * Implements A* with heuristics for Earliest Completion and Min-Cost Subscription.
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <algorithm>
#include <queue>
#include <cmath>
#include <climits>
#include <bitset>

using namespace std;

// ==========================================
// Data Structures
// ==========================================

enum ModelType
{
    CHATGPT,
    GEMINI
};

struct Task
{
    int id;                   // Original ID
    int internal_id;          // 0 to NumTasks-1 for bitmasking
    int cost;                 // Prompt count
    ModelType model;          // derived from id % 2
    vector<int> dependencies; // List of internal_ids
};

struct Subscription
{
    int limit_gpt;
    int limit_gemini;

    bool operator<(const Subscription &other) const
    {
        if (limit_gpt != other.limit_gpt)
            return limit_gpt < other.limit_gpt;
        return limit_gemini < other.limit_gemini;
    }
};

// State for A* Search
struct State
{
    unsigned long long completed_mask; // Bitmask of tasks finished BEFORE today

    // For comparison in set/map
    bool operator<(const State &other) const
    {
        return completed_mask < other.completed_mask;
    }

    bool operator==(const State &other) const
    {
        return completed_mask == other.completed_mask;
    }
};

struct Node
{
    State state;
    int g; // Days passed (Cost so far)
    int f; // g + h (Estimated total cost)

    // For Priority Queue (Min-Heap based on f)
    bool operator>(const Node &other) const
    {
        return f > other.f;
    }
};

// ==========================================
// Global Variables & Config
// ==========================================
vector<Task> tasks;
int N_students = 0; // Will be set via command line or default
int num_tasks = 0;
map<int, int> id_map; // Map original ID -> internal ID

// ==========================================
// Parsing Logic
// ==========================================

void parseInput(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }

    string line;
    int internal_counter = 0;

    // First pass: Read Tasks and create mapping
    while (getline(file, line))
    {
        if (line.empty() || line[0] == '%' || line[0] == '$')
            continue;

        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "A")
        {
            int id, cost;
            ss >> id >> cost;

            Task t;
            t.id = id;
            t.cost = cost;
            t.model = (id % 2 == 0) ? CHATGPT : GEMINI;
            t.internal_id = internal_counter;

            id_map[id] = internal_counter;
            tasks.push_back(t);
            internal_counter++;
        }
    }

    num_tasks = tasks.size();

    // Second Pass (or smarter parsing): Let's just parse line by line and store raw deps
    // Reset file
    file.clear();
    file.seekg(0);
    tasks.clear();
    id_map.clear();
    internal_counter = 0;

    while (getline(file, line))
    {
        if (line.empty() || line[0] == '%' || line[0] == '$')
            continue;

        stringstream ss(line);
        string token;
        ss >> token;

        if (token == "N")
        {
            // Ignore N from file as per instructions, but useful default
            int val;
            ss >> val; // N_students = val;
        }
        else if (token == "K")
        {
            // Ignore K
        }
        else if (token == "A")
        {
            int id, cost;
            ss >> id >> cost;

            Task t;
            t.id = id;
            t.cost = cost;
            t.model = (id % 2 == 0) ? CHATGPT : GEMINI;
            t.internal_id = internal_counter;
            id_map[id] = internal_counter;

            int dep;
            vector<int> raw_deps;
            while (ss >> dep && dep != 0)
            {
                raw_deps.push_back(dep);
            }

            t.dependencies = raw_deps;

            tasks.push_back(t);
            internal_counter++;
        }
    }
    num_tasks = tasks.size();

    // Fix dependencies mapping
    for (auto &t : tasks)
    {
        vector<int> mapped_deps;
        for (int raw_id : t.dependencies)
        {
            if (id_map.find(raw_id) != id_map.end())
            {
                mapped_deps.push_back(id_map[raw_id]);
            }
        }
        t.dependencies = mapped_deps;
    }
}

// ==========================================
// Heuristics
// ==========================================
vector<int> critical_path_cache;

int get_critical_path(int u)
{
    if (critical_path_cache[u] != -1)
        return critical_path_cache[u];

    int max_child_path = 0;

    int max_depth = 0;
    for (int i = 0; i < num_tasks; i++)
    {
        // If task i depends on u
        bool depends = false;
        for (int dep : tasks[i].dependencies)
        {
            if (dep == u)
            {
                depends = true;
                break;
            }
        }
        if (depends)
        {
            max_depth = max(max_depth, get_critical_path(i));
        }
    }

    return critical_path_cache[u] = 1 + max_depth;
}

void precompute_heuristics()
{
    critical_path_cache.assign(num_tasks, -1);
    for (int i = 0; i < num_tasks; i++)
    {
        get_critical_path(i);
    }
}

// Heuristic Function h(n)
int calculate_heuristic(const State &state, const Subscription &sub)
{
    // H1: Resource Bottleneck
    long long gpt_needed = 0;
    long long gem_needed = 0;
    int max_chain = 0;

    for (int i = 0; i < num_tasks; i++)
    {
        if (!((state.completed_mask >> i) & 1))
        {
            // Task not done
            if (tasks[i].model == CHATGPT)
                gpt_needed += tasks[i].cost;
            else
                gem_needed += tasks[i].cost;

            max_chain = max(max_chain, critical_path_cache[i]);
        }
    }

    int days_gpt = (sub.limit_gpt > 0) ? (int)ceil((double)gpt_needed / sub.limit_gpt) : (gpt_needed > 0 ? 9999 : 0);
    int days_gem = (sub.limit_gemini > 0) ? (int)ceil((double)gem_needed / sub.limit_gemini) : (gem_needed > 0 ? 9999 : 0);

    return max({days_gpt, days_gem, max_chain});
}

// ==========================================
// Solver Logic (A*)
// ==========================================

// Helper: Check if task dependencies are met in the 'completed' mask
bool are_deps_met(int task_idx, unsigned long long completed_mask)
{
    for (int dep : tasks[task_idx].dependencies)
    {
        if (!((completed_mask >> dep) & 1))
            return false;
    }
    return true;
}
void generate_moves(
    int idx,
    const vector<int> &candidates,
    int current_gpt,
    int current_gem,
    int current_students,
    unsigned long long current_mask,
    vector<unsigned long long> &valid_moves,
    bool is_case_a)
{
    if (idx == candidates.size())
    {
        if (current_mask > 0)
            valid_moves.push_back(current_mask);
        return;
    }

    int task_id = candidates[idx];
    const Task &t = tasks[task_id];

    // Option 1: Don't do this task
    generate_moves(idx + 1, candidates, current_gpt, current_gem, current_students, current_mask, valid_moves, is_case_a);

    // Option 2: Do this task (if resources allow)
    bool resource_ok = false;
    if (t.model == CHATGPT && current_gpt >= t.cost)
        resource_ok = true;
    if (t.model == GEMINI && current_gem >= t.cost)
        resource_ok = true;

    bool student_ok = true;
    if (is_case_a)
    {
        if (current_students < 1)
            student_ok = false;
    }

    if (resource_ok && student_ok)
    {
        int next_gpt = (t.model == CHATGPT) ? current_gpt - t.cost : current_gpt;
        int next_gem = (t.model == GEMINI) ? current_gem - t.cost : current_gem;
        int next_stud = is_case_a ? current_students - 1 : current_students;

        generate_moves(idx + 1, candidates, next_gpt, next_gem, next_stud, current_mask | (1ULL << task_id), valid_moves, is_case_a);
    }
}

int solve_earliest_completion(Subscription sub, bool is_case_a, int max_search_depth = 100)
{
    priority_queue<Node, vector<Node>, greater<Node>> pq;
    map<unsigned long long, int> visited_cost; // mask -> g

    State start_state = {0};
    pq.push({start_state, 0, calculate_heuristic(start_state, sub)});
    visited_cost[0] = 0;

    int min_days = INT_MAX;

    while (!pq.empty())
    {
        Node current = pq.top();
        pq.pop();

        // FIX: Access completed_mask via current.state
        unsigned long long current_mask = current.state.completed_mask;

        if (current.g >= min_days)
            continue;

        // Check if all tasks are done
        if (current_mask == (1ULL << num_tasks) - 1)
        {
            if (current.g < min_days)
                min_days = current.g;
            continue;
        }

        // Pruning: If we found a path to this state with lower cost, skip
        if (visited_cost.find(current_mask) != visited_cost.end() &&
            visited_cost[current_mask] < current.g)
        {
            continue;
        }

        // Identify available tasks
        vector<int> candidates;
        for (int i = 0; i < num_tasks; i++)
        {
            if (!((current_mask >> i) & 1))
            {
                if (are_deps_met(i, current_mask))
                {
                    candidates.push_back(i);
                }
            }
        }

        if (candidates.empty())
        {
            continue;
        }

        vector<unsigned long long> moves;
        generate_moves(0, candidates, sub.limit_gpt, sub.limit_gemini, N_students, 0, moves, is_case_a);

        for (unsigned long long move_mask : moves)
        {
            // FIX: Use current_mask variable
            unsigned long long new_completed = current_mask | move_mask;
            int new_g = current.g + 1;

            if (visited_cost.find(new_completed) == visited_cost.end() || visited_cost[new_completed] > new_g)
            {
                visited_cost[new_completed] = new_g;
                int h = calculate_heuristic({new_completed}, sub);
                pq.push({{new_completed}, new_g, new_g + h});
            }
        }
    }

    return (min_days == INT_MAX) ? -1 : min_days;
}

// ==========================================
// Main Logic
// ==========================================

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        cout << "Usage: " << argv[0] << " <input_file> <c1> <c2> [options]" << endl;
        cout << "Example: " << argv[0] << " input.txt 10 20 -caseA -earliest <gpt_lim> <gem_lim> <N>" << endl;
        cout << "Example: " << argv[0] << " input.txt 10 20 -caseB -best <days> <N>" << endl;
        return 1;
    }

    string filename = argv[1];
    double c1 = stod(argv[2]); // Cost ChatGPT
    double c2 = stod(argv[3]); // Cost Gemini

    // Parse args
    string mode = "-earliest";
    bool case_a = false;
    int arg_idx = 4;

    // Default dummy values
    int limit_gpt = 5;
    int limit_gem = 5;
    int max_days = 10;

    // Simple Argument Parser
    for (int i = 4; i < argc; i++)
    {
        string arg = argv[i];
        if (arg == "-caseA")
            case_a = true;
        else if (arg == "-caseB")
            case_a = false;
        else if (arg == "-earliest")
        {
            mode = "earliest";
            if (i + 3 < argc)
            {
                limit_gpt = stoi(argv[++i]);
                limit_gem = stoi(argv[++i]);
                N_students = stoi(argv[++i]);
            }
        }
        else if (arg == "-best")
        {
            mode = "best";
            if (i + 2 < argc)
            {
                max_days = stoi(argv[++i]);
                N_students = stoi(argv[++i]);
            }
        }
    }

    parseInput(filename);
    precompute_heuristics();

    cout << "Loaded " << num_tasks << " tasks." << endl;
    cout << "Scenario: " << (case_a ? "Case A (1 task/day/student)" : "Case B (Multi-task)") << endl;

    if (mode == "earliest")
    {
        cout << "Solving for Earliest Completion..." << endl;
        cout << "Subscription: GPT=" << limit_gpt << ", Gemini=" << limit_gem << ", Students=" << N_students << endl;

        Subscription sub = {limit_gpt, limit_gem};
        int days = solve_earliest_completion(sub, case_a);

        if (days == -1)
            cout << "No solution found." << endl;
        else
            cout << "Earliest Completion: " << days << " days." << endl;
    }
    else if (mode == "best")
    {
        cout << "Solving for Best Subscription (Min Cost) within " << max_days << " days..." << endl;
        cout << "Costs: c1=" << c1 << ", c2=" << c2 << endl;

        int max_gpt_needed = 0;
        int max_gem_needed = 0;
        int total_gpt = 0;
        int total_gem = 0;

        for (const auto &t : tasks)
        {
            if (t.model == CHATGPT)
            {
                max_gpt_needed = max(max_gpt_needed, t.cost);
                total_gpt += t.cost;
            }
            else
            {
                max_gem_needed = max(max_gem_needed, t.cost);
                total_gem += t.cost;
            }
        }

        double min_cost = 1e9;
        Subscription best_sub = {-1, -1};

        for (int l_gpt = max_gpt_needed; l_gpt <= max(max_gpt_needed + 10, total_gpt); l_gpt++)
        {
            for (int l_gem = max_gem_needed; l_gem <= max(max_gem_needed + 10, total_gem); l_gem++)
            {

                double current_cost = l_gpt * c1 + l_gem * c2;
                if (current_cost >= min_cost)
                    continue; // Pruning based on cost

                Subscription sub = {l_gpt, l_gem};
                int days = solve_earliest_completion(sub, case_a);

                if (days != -1 && days <= max_days)
                {
                    if (current_cost < min_cost)
                    {
                        min_cost = current_cost;
                        best_sub = sub;
                        cout << "Found better: GPT=" << l_gpt << ", GEM=" << l_gem << " => Cost=" << min_cost << endl;
                    }
                }
            }
        }

        if (best_sub.limit_gpt != -1)
        {
            cout << "Best Subscription: GPT=" << best_sub.limit_gpt << ", Gemini=" << best_sub.limit_gemini << endl;
            cout << "Daily Cost: " << min_cost << endl;
        }
        else
        {
            cout << "No feasible subscription found within " << max_days << " days." << endl;
        }
    }

    return 0;
}