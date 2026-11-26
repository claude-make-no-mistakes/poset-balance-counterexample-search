# One-third–two-thirds counterexample search

Deleting one point from a hypothetical counterexample must leave only singleton components and components of order at least seven in the incomparability graph, with at least one of the latter. The proof for components of orders two and three is elementary. The extension through order six uses exact rational intervals for the admissible insertion weights, so the ambient order is unrestricted. [Paper](paper.pdf) · [Source](paper.tex) · [Recent general bounds](https://arxiv.org/abs/2609.30888v2)

We also tried to falsify the conjecture by inserting points into order-14 low-balance posets. The tail uses one published seed per reported balance value. The equality cohort uses the archive's stored representatives. The counts are labelled insertion histories and may include isomorphic copies.

| Insertions | Seed cohort | Histories generated | Connected histories checked | Lowest connected balance |
| --- | --- | ---: | ---: | ---: |
| One | 42 tail representatives | 14,335 | 6,943 | 889/2483 |
| One | 68 equality representatives | 13,036 | 372 | 12/29 |
| Two | 42 tail representatives | 6,899,699 | 4,281,043 | 269/767 |
| Two | 68 equality representatives | 3,563,408 | 336,004 | 63/160 |

The one-insertion sweep also checked the disconnected histories. No checked history violates the conjecture. The order-16 cases outside these seed neighbourhoods remain outside the computation. The Python evaluator was independently checked by an enumeration using a different insertion generator and pair-counting recurrence.

## Replay

Use Python 3.10 or newer. The two-insertion replay also uses GCC with C++17 support. Compare the generated JSON with the files in [evidence](evidence/). The native driver sends progress to standard error.

```sh
python sweep.py upstream/census-n14.txt sweep.json
python boundary.py 6 boundary.json
g++ -O3 -std=c++17 double.cpp -o double
python double_seeds.py upstream/census-n14.txt --group tail | ./double > double-tail.json
python double_seeds.py upstream/census-n14.txt --group equality | ./double > double-equality.json
python balance.py evidence/tail-16-minimizer.json
python balance.py evidence/equality-16-minimizer.json
```

The seed data comes from Gupta's census, pinned at commit `dd1d44610be8cc2576cfeac333e45267ff349cda`. The file has SHA-256 `bd03553ed50c9875800760dd26623c6e9c1a202271cb79ec03f3a7404fffef7c`. The included license accompanies the upstream data. [Pinned data](https://github.com/agupta/gold-partition-conjecture/blob/dd1d44610be8cc2576cfeac333e45267ff349cda/data/census-n14.txt) · [Census paper](https://arxiv.org/abs/2607.23926v2)
