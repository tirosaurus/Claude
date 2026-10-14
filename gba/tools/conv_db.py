"""data/db.json (exportado de DB.gd con tools/export_db.gd) -> build/gen/db.c"""
import json
import os
from lib import *

STATS = ["hp", "mp", "atk", "def", "mag", "res", "spd"]
TARGETS = ["enemy", "enemies", "ally", "allies", "self", "ally_dead", "enemies_random"]
KINDS = ["phys", "mag", "heal", "buff", "status", "revive", "steal", "cure", "drain", "summon", "eat"]
ELS = ["", "fire", "ice", "bolt", "light", "dark", "nature", "phys"]
SLOTS = ["weapon", "shield", "head", "body", "ring"]
WKINDS = ["", "sword", "axe", "daggers", "staff", "bow"]
WEIGHTS = ["", "light", "medium", "heavy", "cloth"]
PERKS = ["", "crit", "heal_pow", "mp_regen", "dmg_phys", "dmg_mag", "dmg_red", "lifesteal", "atb_start"]


def f(x):
    s = "%g" % float(x)
    return (s if ("." in s or "e" in s) else s + ".0") + "f"


def stats7(d, cast=int):
    return "{%s}" % ", ".join(str(cast(d.get(k, 0))) if cast is int else f(d.get(k, 0)) for k in STATS)


def mask(lst, table):
    m = 0
    for x in lst:
        m |= 1 << table.index(x)
    return m


