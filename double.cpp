// Exhaustive two-point insertions into labelled seeds of order 14.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

using Mask = std::uint32_t;
using Count = std::uint64_t;
constexpr int max_n = 16;
constexpr int capacity = 1 << max_n;
using Order = std::array<Mask, max_n>;

int first(Mask mask) { return __builtin_ctz(mask); }
Mask one(int v) { return Mask{1} << v; }
Mask universe(int n) { return one(n) - 1; }

void validate_seed(const Order& pred) {
    for (int v = 0; v < 14; ++v) {
        if (pred[v] & (~universe(14) | one(v))) {
            throw std::runtime_error("seed predecessor mask outside its domain");
        }
        for (Mask lower = pred[v]; lower; lower &= lower - 1) {
            if (pred[first(lower)] & ~pred[v]) {
                throw std::runtime_error("seed predecessor masks are not transitively closed");
            }
        }
    }
}

Order reverse(const Order& pred, int n) {
    Order succ{};
    for (int v = 0; v < n; ++v) {
        for (Mask p = pred[v]; p; p &= p - 1) succ[first(p)] |= one(v);
    }
    return succ;
}

Mask available(const Order& pred, Mask mask, int n) {
    Mask result = 0;
    for (Mask unseen = universe(n) ^ mask; unseen; unseen &= unseen - 1) {
        int v = first(unseen);
        if (!(pred[v] & ~mask)) result |= one(v);
    }
    return result;
}

struct Work {
    std::array<std::uint32_t, capacity> seen{};
    std::array<Mask, capacity> next{};
    std::array<Count, capacity> forward{};
    std::array<Count, capacity> backward{};
    std::uint32_t stamp = 0;

    void generate(const Order& pred, int n, std::vector<Mask>& ideals) {
        if (++stamp == 0) {
            seen.fill(0);
            ++stamp;
        }
        ideals.clear();
        ideals.push_back(0);
        seen[0] = stamp;
        for (std::size_t k = 0; k < ideals.size(); ++k) {
            Mask mask = ideals[k];
            next[mask] = available(pred, mask, n);
            for (Mask a = next[mask]; a; a &= a - 1) {
                Mask larger = mask | (a & -a);
                if (seen[larger] != stamp) {
                    seen[larger] = stamp;
                    ideals.push_back(larger);
                }
            }
        }
    }
};

bool connected(const Order& pred, int n) {
    Order succ = reverse(pred, n);
    Mask touched = 1;
    Mask queue = 1;
    while (queue) {
        int v = first(queue);
        queue &= queue - 1;
        Mask unseen = universe(n) & ~(pred[v] | succ[v] | touched);
        touched |= unseen;
        queue |= unseen;
    }
    return touched == universe(n);
}

template <typename Callback>
void each_insertion(const Order& pred, int n, Work& work,
                    std::vector<Mask>& ideals, std::vector<Mask>& filters,
                    Callback&& callback) {
    work.generate(pred, n, ideals);
    Order succ = reverse(pred, n);
    work.generate(succ, n, filters);
    for (Mask lower : ideals) {
        Mask eligible = universe(n);
        for (Mask left = lower; left; left &= left - 1) eligible &= succ[first(left)];
        for (Mask upper : filters) {
            if (upper & (~eligible | lower)) continue;
            Order extended = pred;
            extended[n] = lower;
            for (Mask right = upper; right; right &= right - 1) {
                extended[first(right)] |= one(n);
            }
            callback(extended);
        }
    }
}

struct Balance {
    Count best = 0;
    Count total = 1;
    int x = 0;
    int y = 0;
};

Balance exact_balance(const Order& pred, Work& work, std::vector<Mask>& ideals) {
    work.generate(pred, max_n, ideals);
    for (Mask mask : ideals) {
        work.forward[mask] = 0;
        work.backward[mask] = 0;
    }
    Mask full = universe(max_n);
    work.backward[full] = 1;
    for (auto it = ideals.rbegin(); it != ideals.rend(); ++it) {
        Mask mask = *it;
        for (Mask a = work.next[mask]; a; a &= a - 1) {
            work.backward[mask] += work.backward[mask | (a & -a)];
        }
    }
    std::array<std::array<Count, max_n>, max_n> counts{};
    work.forward[0] = 1;
    for (Mask mask : ideals) {
        for (Mask available = work.next[mask]; available; available &= available - 1) {
            int v = first(available);
            Mask next = mask | one(v);
            work.forward[next] += work.forward[mask];
            Count ways = work.forward[mask] * work.backward[next];
            for (Mask past = mask & ~pred[v]; past; past &= past - 1) {
                counts[first(past)][v] += ways;
            }
        }
    }
    Balance result{0, work.backward[0], 0, 0};
    if (result.total != work.forward[full]) throw std::runtime_error("inconsistent counts");
    for (int i = 0; i < max_n; ++i) {
        for (int j = i + 1; j < max_n; ++j) {
            if ((pred[i] & one(j)) || (pred[j] & one(i))) continue;
            if (counts[i][j] + counts[j][i] != result.total) {
                throw std::runtime_error("inconsistent pair counts");
            }
            Count smaller = std::min(counts[i][j], counts[j][i]);
            if (smaller > result.best) result = {smaller, result.total, i, j};
        }
    }
    return result;
}

bool smaller(const Balance& a, const Balance& b) {
    return static_cast<__uint128_t>(a.best) * b.total
         < static_cast<__uint128_t>(b.best) * a.total;
}

int main() {
    static Work work;
    std::vector<Mask> lower14, upper14, lower15, upper15, ideals;
    std::uint64_t cases = 0, connected_cases = 0, failures = 0;
    int seeds = 0;
    Balance best{1, 1, 0, 0};
    Order witness{};
    Order base{};
    while (std::cin >> base[0]) {
        for (int i = 1; i < 14; ++i) {
            if (!(std::cin >> base[i])) throw std::runtime_error("truncated seed");
        }
        validate_seed(base);
        ++seeds;
        each_insertion(base, 14, work, lower14, upper14, [&](const Order& first_extension) {
            each_insertion(first_extension, 15, work, lower15, upper15, [&](const Order& second) {
                ++cases;
                if (!connected(second, 16)) return;
                ++connected_cases;
                Balance candidate = exact_balance(second, work, ideals);
                if (smaller(candidate, best)) {
                    best = candidate;
                    witness = second;
                }
                if (candidate.best * 3 < candidate.total) ++failures;
            });
        });
        std::cerr << "seeds " << seeds << ", cases " << cases << ", connected " << connected_cases << '\n';
    }
    if (seeds == 0 || connected_cases == 0) throw std::runtime_error("empty search");
    Count divisor = std::gcd(best.best, best.total);
    std::cout << "{\"seeds\":" << seeds << ",\"cases\":" << cases
              << ",\"connected\":" << connected_cases << ",\"counterexamples\":" << failures
              << ",\"minimum\":{\"balance\":\"" << best.best / divisor << '/' << best.total / divisor
              << "\",\"extensions\":" << best.total << ",\"pair\":[" << best.x << ',' << best.y
              << "],\"pred\":[";
    for (int i = 0; i < max_n; ++i) std::cout << (i ? "," : "") << witness[i];
    std::cout << "]}}\n";
}
