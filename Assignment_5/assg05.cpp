#include <bits/stdc++.h>
using namespace std;

struct Course
{
    int id;
    int start;
    int end;
    int duration;
    vector<int> valid_starts;
};

long long key(int c, int r, int t)
{
    return (long long)c * 1000000LL + r * 1000LL + t;
}

void cnf(string filename, int vars, vector<vector<int>> &clauses)
{
    ofstream out(filename);

    out << "p cnf " << vars << " " << clauses.size() << "\n";

    for (auto &c : clauses)
    {
        for (int x : c)
            out << x << " ";
        out << "0\n";
    }
}

vector<Course> parse_courses(vector<vector<int>> raw)
{
    vector<Course> courses;

    for (auto &r : raw)
    {
        Course c;
        c.id = r[0];
        c.start = r[1];
        c.end = r[2];
        c.duration = r[3];

        for (int t = c.start; t <= c.end - c.duration + 1; t++)
            c.valid_starts.push_back(t);

        courses.push_back(c);
    }

    return courses;
}

pair<vector<vector<int>>, int> first_encoding(int rooms, vector<Course> &courses)
{
    unordered_map<long long, int> var;
    int id = 1;

    for (auto &c : courses)
        for (int r = 1; r <= rooms; r++)
            for (int t : c.valid_starts)
                var[key(c.id, r, t)] = id++;

    vector<vector<int>> clauses;

    for (auto &c : courses)
    {
        vector<int> group;

        for (int r = 1; r <= rooms; r++)
            for (int t : c.valid_starts)
                group.push_back(var[key(c.id, r, t)]);

        clauses.push_back(group);

        for (int i = 0; i < group.size(); i++)
            for (int j = i + 1; j < group.size(); j++)
                clauses.push_back({-group[i], -group[j]});
    }

    int max_day = 0;
    for (auto &c : courses)
        max_day = max(max_day, c.end);

    for (int r = 1; r <= rooms; r++)
    {
        for (int d = 1; d <= max_day; d++)
        {
            vector<int> overlap;

            for (auto &c : courses)
                for (int s : c.valid_starts)
                    if (d >= s && d < s + c.duration)
                        overlap.push_back(var[key(c.id, r, s)]);

            for (int i = 0; i < overlap.size(); i++)
                for (int j = i + 1; j < overlap.size(); j++)
                    clauses.push_back({-overlap[i], -overlap[j]});
        }
    }

    return {clauses, id - 1};
}

pair<vector<vector<int>>, int> second_encoding(int rooms, vector<Course> &courses)
{
    unordered_map<long long, int> r_var;
    unordered_map<long long, int> t_var;

    int id = 1;

    for (auto &c : courses)
        for (int r = 1; r <= rooms; r++)
            r_var[(long long)c.id * 1000 + r] = id++;

    for (auto &c : courses)
        for (int t : c.valid_starts)
            t_var[(long long)c.id * 1000 + t] = id++;

    vector<vector<int>> clauses;

    for (auto &c : courses)
    {
        vector<int> lits;

        for (int r = 1; r <= rooms; r++)
            lits.push_back(r_var[(long long)c.id * 1000 + r]);

        clauses.push_back(lits);

        for (int i = 0; i < lits.size(); i++)
            for (int j = i + 1; j < lits.size(); j++)
                clauses.push_back({-lits[i], -lits[j]});
    }

    for (auto &c : courses)
    {
        vector<int> lits;

        for (int t : c.valid_starts)
            lits.push_back(t_var[(long long)c.id * 1000 + t]);

        clauses.push_back(lits);

        for (int i = 0; i < lits.size(); i++)
            for (int j = i + 1; j < lits.size(); j++)
                clauses.push_back({-lits[i], -lits[j]});
    }

    for (int r = 1; r <= rooms; r++)
    {
        for (int i = 0; i < courses.size(); i++)
        {
            for (int j = i + 1; j < courses.size(); j++)
            {
                auto &c1 = courses[i];
                auto &c2 = courses[j];

                for (int s1 : c1.valid_starts)
                {
                    for (int s2 : c2.valid_starts)
                    {
                        if (s1 < s2 + c2.duration && s2 < s1 + c1.duration)
                        {
                            clauses.push_back({-r_var[(long long)c1.id * 1000 + r],
                                               -r_var[(long long)c2.id * 1000 + r],
                                               -t_var[(long long)c1.id * 1000 + s1],
                                               -t_var[(long long)c2.id * 1000 + s2]});
                        }
                    }
                }
            }
        }
    }

    return {clauses, id - 1};
}

vector<vector<int>> random_courses(int n)
{
    vector<vector<int>> raw;

    mt19937 rng(42);

    uniform_int_distribution<int> dur(2, 5);
    uniform_int_distribution<int> start(1, 10);
    uniform_int_distribution<int> ext(2, 10);

    for (int i = 1; i <= n; i++)
    {
        int d = dur(rng);
        int s = start(rng);
        int e = s + d + ext(rng);

        raw.push_back({i, s, e, d});
    }

    return raw;
}

void generate_tests()
{
    mt19937 rng(42);

    uniform_int_distribution<int> rooms(2, 4);
    uniform_int_distribution<int> courses_n(4, 10);

    for (int test = 1; test <= 100; test++)
    {
        int m = rooms(rng);
        int n = courses_n(rng);

        auto raw = random_courses(n);

        auto courses = parse_courses(raw);

        auto enc1 = first_encoding(m, courses);
        auto enc2 = second_encoding(m, courses);

        string f1 = "" + to_string(test) + "_encoding1.cnf";
        string f2 = "" + to_string(test) + "_encoding2.cnf";

        cnf(f1, enc1.second, enc1.first);
        cnf(f2, enc2.second, enc2.first);
    }

    cout << "Generated 100 SAT test cases.\n";
}

int main()
{
    generate_tests();
}