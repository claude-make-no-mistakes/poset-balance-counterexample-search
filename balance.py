"""Count linear extensions and all pair precedences with integer arithmetic."""

import argparse
import json
from fractions import Fraction
from functools import cache
from pathlib import Path


def bits(mask):
    while mask:
        bit = mask & -mask
        yield bit.bit_length() - 1
        mask ^= bit


def validate(pred):
    n = len(pred)
    for vertex, mask in enumerate(pred):
        if not 0 <= mask < 1 << n or mask & (1 << vertex):
            raise ValueError("predecessor set outside the strict order domain")
        if any(pred[u] & ~mask for u in bits(mask)):
            raise ValueError("predecessor sets must be transitively closed")


def count(pred):
    validate(pred)
    n = len(pred)
    full = (1 << n) - 1

    @cache
    def next_vertices(mask):
        return tuple(v for v in bits(full ^ mask) if not pred[v] & ~mask)

    @cache
    def suffix(mask):
        if mask == full:
            return 1
        return sum(suffix(mask | (1 << v)) for v in next_vertices(mask))

    totals = [[0] * n for _ in range(n)]
    layer = {0: 1}
    for _ in range(n):
        following = {}
        for mask, prefix in layer.items():
            placed = tuple(bits(mask))
            for v in next_vertices(mask):
                next_mask = mask | (1 << v)
                following[next_mask] = following.get(next_mask, 0) + prefix
                through = prefix * suffix(next_mask)
                for u in placed:
                    totals[u][v] += through
        layer = following
    total = suffix(0)
    if layer != {full: total}:
        raise RuntimeError("forward and backward extension counts disagree")
    return total, totals


def value(pred):
    total, tallies = count(pred)
    best = 0
    pair = None
    for i in range(len(pred)):
        for j in range(i + 1, len(pred)):
            before = tallies[i][j]
            after = tallies[j][i]
            if before + after != total:
                raise RuntimeError("pair counts do not partition the extensions")
            smaller = min(before, after)
            if smaller > best:
                best, pair = smaller, (i, j)
    return Fraction(best, total), total, pair


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("poset", type=Path)
    args = parser.parse_args()
    pred = json.loads(args.poset.read_text(encoding="utf-8"))["pred"]
    delta, total, pair = value(pred)
    print(json.dumps({"order": len(pred), "extensions": total, "balance": str(delta),
                      "pair": pair, "counterexample": pair is not None
                      and delta < Fraction(1, 3)}))


if __name__ == "__main__":
    main()
