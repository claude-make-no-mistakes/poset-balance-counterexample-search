"""Write the published low-tail seeds in the native census input format."""

import argparse
from pathlib import Path

from sweep import load_census


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("census", type=Path)
    parser.add_argument("--limit", type=int)
    parser.add_argument("--group", choices=("tail", "equality"), default="tail")
    args = parser.parse_args()
    _, seeds = load_census(args.census)
    cohort = [pred for group, _, pred in seeds if group == args.group]
    for pred in cohort[:args.limit]:
        print(*pred)


if __name__ == "__main__":
    main()
