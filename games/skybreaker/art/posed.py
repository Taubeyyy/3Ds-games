"""Figuren in Pose zusammensetzen (für Vorschau/Konzeptblätter). Gleiche Platzierung wie im Spiel (source/pose.hpp)."""
import math
from characters import *

# Waffen-Halterung je Figur: (Position der Hand, Drehungen)
HOLD = {
    "seren": ((0.275, 0.81, 0.11), [("x", PI * 0.78), ("z", -0.12)]),
}


def posed(name, parts):
    out = []
    for k, m in parts.items():
        if k.startswith("weapon"):
            pos, rots = HOLD.get(name, ((0, 0, 0), []))  # Götter: Waffe schon im Modell platziert
            for ax, a in rots:
                m = m.rx(a) if ax == "x" else m.ry(a) if ax == "y" else m.rz(a)
            m = m.move(*pos)
        out.append((m, k in ("glow", "weapon_glow", "orbit_glow", "fx", "pet")))
    return out