def main():
    d = json.load(open(os.path.join(GBA, "data", "db.json")))
    status_ids = list(d["STATUS"].keys())
    skill_ids = list(d["SKILLS"].keys())
    item_ids = list(d["ITEMS"].keys())
    equip_ids = list(d["EQUIP"].keys())
    rar_ids = list(d["RARITY"].keys())
    class_ids = list(d["CLASSES"].keys())
    branch_ids = list(d["BRANCHES"].keys())
    race_ids = list(d["RACES"].keys())
    member_ids = list(d["MEMBERS"].keys())
    enemy_ids = list(d["ENEMIES"].keys())
    c = ['#include "../../src/db.h"', ""]
    h = []

    def sk(x):
        return skill_ids.index(x) if x else -1

    # estados
    c.append("const StatusDef g_status[] = {%s};" % ", ".join(
        "{%s, %s, %s}" % (c_string(k), c_string(v["name"]), c_string(v["icon"])) for k, v in d["STATUS"].items()))
    for i, k in enumerate(status_ids):
        h.append("#define S_%s %d" % (k.upper(), i))
    h.append("#define S_COUNT %d" % len(status_ids))

    def slist(lst):
        return "{%s}, %d" % (", ".join(str(status_ids.index(s)) for s in lst) or "0", len(lst))

    # habilidades
    rows = []
    for k, s in d["SKILLS"].items():
        rows.append("{%s, %s, %s, %d, %d, %d, %d, %d, %s, %s, %s, %s, %s, %s, %s, %d, %s, %s, %s, %d, %d, %d, %d}" % (
            c_string(k), c_string(s["name"]), c_string(s.get("desc", "")), int(s.get("mp", 0)),
            TARGETS.index(s["target"]), KINDS.index(s["kind"]), ELS.index(s.get("el", "")), int(s.get("hits", 1)),
            f(s.get("power", 1.0)), f(s.get("base", 0)), f(s.get("crit", 0)), f(s.get("chance", 0.5)), f(s.get("ratio", 0)),
            f(s.get("heal_party", 0)), f(s.get("vs_poison", 0)),
            status_ids.index(s["inflict"]) if "inflict" in s else -1,
            slist(s.get("status", [])), slist(s.get("self_status", [])), slist(s.get("add_status", [])),
            int(s.get("turns", 3)), 1 if s.get("cure") else 0, 1 if "heal_party" in s else 0, 1 if "vs_poison" in s else 0))
    c.append("const SkillDef g_skills[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_SKILLS %d" % len(skill_ids))
    # objetos
    rows = []
    for k, it in d["ITEMS"].items():
        rows.append("{%s, %s, %s, %s, %d, %d, %d, %d, %d, %s, %d, %d, %d, %s, %d, %d}" % (
            c_string(k), c_string(it["name"]), c_string(it["desc"]), c_string(it["icon"]), int(it["price"]),
            TARGETS.index(it["target"]), int(it.get("heal", 0)), int(it.get("mp", 0)), 1 if it.get("cure") else 0,
            f(it.get("revive", 0)), 1 if "revive" in it else 0, int(it.get("damage", 0)), ELS.index(it.get("el", "")),
            c_string(it.get("special")), 1 if it.get("key") else 0, 1 if it.get("nap") else 0))
    c.append("const ItemDef g_items[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_ITEMS %d" % len(item_ids))
    # rarezas
    c.append("const RarityDef g_rarity[] = {%s};" % ", ".join(
        "{%s, %s, {%d, %d, %d}}" % (c_string(k), c_string(v["name"]), *v["color"]) for k, v in d["RARITY"].items()))
    for i, k in enumerate(rar_ids):
        h.append("#define R_%s %d" % (k.upper(), i))
    # equipo
    rows = []
    for k, e in d["EQUIP"].items():
        st = e["stats"]
        order = [STATS.index(s) for s in st.keys()]
        rows.append("{%s, %d, %d, %s, %d, %d, %d, %d, %s, %d, {%s}, %d, %s, %d, %s, %s}" % (
            c_string(k), rar_ids.index(e.get("rarity", "basic")), int(e.get("lvl", 1)), c_string(e["name"]),
            SLOTS.index(e["slot"]), WKINDS.index(e.get("kind", "")), int(e.get("tier", 1)), int(e["price"]),
            stats7(st), mask(list(st.keys()), STATS), ", ".join(map(str, order)) or "0", len(order),
            c_string(e.get("look")), WEIGHTS.index(e.get("weight", "")), c_string(e.get("orb")), c_string(e.get("lore"))))
    c.append("const EquipDef g_equip[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_EQUIP %d" % len(equip_ids))
    # razas
    c.append("const RaceDef g_races[] = {%s};" % ",\n\t".join(
        "{%s, %s, %s, %s, %s, %s}" % (c_string(k), c_string(r["name"]), c_string(r["desc"]), c_string(r["trait"]),
                                    stats7(r["base"]), stats7(r["grow"], float)) for k, r in d["RACES"].items()))
    h.append("#define N_RACES %d" % len(race_ids))
    # sendas
    c.append("const BranchDef g_branches[] = {%s};" % ",\n\t".join(
        "{%s, %s, %s, %s, %s, {%s}}" % (c_string(k), c_string(b["name"]), c_string(b["desc"]), c_string(b["icon"]),
                                      stats7(b["bonus"]), ", ".join(str(class_ids.index(x)) for x in b["classes"]))
        for k, b in d["BRANCHES"].items()))
    h.append("#define N_BRANCHES %d" % len(branch_ids))
    # clases
    rows = []
    for k, cl in d["CLASSES"].items():
        nodes = []
        for n in cl["tree"]:
            perk = n.get("perk", ["", 0])
            nodes.append("{%s, %d, %d, %d, %s, %d, %d, %s, %s, %s}" % (
                c_string(n["id"]), int(n["tier"]), int(n["max"]), sk(n.get("skill")), stats7(n.get("stats", {})),
                1 if "stats" in n else 0, PERKS.index(perk[0]), f(perk[1]), c_string(n["name"]), c_string(n.get("desc", ""))))
        rows.append("{%s, %s, %s, %s, %d, %s, %s, %d, {%s}}" % (
            c_string(k), c_string(cl["name"]), c_string(cl["desc"]), c_string(cl["icon"]), WKINDS.index(cl["weapon"]),
            c_string(cl.get("orb")), stats7(cl["bonus"]), sk(cl["starter"]), ",\n\t\t".join(nodes)))
    c.append("const ClassDef g_classes[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_CLASSES %d" % len(class_ids))
    # miembros
    rows = []
    for k, m in d["MEMBERS"].items():
        def tab(t):
            ks = sorted(t.keys(), key=int)
            return "{%s}, {%s}, %d" % (", ".join(ks) or "0", ", ".join(str(sk(t[x])) for x in ks) or "0", len(ks))
        rows.append("{%s, %s, %s, %s, %s, %s, %s}" % (
            c_string(k), c_string(m["name"]), c_string(m["role"]), stats7(m["base"]), stats7(m["grow"], float),
            tab(m["skills"]), tab(m.get("dark_skills", {}))))
    c.append("const MemberDef g_members[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_MEMBERS %d" % len(member_ids))
    # enemigos
    rows = []
    for k, e in d["ENEMIES"].items():
        ai = ", ".join("{%d, %d}" % (sk(a[0]), int(a[1])) for a in e["ai"])
        drops = ", ".join("{%s, %s}" % (c_string(x[0]), f(x[1])) for x in e.get("drops", []))
        tint = e.get("tint")
        rows.append("{%s, %s, %d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, {%s}, %d, %d, {%s}, %d, {%s}, %d, %s, %s}" % (
            c_string(k), c_string(e["name"]), int(e["frames"]), f(e["scale"]), int(e["hp"]), int(e["atk"]), int(e["def"]),
            int(e["mag"]), int(e["res"]), int(e["spd"]), int(e["xp"]), int(e["gold"]),
            mask(e.get("weak", []), ELS), mask(e.get("resist", []), ELS), ai, len(e["ai"]), 1 if e.get("boss") else 0,
            drops or '{0, 0}', len(e.get("drops", [])), ", ".join(f(x) for x in (tint or [1, 1, 1])), 1 if tint else 0,
            c_string(e.get("summon")), c_string(e.get("summon_msg"))))
    c.append("const EnemyDef g_enemies[] = {\n\t%s\n};" % ",\n\t".join(rows))
    h.append("#define N_ENEMIES %d" % len(enemy_ids))
    # reglas de equipo
    c.append("const EquipRule g_equip_rules[] = {%s};" % ",\n\t".join(
        "{%s, %d, %d, %d}" % (c_string(k), mask(r["weapons"], WKINDS), 1 if r["shield"] else 0, mask(r["weights"], WEIGHTS))
        for k, r in d["EQUIP_RULES"].items()))
    c.append("const int g_nequip_rules = %d;" % len(d["EQUIP_RULES"]))

    def gear(tab, name):
        lines = []
        for k, lst in tab.items():
            lines.append("{%s, {%s}, %d}" % (c_string(k), ", ".join(c_string(x) for x in lst), len(lst)))
        c.append("const GearList %s[] = {%s};" % (name, ",\n\t".join(lines)))
        c.append("const int %s_n = %d;" % (name, len(tab)))

    gear(d["CLASS_STARTER_GEAR"], "g_class_gear")
    gear(d["MEMBER_START_GEAR"], "g_member_gear")
    # tiendas
    lines = []
    for k, s in d["SHOPS"].items():
        c.append("static const char* const shop_%s[] = {%s};" % (k, ", ".join(c_string(x) for x in s["stock"])))
        lines.append("{%s, %s, shop_%s, %d}" % (c_string(k), c_string(s["name"]), k, len(s["stock"])))
    c.append("const ShopDef g_shops[] = {%s};\nconst int g_nshops = %d;" % (", ".join(lines), len(lines)))
    # cofres
    lines = []
    for k, s in d["CHESTS"].items():
        lines.append("{%s, {%s}, %d}" % (c_string(k), ", ".join("{%s, %d}" % (c_string(a), int(b)) for a, b in s.items()), len(s)))
    c.append("const ChestDef g_chests[] = {%s};\nconst int g_nchests = %d;" % (",\n\t".join(lines), len(lines)))
    # mejoras
    c.append("const UpgradeDef g_upgrades[] = {%s};\nconst int g_nupgrades = %d;" % (", ".join(
        "{%s, %s, %s, %d, %d, %d}" % (c_string(k), c_string(u["name"]), c_string(u["desc"]), int(u["price"]),
                                    1 if u["kind"] == "weapon" else 0, int(u["tier"])) for k, u in d["UPGRADES"].items()),
        len(d["UPGRADES"])))
    c.append("const char* const g_legendaries[] = {%s};\nconst int g_nlegendaries = %d;" % (
        ", ".join(c_string(x) for x in d["LEGENDARIES"]), len(d["LEGENDARIES"])))
    c.append("const char* const g_vael_set[] = {%s};" % ", ".join(c_string(x) for x in d["VAEL_SET"]))
    c.append("const char* const g_slot_names[] = {%s};" % ", ".join(c_string(d["SLOT_NAMES"][s]) for s in SLOTS))
    c.append("const char* const g_stat_names[] = {%s};" % ", ".join(c_string(d["STAT_NAMES"][s]) for s in STATS))
    # aspecto
    for key in ("HAIR_COLORS", "SKIN_TONES", "OUTFITS"):
        c.append("const ColorOpt g_%s[] = {%s};" % (key.lower(), ", ".join(
            "{%s, {%d, %d, %d}}" % (c_string(n), *col) for n, col in d[key])))
        h.append("#define N_%s %d" % (key, len(d[key])))
    c.append("const NameOpt g_hair_styles[] = {%s};" % ", ".join("{%s, %s}" % (c_string(a), c_string(b)) for a, b in d["HAIR_STYLES"]))
    h.append("#define N_HAIR_STYLES %d" % len(d["HAIR_STYLES"]))
    h.append("#define MAX_LEVEL %d" % int(d["MAX_LEVEL"]))
    write_if_changed(os.path.join(OUT, "db.c"), "\n".join(c) + "\n")
    write_if_changed(os.path.join(OUT, "db_ids.h"), "#pragma once\n" + "\n".join(h) + "\n")


if __name__ == "__main__":
    main()
