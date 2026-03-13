#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

int randomInt(int low, int high)
{
    return low + rand() % (high - low + 1);
}

int main()
{
    srand(time(0));

    int K = randomInt(2, 6);
    int N = randomInt(5, 12);
    int MAX_TIME = 100;
    int MAX_CHARGE = 40;

    cout << "K " << K << endl;

    // Generate prices (increasing with port speed)
    cout << "P ";
    vector<int> prices(K);

    int base = randomInt(5, 15);
    for (int i = 0; i < K; i++)
    {
        base += randomInt(3, 10);
        prices[i] = base;
        cout << prices[i] << " ";
    }
    cout << endl;

    // Generate vehicle requests
    for (int i = 1; i <= N; i++)
    {

        int arrival = randomInt(0, MAX_TIME - 20);
        int charge_time = randomInt(5, MAX_CHARGE);

        int min_departure = arrival + charge_time;
        int departure = randomInt(min_departure, min_departure + 30);

        cout << "V " << i << " "
             << arrival << " "
             << departure << " "
             << charge_time << endl;
    }

    return 0;
}