#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <z3++.h>

using namespace std;
using namespace z3;

struct Vehicle
{
    int id;
    int arrival;
    int departure;
    int charge_time;
};

int main()
{
    int K;
    string label;

    // Read number of ports
    if (!(cin >> label >> K))
        return 0;

    // Read prices
    vector<int> P(K);
    cin >> label;
    for (int i = 0; i < K; i++)
        cin >> P[i];

    // Read vehicle requests
    vector<Vehicle> vehicles;
    while (cin >> label && label == "V")
    {
        Vehicle v;
        cin >> v.id >> v.arrival >> v.departure >> v.charge_time;
        vehicles.push_back(v);
    }

    int N = vehicles.size();

    context c;
    optimize opt(c);

    vector<expr> ports;
    vector<expr> starts;
    vector<expr> durations;
    vector<expr> costs;

    // Create decision variables
    for (int i = 0; i < N; i++)
    {

        ports.push_back(c.int_const(("port_" + to_string(i)).c_str()));
        starts.push_back(c.int_const(("start_" + to_string(i)).c_str()));
        durations.push_back(c.int_const(("duration_" + to_string(i)).c_str()));
        costs.push_back(c.int_const(("cost_" + to_string(i)).c_str()));

        // Port range constraint
        opt.add(ports[i] >= 1 && ports[i] <= K);

        // Start time constraint
        opt.add(starts[i] >= vehicles[i].arrival);

        // Must finish before departure
        opt.add(starts[i] + durations[i] <= vehicles[i].departure);

        // Duration must be positive
        opt.add(durations[i] > 0);

        // Cost must be non-negative
        opt.add(costs[i] >= 0);

        // Use ite to model duration and cost based on selected port
        expr duration_expr = c.int_val(0);
        expr cost_expr = c.int_val(0);

        for (int k = 1; k <= K; k++)
        {

            int actual_duration = (vehicles[i].charge_time + k - 1) / k; // ceil
            int actual_cost = P[k - 1] * actual_duration;

            duration_expr = ite(ports[i] == k,
                                c.int_val(actual_duration),
                                duration_expr);

            cost_expr = ite(ports[i] == k,
                            c.int_val(actual_cost),
                            cost_expr);
        }

        opt.add(durations[i] == duration_expr);
        opt.add(costs[i] == cost_expr);
    }

    // Non-overlapping constraint
    for (int i = 0; i < N; i++)
    {
        for (int j = i + 1; j < N; j++)
        {

            expr no_overlap =
                (starts[i] + durations[i] <= starts[j]) ||
                (starts[j] + durations[j] <= starts[i]);

            opt.add(implies(ports[i] == ports[j], no_overlap));
        }
    }

    // Minimize total cost
    expr total_cost = c.int_val(0);
    for (int i = 0; i < N; i++)
        total_cost = total_cost + costs[i];

    opt.minimize(total_cost);

    // Solve
    if (opt.check() == sat)
    {
        model m = opt.get_model();

        cout << "Optimal Schedule Found\n";
        cout << "Minimum Total Cost: " << m.eval(total_cost) << "\n";

        for (int i = 0; i < N; i++)
        {
            cout << "Vehicle " << vehicles[i].id
                 << " -> Port: " << m.eval(ports[i])
                 << " | Start: " << m.eval(starts[i])
                 << " | Duration: " << m.eval(durations[i])
                 << " | Cost: " << m.eval(costs[i])
                 << "\n";
        }
    }
    else
    {
        cout << "No feasible schedule exists within given constraints.\n";
    }

    return 0;
}