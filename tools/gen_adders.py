#!/usr/bin/env python3
"""Generate 8-bit structural adder netlists for the ADS project.

Usage: python3 tools/gen_adders.py [output-directory]
Only gates supported by the current parser are emitted.
"""
from pathlib import Path
import sys

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("tests")
OUT.mkdir(parents=True, exist_ok=True)

def emit_ripple():
    lines = ["# 8-bit ripple-carry adder", "INPUT cin " + " ".join(f"a{i} b{i}" for i in range(8)),
             "OUTPUT " + " ".join([f"s{i}" for i in range(8)] + ["c8"])]
    for i in range(8):
        lines += [f"XOR p{i} 1 a{i} b{i}",
                  f"XOR s{i} 1 p{i} c{i}" if i else f"XOR s{i} 1 p0 cin"]
        # c0 is the external carry-in; internal carry c{i+1} is generated below.
        if i == 0:
            lines += ["AND g0 1 a0 b0", "AND h0 1 p0 cin", "OR c1 1 g0 h0"]
        else:
            lines += [f"AND g{i} 1 a{i} b{i}", f"AND h{i} 1 p{i} c{i}",
                      f"OR c{i+1} 1 g{i} h{i}"]
    # Replace carry-in reference for bit zero and provide carry chain aliases.
    # The first sum expression above is already correct.
    return "\n".join(lines) + "\n"

def emit_cla():
    lines = ["# 8-bit carry-lookahead adder, two 4-bit lookahead groups",
             "INPUT cin " + " ".join(f"a{i} b{i}" for i in range(8)),
             "OUTPUT " + " ".join([f"s{i}" for i in range(8)] + ["c8"])]

    def tree(op, items, prefix):
        """Build a balanced binary AND/OR tree; return the resulting net."""
        level = list(items)
        depth = 0
        while len(level) > 1:
            nxt = []
            for j in range(0, len(level), 2):
                if j + 1 == len(level):
                    nxt.append(level[j])
                else:
                    name = f"{prefix}_{depth}_{j//2}"
                    lines.append(f"{op} {name} 1 {level[j]} {level[j+1]}")
                    nxt.append(name)
            level = nxt
            depth += 1
        return level[0]

    for i in range(8):
        lines += [f"XOR p{i} 1 a{i} b{i}", f"AND g{i} 1 a{i} b{i}"]

    # Each four-bit group computes carries using expanded lookahead equations.
    # First group has cin as its incoming carry.
    for i in range(1, 5):
        products = []
        for j in range(i - 1, -1, -1):
            factors = [f"g{j}"] + [f"p{k}" for k in range(j + 1, i)]
            products.append(tree("AND", factors, f"cla{i}_g{j}"))
        products.append(tree("AND", ["cin"] + [f"p{k}" for k in range(i)], f"cla{i}_cin"))
        lines.append(f"OR c{i} 1 {products[0]} {products[1]}" if len(products) == 2
                     else f"OR c{i}_or0 1 {products[0]} {products[1]}")
        if len(products) > 2:
            acc = "c" + str(i) + "_or0"
            for j, term in enumerate(products[2:-1], start=1):
                name = f"c{i}_or{j}"
                lines.append(f"OR {name} 1 {acc} {term}")
                acc = name
            name = f"c{i}"
            lines.append(f"OR {name} 1 {acc} {products[-1]}")
        elif len(products) == 2:
            # The two-term case above already created c{i}.
            pass

    # Second group is identical, but its incoming carry is c4.
    for i in range(5, 9):
        products = []
        for j in range(i - 1, 3, -1):
            factors = [f"g{j}"] + [f"p{k}" for k in range(j + 1, i)]
            products.append(tree("AND", factors, f"cla{i}_g{j}"))
        products.append(tree("AND", ["c4"] + [f"p{k}" for k in range(4, i)], f"cla{i}_cin"))
        acc = products[0]
        for j, term in enumerate(products[1:], start=0):
            name = f"c{i}" if j == len(products) - 2 else f"c{i}_or{j}"
            lines.append(f"OR {name} 1 {acc} {term}")
            acc = name

    for i in range(8):
        carry = "cin" if i == 0 else f"c{i}"
        lines.append(f"XOR s{i} 1 p{i} {carry}")
    return "\n".join(lines) + "\n"

(OUT / "ripple8.net").write_text(emit_ripple())
(OUT / "cla8.net").write_text(emit_cla())
print(f"Generated {OUT / 'ripple8.net'} and {OUT / 'cla8.net'}")
