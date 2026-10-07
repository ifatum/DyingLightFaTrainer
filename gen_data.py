#!/usr/bin/env python3
import math
import os
import re
import struct
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
STEAM = os.path.expanduser("~/.local/share/Steam")
STATS = ["Damage", "Condition", "CriticalProb", "CriticalDamage", "Force", "StaminaUsage", "DamageRange",
         "UpgradeLevel", "AllowedRepairs", "MaxStackCount", "Price", "Color", "AmmoCount", "ReloadTime", "DepletionTime"]
COLORS = {"Color_White": 0, "Color_Green": 1, "Color_Blue": 2, "Color_Violet": 3, "Color_Orange": 4, "Color_Platinum": 5}


def game_dw():
    vdf = os.path.join(STEAM, "steamapps/libraryfolders.vdf")
    for lib in re.findall(r'"path"\s+"([^"]+)"', open(vdf).read()):
        p = os.path.join(lib, "steamapps/common/Dying Light/DW")
        if os.path.exists(os.path.join(p, "Data0.pak")):
            return p
    sys.exit("Dying Light install not found")


def read_texts(t):
    out, p = {}, 8
    for _ in range(struct.unpack_from("<I", t, 4)[0]):
        n = struct.unpack_from("<H", t, p)[0]
        key = t[p + 2:p + 2 + n].decode("latin1")
        p += 2 + n
        n = struct.unpack_from("<H", t, p)[0]
        text = t[p + 2:p + 2 + 2 * n].decode("utf-16-le", "replace")
        p += 2 + 2 * n
        if key.endswith("_N"):
            out[key[:-2]] = text
    return out


def number(arg):
    arg = arg.strip()
    if arg in COLORS:
        return float(COLORS[arg])
    if re.fullmatch(r"-?[\d.]+(\s*[*/+-]\s*-?[\d.]+)*", arg):
        try:
            return float(eval(arg, {"__builtins__": {}}))
        except (SyntaxError, ZeroDivisionError):
            return None
    return None


def item_stats(z):
    stats, cur = {}, None
    for n in sorted(z.namelist()):
        if not (n.startswith("data/scripts/inventory/") and n.endswith(".scr")):
            continue
        for line in z.read(n).decode("latin1").splitlines():
            m = re.match(r'\s*Item\(\s*"([^"]+)"', line)
            if m:
                cur = stats.setdefault(m.group(1), {})
                continue
            m = re.match(r"\s*(\w+)\((.*)\);", line)
            if m and cur is not None and m.group(1) in STATS and m.group(1) not in cur:
                v = number(m.group(2))
                if v is not None:
                    cur[m.group(1)] = v
    return stats


def c_str(s):
    out = []
    for ch in s.encode("utf-8"):
        c = chr(ch)
        out.append(c if 32 <= ch < 127 and c not in '"\\?' else f"\\{ch:03o}")
    return '"' + "".join(out) + '"'


def c_float(v):
    return "NAN" if v is None or math.isnan(v) else repr(float(v)) + "f"


def main():
    dw = game_dw()
    with zipfile.ZipFile(os.path.join(dw, "Data0.pak")) as z:
        ids = sorted(set(re.findall(r'Item\(\d+,\s*"([^"]+)"\)', z.read("data/scripts/inventory/validitemids.scr").decode("latin1"))),
                     key=lambda s: s.encode())
        stats = item_stats(z)
    with zipfile.ZipFile(os.path.join(dw, "DataEn.pak")) as z:
        names = read_texts(z.read("data/maps/common_texts_all.bin"))
    rows = []
    for i in ids:
        st = stats.get(i, {})
        vals = ", ".join(c_float(st.get(k)) for k in STATS)
        rows.append(f"  {{{c_str(i)}, {c_str(names.get(i, ''))}, {{{vals}}}}}")
    enum = ", ".join(f"ST_{k}" for k in STATS)
    labels = ", ".join(c_str(k) for k in STATS)
    with open(os.path.join(HERE, "src/items.h"), "w") as f:
        f.write("#pragma once\n#include <cmath>\n"
                f"enum Stat {{ {enum}, ST_COUNT }};\n"
                f"static const char* STAT_KEYS[ST_COUNT] = {{{labels}}};\n"
                "struct ItemInfo { const char* id; const char* name; float st[ST_COUNT]; };\n"
                "static const ItemInfo ITEMS[] = {\n" + ",\n".join(rows) + "\n};\n"
                f"static const int ITEM_COUNT = {len(ids)};\n")
    have = sum(1 for i in ids if stats.get(i))
    print(f"items.h: {len(ids)} items, {sum(1 for i in ids if names.get(i))} named, {have} with stats")


if __name__ == "__main__":
    main()
