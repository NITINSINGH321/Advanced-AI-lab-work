#include <bits/stdc++.h>
using namespace std;

struct Course
{
    int id;
    int start;
    int deadline;
    int duration;
};

vector<Course> courses;

int M;     // rooms
int N;     // courses
int T = 0; // maximum day

vector<vector<int>> clauses;

/* ---------- Encoding 1 variable mapping ---------- */
/* z(i,j,t) */
int var_z(int i, int j, int t)
{
    return (i - 1) * (M * T) + (j - 1) * T + t;
}

/* ---------- Encoding 2 variable mapping ---------- */
/* x(i,j) */
int var_x(int i, int j)
{
    return (i - 1) * M + j;
}

/* y(i,t) */
int var_y(int i, int t)
{
    return N * M + (i - 1) * T + t;
}

/* ---------- Check time overlap ---------- */
bool overlap(int s1, int d1, int s2, int d2)
{
    int e1 = s1 + d1 - 1;
    int e2 = s2 + d2 - 1;

    return (s1 <= e2 && s2 <= e1);
}

/* ---------- Write DIMACS ---------- */
void write_dimacs(string filename, int variables)
{

    ofstream fout(filename);

    fout << "p cnf " << variables << " " << clauses.size() << "\n";

    for (auto &c : clauses)
    {
        for (int x : c)
            fout << x << " ";
        fout << "0\n";
    }

    fout.close();
}

void encoding1()
{

    clauses.clear();

    for (int i = 1; i <= N; i++)
    {

        vector<int> clause;

        int s = courses[i - 1].start;
        int d = courses[i - 1].deadline;
        int dur = courses[i - 1].duration;

        for (int j = 1; j <= M; j++)
        {

            for (int t = s; t <= d - dur + 1; t++)
            {

                clause.push_back(var_z(i, j, t));
            }
        }

        clauses.push_back(clause);
    }

    /* At most one start */

    for (int i = 1; i <= N; i++)
    {

        int s = courses[i - 1].start;
        int d = courses[i - 1].deadline;
        int dur = courses[i - 1].duration;

        vector<int> starts;

        for (int j = 1; j <= M; j++)
            for (int t = s; t <= d - dur + 1; t++)
                starts.push_back(var_z(i, j, t));

        for (int a = 0; a < starts.size(); a++)
            for (int b = a + 1; b < starts.size(); b++)
                clauses.push_back({-starts[a], -starts[b]});
    }

    /* Room conflicts */

    for (int i = 1; i <= N; i++)
    {

        for (int k = i + 1; k <= N; k++)
        {

            int s1 = courses[i - 1].start;
            int d1 = courses[i - 1].deadline;
            int dur1 = courses[i - 1].duration;

            int s2 = courses[k - 1].start;
            int d2 = courses[k - 1].deadline;
            int dur2 = courses[k - 1].duration;

            for (int j = 1; j <= M; j++)
            {

                for (int t1 = s1; t1 <= d1 - dur1 + 1; t1++)
                {

                    for (int t2 = s2; t2 <= d2 - dur2 + 1; t2++)
                    {

                        if (overlap(t1, dur1, t2, dur2))
                        {

                            clauses.push_back({-var_z(i, j, t1),
                                               -var_z(k, j, t2)});
                        }
                    }
                }
            }
        }
    }

    int variables = N * M * T;

    write_dimacs("encoding1.cnf", variables);
}

void encoding2()
{

    clauses.clear();

    /* Room assignment */

    for (int i = 1; i <= N; i++)
    {

        vector<int> clause;

        for (int j = 1; j <= M; j++)
            clause.push_back(var_x(i, j));

        clauses.push_back(clause);

        for (int j1 = 1; j1 <= M; j1++)
            for (int j2 = j1 + 1; j2 <= M; j2++)
                clauses.push_back({-var_x(i, j1), -var_x(i, j2)});
    }

    /* Start time */

    for (int i = 1; i <= N; i++)
    {

        int s = courses[i - 1].start;
        int d = courses[i - 1].deadline;
        int dur = courses[i - 1].duration;

        vector<int> clause;

        for (int t = s; t <= d - dur + 1; t++)
            clause.push_back(var_y(i, t));

        clauses.push_back(clause);

        for (int a = 0; a < clause.size(); a++)
            for (int b = a + 1; b < clause.size(); b++)
                clauses.push_back({-clause[a], -clause[b]});
    }

    /* Room conflict */

    for (int i = 1; i <= N; i++)
    {

        for (int k = i + 1; k <= N; k++)
        {

            int s1 = courses[i - 1].start;
            int d1 = courses[i - 1].deadline;
            int dur1 = courses[i - 1].duration;

            int s2 = courses[k - 1].start;
            int d2 = courses[k - 1].deadline;
            int dur2 = courses[k - 1].duration;

            for (int j = 1; j <= M; j++)
            {

                for (int t1 = s1; t1 <= d1 - dur1 + 1; t1++)
                {

                    for (int t2 = s2; t2 <= d2 - dur2 + 1; t2++)
                    {

                        if (overlap(t1, dur1, t2, dur2))
                        {

                            clauses.push_back({-var_x(i, j),
                                               -var_y(i, t1),
                                               -var_x(k, j),
                                               -var_y(k, t2)});
                        }
                    }
                }
            }
        }
    }

    int variables = N * M + N * T;

    write_dimacs("encoding2.cnf", variables);
}

// Main function
int main()
{

    cin >> M;
    cin >> N;

    courses.resize(N);

    for (int i = 0; i < N; i++)
    {

        string C;
        cin >> C;

        cin >> courses[i].id >> courses[i].start >> courses[i].deadline >> courses[i].duration;

        T = max(T, courses[i].deadline);
    }

    encoding1();
    encoding2();

    cout << "DIMACS files generated:\n";
    cout << "encoding1.cnf\n";
    cout << "encoding2.cnf\n";

    return 0;
}