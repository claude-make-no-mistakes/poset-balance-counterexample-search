"""Check balanced-pair coverage for every real boundary weight r >= 1."""

import argparse
import json
from fractions import Fraction
from pathlib import Path

from balance import bits
from sweep import connected_incomparability, ideals


def ordered_posets(n):
    # Every finite poset can be relabelled in the order of one linear extension.
    if n == 0:
        yield []
        return
    for old in ordered_posets(n - 1):
        for pred in ideals(old):
            yield old + [pred]


def linear_extensions(pred):
    full = (1 << len(pred)) - 1
    prefix = []

    def visit(mask):
        if mask == full:
            yield tuple(prefix)
            return
        for vertex in bits(full ^ mask):
            if pred[vertex] & ~mask:
                continue
            prefix.append(vertex)
            yield from visit(mask | (1 << vertex))
            prefix.pop()

    return list(visit(0))


def interval(coefficients):
    low, high = Fraction(1), None
    for slope, intercept in coefficients:
        if slope == 0:
            if intercept < 0:
                return None
            continue
        crossing = Fraction(-intercept, slope)
        if slope > 0:
            low = max(low, crossing)
        elif high is None or crossing < high:
            high = crossing
    return (low, high) if high is None or low <= high else None


def covers(intervals):
    edge = Fraction(1)
    for low, high in sorted(intervals, key=lambda pair: pair[0]):
        if low > edge:
            return False
        if high is None:
            return True
        edge = max(edge, high)
    return False


def inspect(pred):
    n = len(pred)
    pairs = [(u, v) for v in range(n) for u in range(v) if not pred[v] & (1 << u)]
    orders = linear_extensions(pred)
    positions = [[order.index(v) for v in range(n)] for order in orders]
    events = [tuple(k for k, ranks in enumerate(positions) if ranks[u] < ranks[v])
              for u, v in pairs]
    failures = []
    for lower in ideals(pred):
        predecessors = tuple(bits(lower))
        offsets = [n - max((ranks[v] + 1 for v in predecessors), default=0)
                   for ranks in positions]
        h = sum(offsets)
        length = len(orders)
        acceptable = []
        for event in events:
            c = len(event)
            d = sum(offsets[k] for k in event)
            coefficients = ((3 * c - length, 3 * d - h),
                            (2 * length - 3 * c, 2 * h - 3 * d))
            if all(slope >= 0 and slope + intercept >= 0
                   for slope, intercept in coefficients):
                acceptable = [(Fraction(1), None)]
                break
            candidate = interval(coefficients)
            if candidate is not None:
                acceptable.append(candidate)
        if not covers(acceptable):
            failures.append(lower)
    return len(ideals(pred)), failures


def check(n):
    all_orders = checked = ideals_checked = 0
    failures = []
    for pred in ordered_posets(n):
        all_orders += 1
        if not connected_incomparability(pred):
            continue
        checked += 1
        total, uncovered = inspect(pred)
        ideals_checked += total
        failures.extend({"pred": pred, "lower": lower} for lower in uncovered)
    return {"order": n, "naturally_labelled_orders": all_orders, "connected": checked,
            "boundary_ideals": ideals_checked, "uncovered": len(failures),
            "examples": failures[:5]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("maximum", type=int)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if not 2 <= args.maximum <= 7:
        parser.error("maximum must be between two and seven")
    results = []
    for n in range(2, args.maximum + 1):
        result = check(n)
        results.append(result)
        print(json.dumps(result), flush=True)
    args.output.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
