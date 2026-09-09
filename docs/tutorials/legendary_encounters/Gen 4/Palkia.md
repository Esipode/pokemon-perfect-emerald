## Palkia — The Spatial Pokémon

Palkia fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Palkia decides how far away it is standing.** Every other boss asks *what* you attack with.
> Palkia asks *how* — because the only thing that reaches it is an attack shaped like the distance
> it has chosen. Send the wrong shape and the blow falls into the torn space and comes back out
> behind your own Pokémon.

This is a fight about **move category**. Physical, special, or neither.

---

### The zone wheel

Palkia announces a zone, holds it for a few turns, then shifts to the next one. The wheel is
**fixed**, so once you have seen it turn twice you can plan two shifts ahead.

```
   COMPRESSED  ->  SEPARATED  ->  WARPED  ->  back to COMPRESSED
   (Phase 0 skips WARPED: the two zones simply alternate)
```

| Zone | What reaches Palkia | What else is true | Guard |
| --- | --- | --- | --- |
| **COMPRESSED** — *"Palkia is suddenly close enough to touch"* | **Physical** | Fairy Lock: you cannot switch or flee | **−6** |
| **SEPARATED** — *"Palkia pulls the distance out long"* | **Special** | Trapping released — and a switch here is the one switch that costs you nothing | **+2** |
| **WARPED** — *"Palkia is standing in a direction you cannot aim at"* | **Neither** | Palkia gains +2 evasion. Any damaging move goes into the rift | **+0** |

**WARPED is the zone the fight exists for.** It is the one state where the correct play is to spend
the turn on something that is not damage — set up, heal, status, or step back — and you can see it
coming on the wheel.

A move that matches the zone lands at that zone's own guard, and pays you an ANCHOR rung. A damaging
move that does not match takes the rift chip: **7% / 9% / 11%** of your max HP by phase, and from
Phase 1 it also costs you an ANCHOR rung. Status moves are never wrong.

---

### SPATIAL DISTORTION

One meter, and it is Palkia's guard.

```
 SPATIAL DISTORTION   0         1         2         3          4            5
                    STABLE   RIPPLING   WARPING  FRACTURED  UNMOORED  SHATTERED SPACE

 guard                84        88        90        92         94          96   % damage reduction
                      (the zone modifier above is added on top of this)
```

```
 DISTORTION RISES                          DISTORTION FALLS
 +1  every SPACE SHIFT (Palkia's clock)    -1  you let a wormhole close on nothing
                                           -2  you drive the ANCHOR in (Phase 2)
 ceilings: 2 in Phase 0, 4 in Phase 1, 5 in Phase 2 (floor 3 from Phase 1 on)
```

The level is announced every time it changes, in both directions.

---

### ANCHOR — the meter you own

```
 ANCHOR  0  ------------  1  ------------  2  ------------  3  ANCHORED
```

| Change | When |
| --- | --- |
| **+1** | Your damaging move matched the zone |
| **+1** | You committed no damaging move during a WARPED turn |
| **+1** | You did not attack into an open wormhole |
| **−1** | Your damaging move did not match the zone (Phase 1+; Phase 0 is a free lesson) |
| **0** | A voluntary switch (except in SEPARATED), a faint, or being displaced |

At **ANCHOR 3** two things are true: Palkia's displacement is **refused outright**, and during the
Phase 2 charge you are offered the chance to spend the anchor to collapse the rift.

---

### Wormholes (Phase 1 onward)

```
 turn N      "A wormhole opens beside Palkia."          <- the telegraph
 turn N+1    Palkia is protected for the whole turn     <- the read
             you attacked into it  -> DOUBLE the rift chip
             you did not          -> ANCHOR +1, DISTORTION -1
```

---

### The phases

| HP | Phase | What changes |
| --- | --- | --- |
| 100–50% | **THE FIRST STEP** | Two zones, a 4-turn clock, Distortion capped at 2. Wrong answers are free |
| 50–25% | **SPATIAL COLLAPSE** | WARPED joins the wheel, clock 3, Distortion 3–4, wormholes armed, displacement armed at Distortion 3+, and Palkia sheds its own status at every shift |
| 25–10% | **ORIGIN FORME** | Form change, Spacial Rend replaces Hydro Pump, clock 2, Distortion pinned at 5, the SPATIAL REND charge begins |
| 10–0% | **THE COLLAPSE** | Everything cleared, the guard falls, Poké Balls unlocked |

**Displacement.** From Phase 1, at Distortion 3 or higher, a SPACE SHIFT can tear the field open
under your active Pokémon and drag a random party member out in its place. At ANCHOR 3 it simply
fails. With nobody left on the bench to send in, the tear closes on you instead and you take the
rift chip.

**SPATIAL REND.** In Origin Forme a rift charges over three turns, each one louder than the last,
then fires for **35% of your max HP** — and leaves Palkia recharging for a turn. It restarts
immediately, so Phase 2 is a repeating deadline, not a single event. At ANCHOR 3 you are asked
whether to drive the anchor in: **yes** collapses the charge, drops Distortion two rungs and costs
Palkia its action, but spends the anchor and leaves you displaceable again.

---

### Palkia's moves

| Move | Why it is there |
| --- | --- |
| **Hydro Pump** | Special Water STAB (replaced by **Spacial Rend** in Origin Forme) |
| **Aqua Tail** | Physical Water STAB — COMPRESSED is not a safe place to stand |
| **Dragon Claw** | Physical Dragon STAB, no drawback |
| **Aura Sphere** | Never misses. Double Team, Sand Veil and Bright Powder buy you nothing |

Its ability is **Pressure** until Origin Forme, which brings its own.

---

### Catching it

Poké Balls are blocked for the entire fight. At **10% HP** Palkia's folds collapse, the message says
so plainly, and balls unlock at a catch rate of 30. From that moment the engine will not let a stray
hit finish it off.

---

### The short version

1. Read the zone banner. **COMPRESSED = physical. SEPARATED = special. WARPED = do something else.**
2. The wheel is fixed. Count the turns and know where it is going.
3. Status moves are always safe; save your setup and healing for WARPED.
4. Build the ANCHOR to 3 and hold it — it stops displacement outright and breaks the rend.
5. A telegraphed wormhole is a free ANCHOR rung and a free rung off the guard, if you let it close.
