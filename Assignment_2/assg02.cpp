#include <bits/stdc++.h>
using namespace std;

// --- Data Structures ---
struct Task
{
    int id;
    int cost;
    vector<int> dependencies; // Parents
    vector<int> dependents;   // Children (Adjacency List)
};

map<int, Task> all_tasks;
vector<int> all_task_ids;

// --- Input Parsing ---
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
        if (line.empty())
            continue;

        // Remove comments
        size_t comment_pos = line.find('%');
        if (comment_pos != string::npos)
        {
            line = line.substr(0, comment_pos);
        }
        if (line.empty())
            continue;

        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "A")
        {
            Task t;
            ss >> t.id >> t.cost;
            int dep;
            while (ss >> dep && dep != 0)
            {
                t.dependencies.push_back(dep);
            }
            all_tasks[t.id] = t;
            all_task_ids.push_back(t.id);
        }
    }

    // Build dependents list
    for (int id : all_task_ids)
    {
        for (int parent : all_tasks[id].dependencies)
        {
            if (all_tasks.find(parent) != all_tasks.end())
            {
                all_tasks[parent].dependents.push_back(id);
            }
        }
    }
    infile.close();
}

// Returns the number of days needed to complete all tasks given N and K.
int simulate_days(int N, int K, bool delayed_handoff)
{

    // 1. Setup State
    map<int, int> current_indegree;
    for (int id : all_task_ids)
    {
        current_indegree[id] = 0;
        for (int parent : all_tasks[id].dependencies)
        {
            if (all_tasks.find(parent) != all_tasks.end())
                current_indegree[id]++;
        }
        // Impossible check
        if (all_tasks[id].cost > K)
            return INT_MAX;
    }

    vector<int> ready_queue;

    // Initial population
    for (int id : all_task_ids)
    {
        if (current_indegree[id] == 0)
            ready_queue.push_back(id);
    }

    int days = 0;
    int tasks_completed_count = 0;
    int total_tasks = all_tasks.size();

    // Loop until all tasks are done
    while (tasks_completed_count < total_tasks)
    {
        days++;
        if (days > 20000)
            return INT_MAX; // Failsafe

        // Reset students for the new day
        vector<int> student_capacity(N, K);

        vector<int> next_pass_additions;

        vector<int> next_day_additions;

        bool task_done_this_cycle = true;

        while (task_done_this_cycle)
        {
            task_done_this_cycle = false;
            next_pass_additions.clear();

            // Sort ready_queue descending by cost (Best Fit Heuristic)
            sort(ready_queue.begin(), ready_queue.end(), [](int a, int b)
                 { return all_tasks[a].cost > all_tasks[b].cost; });

            // Try to assign tasks
            for (auto it = ready_queue.begin(); it != ready_queue.end();)
            {
                int tid = *it;
                int cost = all_tasks[tid].cost;
                bool assigned = false;

                for (int s = 0; s < N; ++s)
                {
                    if (student_capacity[s] >= cost)
                    {
                        // Assign Task
                        student_capacity[s] -= cost;
                        assigned = true;
                        tasks_completed_count++;

                        // Handle Dependencies
                        for (int child : all_tasks[tid].dependents)
                        {
                            current_indegree[child]--;
                            if (current_indegree[child] == 0)
                            {
                                if (delayed_handoff)
                                {
                                    next_day_additions.push_back(child);
                                }
                                else
                                {
                                    // Immediate mode: Queue for next pass within SAME day
                                    next_pass_additions.push_back(child);
                                }
                            }
                        }

                        // Remove executed task from queue
                        it = ready_queue.erase(it);
                        task_done_this_cycle = true;
                        break; // Stop looking for a student for this task
                    }
                }
                if (!assigned)
                {
                    ++it;
                }
            }

            // Add newly unlocked tasks for the immediate mode loop
            if (!delayed_handoff && !next_pass_additions.empty())
            {
                ready_queue.insert(ready_queue.end(), next_pass_additions.begin(), next_pass_additions.end());
            }

            if (delayed_handoff)
                break;
        }

        // End of Day: Add tasks unlocked for tomorrow (Delayed Mode)
        if (delayed_handoff)
        {
            ready_queue.insert(ready_queue.end(), next_day_additions.begin(), next_day_additions.end());
        }
    }

    return days;
}

// --- Solvers ---

void solve_min_days(int N, int K, bool delayed)
{
    int days = simulate_days(N, K, delayed);
    if (days == INT_MAX)
    {
        cout << "Impossible with K=" << K << " (Some task cost > K)" << endl;
    }
    else
    {
        cout << "Earliest Completion Time: " << days << " days." << endl;
    }
}

void solve_min_k(int N, int D, bool delayed)
{
    // Binary Search for K
    int max_cost = 0;
    int total_cost = 0;
    for (int id : all_task_ids)
    {
        max_cost = max(max_cost, all_tasks[id].cost);
        total_cost += all_tasks[id].cost;
    }

    int low = max_cost;
    int high = total_cost;
    int ans = -1;

    while (low <= high)
    {
        int mid = low + (high - low) / 2;
        int d = simulate_days(N, mid, delayed);

        if (d <= D)
        {
            ans = mid;
            high = mid - 1; // Try smaller K
        }
        else
        {
            low = mid + 1; // Need more K
        }
    }

    if (ans != -1)
    {
        cout << "Best Subscription (Min Prompts): " << ans << endl;
    }
    else
    {
        cout << "Cannot complete within " << D << " days even with max resources." << endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        cout << "Usage: " << argv[0] << " <input_file> <N> <Val> <Mode>" << endl;
        cout << "Modes:\n 1: Min Days (Std) [Val=K]\n 2: Min K (Std) [Val=Deadline]\n";
        cout << " 3: Min Days (Delayed) [Val=K]\n 4: Min K (Delayed) [Val=Deadline]" << endl;
        return 1;
    }

    string filename = argv[1];
    int N = stoi(argv[2]);
    int Val = stoi(argv[3]);
    int Mode = stoi(argv[4]);

    parse_input(filename);

    cout << "--- Assignment 2 Solver ---" << endl;
    cout << "Students (N): " << N << endl;

    switch (Mode)
    {
    case 1: // Min Days, Standard
        cout << "Mode: Standard Handoff | Given K=" << Val << endl;
        solve_min_days(N, Val, false);
        break;
    case 2: // Min K, Standard
        cout << "Mode: Standard Handoff | Deadline=" << Val << " days" << endl;
        solve_min_k(N, Val, false);
        break;
    case 3: // Min Days, Delayed
        cout << "Mode: Delayed Handoff (Scenario) | Given K=" << Val << endl;
        solve_min_days(N, Val, true);
        break;
    case 4: // Min K, Delayed
        cout << "Mode: Delayed Handoff (Scenario) | Deadline=" << Val << " days" << endl;
        solve_min_k(N, Val, true);
        break;
    default:
        cerr << "Invalid Mode selected." << endl;
        return 1;
    }

    return 0;
}