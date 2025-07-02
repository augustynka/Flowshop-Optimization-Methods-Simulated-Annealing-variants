#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <fstream>
#include <numeric>

using namespace std;
using Clock = chrono::high_resolution_clock;
using Micro = chrono::microseconds;

struct Task {
    int id;
    int p1, p2;    // czasy operacji na M1 i M2
    int due;       // termin wykonania
};

struct Downtime {
    int start;
    int end;
};

bool overlapsDowntime(int start, int duration, const vector<Downtime>& downtimes) {
    for (const auto& d : downtimes) {
        if (!(start + duration <= d.start || start >= d.end)) return true;
    }
    return false;
}

int calculateLmax(const vector<Task>& tasks, const vector<int>& perm, const vector<Downtime>& downtimes) {
    int n = tasks.size();
    int timeM1 = 0;
    int timeM2 = 0;
    vector<int> startTimes(n);

    for (int idx : perm) {
        const Task& t = tasks[idx];
        int s1 = timeM1;
        int s2 = s1 + t.p1;
        while (s2 < timeM2 || overlapsDowntime(s2, t.p2, downtimes)) {
            ++s1;
            s2 = s1 + t.p1;
        }
        startTimes[idx] = s1;
        timeM1 = s1 + t.p1;
        timeM2 = s2 + t.p2;
    }

    int Lmax = 0;
    for (int i = 0; i < n; ++i) {
        const Task& t = tasks[i];
        int Cj = startTimes[i] + t.p1 + t.p2;
        Lmax = max(Lmax, max(0, Cj - t.due));
    }
    return Lmax;
}

int simulatedAnnealing(vector<Task>& tasks, const vector<Downtime>& downtimes,
                       int maxIter = 50000, double temp0 = 1000.0, double alpha = 0.998) {
    int n = tasks.size();
    vector<int> perm(n);
    iota(perm.begin(), perm.end(), 0);
    // losowa permutacja startowa
    random_device rd;
    mt19937 gen(rd());
    shuffle(perm.begin(), perm.end(), gen);

    uniform_real_distribution<> prob(0.0, 1.0);
    uniform_int_distribution<> swapDist(0, n - 1);

    double temp = temp0;
    int bestL = calculateLmax(tasks, perm, downtimes);
    int currL = bestL;
    int stagnation = 0;
    const int reheatingPeriod = maxIter / 10; 

    for (int iter = 1; iter <= maxIter; ++iter) {
        auto newPerm = perm;
        int i = swapDist(gen);
        int j = swapDist(gen);
        if (i != j) swap(newPerm[i], newPerm[j]);

        int newL = calculateLmax(tasks, newPerm, downtimes);
        double acceptProb = exp((currL - newL) / temp);
        if (newL < currL || prob(gen) < acceptProb) {
            perm = newPerm;
            currL = newL;
            stagnation = 0;
            if (newL < bestL) bestL = newL;
        } else {
            stagnation++;
        }

        temp *= alpha;
        if (iter % reheatingPeriod == 0 || stagnation >= reheatingPeriod / 2) {
            temp = temp0;
            stagnation = 0;
        }
    }
    return bestL;
}

vector<Task> generateTasks(int n, int pmin, int pmax, double tightness = 1.0) {
    vector<Task> tasks;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> d1(pmin, pmax), d2(pmin, pmax);
    double avgLen = (pmin + pmax) / 2.0;
    for (int i = 0; i < n; ++i) {
        int op1 = d1(gen);
        int op2 = d2(gen);
        int est = i * int(avgLen);
        int slack = int((op1 + op2) * tightness);
        int due = est + (slack > 0 ? gen() % slack : 0);
        tasks.push_back({i, op1, op2, due});
    }
    return tasks;
}

vector<Downtime> generateDowntimes(int n, int pmax, int h, int tau) {
    int horizon = 2 * n * pmax;
    vector<Downtime> dts;
    for (int t = tau; t < horizon; t += tau)
        dts.push_back({t, t + h});
    return dts;
}

// Blok testowy SA
void runTestBlock(const string& label, const vector<int>& values, ofstream& out,
                  int fixed_n, int fixed_h, int fixed_tau,
                  int pmin, int pmax, double tightness = 1.0,
                  int repeats = 5) {
    int idGlobal = 1;
    for (int v : values) {
        int n    = (label == "n"     ? v : fixed_n);
        int h    = (label == "h"     ? v : fixed_h);
        int tau  = (label == "tau"   ? v : fixed_tau);
        int maxP = (label == "range" ? v : pmax);

        auto tasks = generateTasks(n, pmin, maxP, tightness);
        auto dts   = generateDowntimes(n, maxP, h, tau);

        cout << "Instancja [" << label << " = " << v << "]:\n";
        cout << "  Zadania (id, p1, p2, due):\n";
        for (auto &t : tasks) {
            cout << "    (" << t.id
                 << ", " << t.p1
                 << ", " << t.p2
                 << ", " << t.due << ")\n";
        }
        cout << "  Okresy niedostępności (start--end):\n";
        for (auto &d : dts) {
            cout << "    " << d.start << "--" << d.end << "\n";
        }
        cout << "----------------------------------------\n";

        // SA wielokrotnie na tej samej instancji
        for (int r = 0; r < repeats; ++r) {
            auto t1 = chrono::high_resolution_clock::now();
            int lmax = simulatedAnnealing(tasks, dts);
            auto t2 = chrono::high_resolution_clock::now();
            auto us = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

            // zapisz wynik do pliku CSV
            out << label      << ','
                << idGlobal  << ','
                << n         << ','
                << h         << ','
                << tau       << ','
                << pmin      << ','
                << maxP      << ','
                << tightness << ','
                << lmax      << ','
                << us        << '\n';

            ++idGlobal;
        }
    }
}

int main() {
    ofstream out("SA7_Lmax_results_155_0988.csv");
    out << "TypTestu,TestID,n,h,tau,Pmin,Pmax,Tight,Lmax,TimeUs\n";
    runTestBlock("n", {5,10,15,20,25}, out, 10,3,20,1,5,1.0);
    runTestBlock("h", {1,3,5,7,9}, out, 10,3,20,1,5,1.0);
    runTestBlock("tau", {10,20,30,40,50}, out,10,3,20,1,5,1.0);
    runTestBlock("range",{3,5,10,15,20},out,10,3,20,1,5,1.0);
    out.close();
    cout<<"SA_Lmax_results.csv generated"<<endl;
    return 0;
}
