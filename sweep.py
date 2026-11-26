"""Exhaustively insert one vertex into specified, labelled seed posets."""

import argparse
import hashlib
import json
from fractions import Fraction
from itertools import product
from pathlib import Path

from balance import bits, validate, value


def successors(pred):
    out = [0] * len(pred)
    for v, mask in enumerate(pred):
        for u in bits(mask):
            out[u] |= 1 << v
    return out


def ideals(pred):
    masks = [0]
    seen = {0}
    full = (1 << len(pred)) - 1
    for mask in masks:
        for v in bits(full ^ mask):
            if not pred[v] & ~mask:
                ext = mask | 1 << v
                if ext not in seen:
                    masks.append(ext)
                    seen.add(ext)
    return masks


def extensions(pred):
    validate(pred)
    n = len(pred)
    full = (1 << n) - 1
    above = successors(pred)
    filters = ideals(above)
    for lower in ideals(pred):
        # A successor of the inserted vertex must be above every predecessor.
        eligible = full
        for v in bits(lower):
            eligible &= above[v]
        for upper in filters:
            if upper & ~eligible or upper & lower:
                continue
            yield [mask | ((1 << n) if (upper >> i) & 1 else 0)
                   for i, mask in enumerate(pred)] + [lower]


def connected_incomparability(pred):
    n = len(pred)
    if n < 2:
        return True
    full = (1 << n) - 1
    above = successors(pred)
    seen = 1
    frontier = 1
    while frontier:
        reach = 0
        for v in bits(frontier):
            reach |= full ^ (pred[v] | above[v] | (1 << v))
        frontier = reach & ~seen
        seen |= frontier
    return seen == full


def width_at_least_four(pred):
    # Check the exact order-15 class from the slate.
    above = successors(pred)
    full = (1 << len(pred)) - 1
    noncomparable = [full ^ (down | up | 1 << v)
                     for v, (down, up) in enumerate(zip(pred, above))]
    for i, mask in enumerate(noncomparable):
        mask &= ~((1 << (i + 1)) - 1)
        for j in bits(mask):
            shared = mask & noncomparable[j]
            for k in bits(shared):
                if shared & noncomparable[k]:
                    return True
    return False


def load_census(path):
    source = path.read_bytes()
    text = source.decode("utf-8")
    start = text.index("Low balance tail")
    middle = text.index("Equality cases", start)
    seeds = []
    for line in text[start:middle].splitlines():
        fields = line.split()
        if len(fields) != 6 or not fields[-1].startswith("14:"):
            continue
        word = fields[-1][3:]
        pred = [int(word[i:i + 4], 16) for i in range(0, len(word), 4)]
        seeds.append(("tail", Fraction(fields[0]), pred))
    for line in text[middle:].splitlines():
        fields = line.split()
        if not fields or fields[0] != "count" or not fields[-1].startswith("14:"):
            continue
        word = fields[-1][3:]
        pred = [int(word[i:i + 4], 16) for i in range(0, len(word), 4)]
        seeds.append(("equality", Fraction(1, 3), pred))
    if sum(tag == "tail" for tag, _, _ in seeds) != 42 or len(seeds) != 110:
        raise ValueError("unexpected census layout or seed count")
    return hashlib.sha256(source).hexdigest(), seeds


def ordinal_sum(blocks):
    result = []
    for block in blocks:
        start = len(result)
        result.extend(((1 << start) - 1) | (mask << start) for mask in block)
    return result


def summarize(seeds, *, connected_only=False):
    cases = nonchains = connected = wide = 0
    minimum = None
    minimum_connected = None
    minimum_wide = None
    counterexamples = []
    for pred in seeds:
        for candidate in extensions(pred):
            cases += 1
            indecomposable = connected_incomparability(candidate)
            if indecomposable:
                connected += 1
            elif connected_only:
                continue
            delta, total, pair = value(candidate)
            if pair is None:
                continue
            nonchains += 1
            new = {"pred": candidate, "balance": str(delta), "extensions": total, "pair": pair}
            if minimum is None or delta < Fraction(minimum["balance"]):
                minimum = new
            if indecomposable and (minimum_connected is None
                                   or delta < Fraction(minimum_connected["balance"])):
                minimum_connected = new
            if indecomposable and width_at_least_four(candidate):
                wide += 1
                if minimum_wide is None or delta < Fraction(minimum_wide["balance"]):
                    minimum_wide = new
            if delta < Fraction(1, 3):
                counterexamples.append(new)
    return {"cases": cases, "nonchains_evaluated": nonchains, "connected": connected,
            "connected_width_ge_four": wide, "minimum": minimum,
            "minimum_connected": minimum_connected, "minimum_wide": minimum_wide,
            "counterexamples": counterexamples}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("census", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source, seeds = load_census(args.census)
    for _, expected, pred in seeds:
        if len(pred) != 14 or value(pred)[0] != expected:
            raise ValueError("source seed mismatch")
    report = {"census_sha256": source}
    for tag in ["tail", "equality"]:
        cohort = [pred for group, _, pred in seeds if group == tag]
        report[tag] = {"seeds": len(cohort), **summarize(cohort)}
    # End blocks of size one or (a<b) plus an isolated element. The proof
    # reduces all unbounded cases to these two end blocks and a central chain.
    blocks = [[0], [0, 1, 0]]
    finite = blocks + [ordinal_sum([left, *([[0]] * k), right])
                       for left, right, k in product(blocks, blocks, range(11))]
    report["end_blocks"] = {"seeds": len(finite), **summarize(finite, connected_only=True)}
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
