// Independent boundary check by relation masks and linear extensions.
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

struct Rational {
    int num;
    int den;
};

bool less(Rational x, Rational y) {
    return std::int64_t(x.num) * y.den < std::int64_t(y.num) * x.den;
}

struct Interval {
    Rational lo{1, 1};
    Rational hi{0, 0}; // den == 0 represents no upper endpoint

    bool constrain(int slope, int constant) {
        if (!slope) return constant >= 0;
        if (slope > 0) {
            Rational cut{-constant, slope};
            if (less(lo, cut)) lo = cut;
        } else {
            Rational cut{constant, -slope};
            if (!hi.den || less(cut, hi)) hi = cut;
        }
        return !hi.den || !less(hi, lo);
    }
};

struct OrderData {
    int n;
    std::array<unsigned, 7> pred{};
    std::vector<std::pair<int, int>> pairs;

    bool closed() const {
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < i; ++j)
                if ((pred[i] & (1u << j)) && (pred[j] & ~pred[i])) return false;
        return true;
    }

    bool connected() {
        std::array<unsigned, 7> neighbours{};
        for (int i = 0; i < n; ++i) for (int j = 0; j < i; ++j) {
            if (pred[i] & (1u << j)) continue;
            pairs.emplace_back(j, i);
            neighbours[i] |= 1u << j;
            neighbours[j] |= 1u << i;
        }
        unsigned reach = 1, pending = 1;
        while (pending) {
            int i = __builtin_ctz(pending);
            pending &= pending - 1;
            pending |= neighbours[i] & ~reach;
            reach |= neighbours[i];
        }
        return reach == (1u << n) - 1;
    }

    std::pair<int, int> check() const {
        // Enumerate topological orders, and record which pairs each one orients.
        std::vector<std::array<int, 7>> ranks;
        std::array<int, 7> position{};
        auto visit = [&](auto&& self, unsigned placed, int next) -> void {
            if (next == n) {
                ranks.push_back(position);
                return;
            }
            for (int v = 0; v < n; ++v) {
                if ((placed & (1u << v)) || (pred[v] & ~placed)) continue;
                position[v] = next + 1;
                self(self, placed | (1u << v), next + 1);
            }
        };
        visit(visit, 0, 0);
        const int total = static_cast<int>(ranks.size());
        std::vector<std::vector<int>> events(pairs.size());
        for (int j = 0; j < static_cast<int>(pairs.size()); ++j) {
            auto [u, v] = pairs[j];
            for (int k = 0; k < total; ++k)
                if (ranks[k][u] < ranks[k][v]) events[j].push_back(k);
        }
        int ideals = 0, failures = 0;
        std::vector<int> offsets(total);
        std::vector<Interval> good;
        for (unsigned used = 0; used < (1u << n); ++used) {
            bool ideal = true;
            for (unsigned set = used; set; set &= set - 1)
                if (pred[__builtin_ctz(set)] & ~used) ideal = false;
            if (!ideal) continue;
            ++ideals;
            int all_weight = 0;
            for (int k = 0; k < total; ++k) {
                int last = 0;
                for (unsigned set = used; set; set &= set - 1)
                    last = std::max(last, ranks[k][__builtin_ctz(set)]);
                offsets[k] = n - last;
                all_weight += offsets[k];
            }
            good.clear();
            bool covered = false;
            for (const auto& event : events) {
                int weight = 0;
                for (int k : event) weight += offsets[k];
                int count = static_cast<int>(event.size());
                Interval interval;
                if (!interval.constrain(3 * count - total, 3 * weight - all_weight)
                    || !interval.constrain(2 * total - 3 * count, 2 * all_weight - 3 * weight))
                    continue;
                if (!interval.hi.den && interval.lo.num == interval.lo.den) {
                    covered = true;
                    break;
                }
                good.push_back(interval);
            }
            if (!covered) {
                std::sort(good.begin(), good.end(), [](auto a, auto b) { return less(a.lo, b.lo); });
                Rational end{1, 1};
                for (const Interval& interval : good) {
                    if (less(end, interval.lo)) break;
                    if (!interval.hi.den) {
                        covered = true;
                        break;
                    }
                    if (less(end, interval.hi)) end = interval.hi;
                }
            }
            if (!covered) ++failures;
        }
        return {ideals, failures};
    }
};

int main() {
    for (int n = 2; n <= 7; ++n) {
        std::int64_t orders = 0, connected = 0, ideals = 0, failures = 0;
        for (unsigned edges = 0; edges < (1u << (n * (n - 1) / 2)); ++edges) {
            OrderData data{n, {}, {}};
            unsigned remaining = edges;
            for (int i = 1; i < n; ++i) {
                data.pred[i] = remaining & ((1u << i) - 1);
                remaining >>= i;
            }
            if (!data.closed()) continue;
            ++orders;
            if (!data.connected()) continue;
            ++connected;
            auto [checked, bad] = data.check();
            ideals += checked;
            failures += bad;
        }
        std::cout << "{\"order\":" << n << ",\"naturally_labelled_orders\":" << orders
                  << ",\"connected\":" << connected << ",\"boundary_ideals\":" << ideals
                  << ",\"uncovered\":" << failures << "}" << std::endl;
    }
}
