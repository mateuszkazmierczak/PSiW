#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <random>
#include <chrono>

static const int N = 5;

enum class State {
    THINKING,
    HUNGRY,
    EATING
};

State state[N];
int foodEaten[N];
std::mutex mtx;
std::condition_variable cv[N];

inline int leftNeighbor(int i) {
    return (i + N - 1) % N;
}

inline int rightNeighbor(int i) {
    return (i + 1) % N;
}

inline int getPriority(int i) {
    return -foodEaten[i];
}

void test(int i) {

    int left = leftNeighbor(i);
    int right = rightNeighbor(i);

    if (state[i] == State::HUNGRY &&
        state[left] != State::EATING &&
        state[right] != State::EATING)
    {
        bool canEat = true;
        
        if (state[left] == State::HUNGRY) {
            if (getPriority(left) > getPriority(i)) {
                canEat = false;
            }
        }
        if (state[right] == State::HUNGRY) {
            if (getPriority(right) > getPriority(i)) {
                canEat = false;
            }
        }

        if (canEat) {
            state[i] = State::EATING;
            cv[i].notify_one(); //może jeść
        }
    }
}

void takeForks(int i) {
    std::unique_lock<std::mutex> lock(mtx);
    state[i] = State::HUNGRY;

    test(i);

    while (state[i] != State::EATING) {
        cv[i].wait(lock);
    }
}

void putForks(int i) {
    std::unique_lock<std::mutex> lock(mtx);
    state[i] = State::THINKING;
    test(leftNeighbor(i));
    test(rightNeighbor(i));
}


void hungryMan(int i, int rounds) 
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> eatDist(100, 500);  // ms
    std::uniform_int_distribution<> thinkDist(100, 500);
    std::uniform_int_distribution<> weightDist(1, 5);

    for(int r = 0; r < rounds; ++r) {
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkDist(gen)));

        takeForks(i);
        int eatTime = eatDist(gen);
        int mealWeight = weightDist(gen); 
        std::cout << "Głodomor " << i << " je. Waga posilku: " << mealWeight 
                  << " (dotychczas zjedzone: " << foodEaten[i] << ")"
                  << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(eatTime));
        {
            std::lock_guard<std::mutex> lk(mtx);
            foodEaten[i] += mealWeight;
        }
        putForks(i);
    }
}

int main() {
    for(int i = 0; i < N; ++i) {
        state[i] = State::THINKING;
        foodEaten[i] = 0;
    }

    std::vector<std::thread> threads;
    int rounds = 10;

    for(int i = 0; i < N; ++i) {
        threads.emplace_back(hungryMan, i, rounds);
    }

    for(auto &th : threads) {
        th.join();
    }

    std::cout << "\n--- PODSUMOWANIE ---\n";
    for(int i = 0; i < N; ++i) {
        std::cout << "Głodomor " << i 
                  << " zjadl lacznie: " << foodEaten[i] 
                  << std::endl;
    }

    return 0;
}
