## Keldeo — The Colt Pokémon

Keldeo fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Keldeo fights you honestly, and it only gets stronger by testing itself against you. Its guard
> drops while it duels — and once its Resolve is complete, it stops duelling.**

Two numbers run the fight, and both are announced whenever they change.

---

### THE DUEL — and the guard

Every other Sword of Justice owns its guard from its phase or its meter. **Keldeo's guard is owned by
whether a duel is running.**

```
              guarded   duelling
 Phase 0        93         80
 Phase 1        94         82
 Phase 2        95      (never duels)

 a broken Sacred Sword: guarded − 15 for one turn (78 / 79 / 80)
```

On a cooldown, Keldeo **locks eyes with your active Pokémon and challenges it to a duel**:

- Your Pokémon is **trapped** for the duel's length (3 turns in Phase 0, 4 from Phase 1). Switching
  and fleeing are refused. Ghost-types and Baton Pass still leave under the normal rules.
- Keldeo **drops its guard** for as long as the duel runs. This is the only real damage
  window in the fight.
- **Win the duel** (survive until time runs out) and Keldeo's Resolve *falls* by 1 — but the duels
  keep coming.
- **Lose the duel** (let it KO your Pokémon) and Keldeo's Resolve *rises* by 2, fast.

The challenge is announced before you pick a move. The cooldown is 3 turns in Phase 0, 2 from
Phase 1.

---

### RESOLVE — 0 to 5

Resolve is **pure offence**. It buys Keldeo nothing defensive at all.

```
 0 Apprentice  nothing but the callout
 1 Student     +1 Speed stage
 2 Swordsman   permanently raised critical-hit ratio
 3 Adept       RIPOSTE
 4 Master      THE DISARMING STROKE, and Riposte hits harder
 5 Resolute    form change, the meter LOCKS, the duels stop, the SACRED SWORD test begins

  +2  Keldeo won a duel          −1  Keldeo lost a duel
  +1  it survived a super-effective hit
  +1  it shed a status at the top of a turn   (Phase 1+ only)
  +2  reaching 50% HP            → 5  reaching 20% HP (forced)
```

Ranks **1 and 2 are taken back** when Resolve falls — a lost duel really does cost Keldeo its Speed
stage or its crit ratio.

- **RIPOSTE (Resolve 3+):** any turn you take health off Keldeo **while no duel is running**, it
  answers with a counter-slash for 10% of your side (14% at Resolve 4+). Inside a duel it fights
  straight and takes the hit — the duel is the window, and Riposte is what tells you so.
- **THE DISARMING STROKE (Resolve 4+):** every duel now opens by wiping your stat stages and
  scouring your screens.

---

### The moveset

| Slot | Phase 0 | Mentors' Teachings (50%) | Resolute (Resolve 5) |
| --- | --- | --- | --- |
| 1 | Sacred Sword | (same) | (same) |
| 2 | Scald | **Hydro Pump** | (same) |
| 3 | Aqua Jet | (same) | (same) |
| 4 | Aura Sphere | (same) | **Secret Sword** |

- **Sacred Sword** ignores your defensive stat changes and your evasion — Double Team and Cosmic
  Power stalling do not work. Fixed slot, never leaves.
- **Aura Sphere** never misses either.
- **Scald** burns; **Aqua Jet** is priority and finishes a Pokémon trying to limp out of a duel.
- **Justified** is its ability: a Dark-type move raises its Attack.

---

### Phase 1 — MENTORS' TEACHINGS (50%)

- Slot 2 becomes **Hydro Pump**; raw **Sp. Atk +15%**, written into the stats (a Haze will not
  remove it)
- **+2 Resolve**
- Duels last **4 turns**, cooldown drops to **2**
- Keldeo now **sheds status at the top of every turn** — status keeps working, but costs you a move
  each turn *and hands Keldeo +1 Resolve*. In Phase 0 a status simply sticks and works, which is the
  strongest opening play in the fight.

---

### Phase 2 — TRUE RESOLVE (20%)

Resolve is **forced to 5**. Keldeo becomes Resolute (if it was not already), any running duel ends,
and **it never duels again**. `Survive: True` carries the fight to 1 HP.

From here the only damage window is **breaking the Sacred Sword**.

---

### Resolute — the SACRED SWORD test

Once Resolve hits 5, Keldeo changes form (raw **Sp. Atk +15% / Speed +10%**, Secret Sword in slot 4)
and begins a repeating test:

- At the **start of a turn** it *raises its blade* — you see this before you choose a move.
- At the **end of that same turn** it resolves:
  - **Interrupted** — a status on Keldeo, **or** you took **25%+ of its max HP that turn** — the
    stroke comes apart. Keldeo loses its next action, takes −1 Def / −1 Sp. Def, and its guard
    falls 15 for one turn. **The biggest window in the fight.**
  - **Not interrupted** — **SACRED SWORD** lands for **30% / 36% / 42%** of your side, escalating
    each time it connects.

---

### The last stand, and the catch

At 1 HP Keldeo's legs give out: one last Secret Sword for 10% of your side, and only then do the
guard, the immunities, the type cap and Survive all come off, and Poké Balls unlock at a catch rate
of 30. **This is the only point in the fight where a ball works at all.**

---

### What does and does not work on it

| Plan | What happens |
| --- | --- |
| Sleep / paralysis / freeze | Land and work completely. In Phase 0 that is a strong opening. From 50% Keldeo sheds status every turn — it still works, but costs you a move and feeds Resolve +1 |
| Toxic | Lands, damage is flat, and it is shed from Phase 1 (feeding Resolve) |
| A super-effective coverage move (Flying, Psychic, Electric, Grass, Fairy) | It lands — and it *tempers* Keldeo: **+1 Resolve** if Keldeo survives it. The obvious answer feeds the meter |
| A Dark-type answer | **Justified** — raises its Attack |
| Double Team / Cosmic Power walling | **Sacred Sword and Aura Sphere ignore evasion and defensive stat changes** |
| Switching away from a duel | Refused — you are trapped for the duel's length |
| Winning duels | Keldeo's Resolve falls, and the duel window keeps coming back. This is the skilled line |
| Losing duels on purpose to end them | Resolve climbs +2 each time, straight toward Resolute, where the duels stop for good |
| PP stalling | Every phase transition refills its moves |

---

### The short version

1. **The duel is the only real damage window.** You want Keldeo to keep challenging you — so
   **survive duels**, don't lose them.
2. **Winning a duel lowers Resolve; losing one raises it by 2.** Resolve is all offence — Speed,
   crits, Riposte, the Disarming Stroke.
3. **Don't hit Keldeo outside a duel once it is at Resolve 3+** — that is what Riposte punishes.
4. **Super-effective coverage tempers it.** Landing a weakness hit and leaving Keldeo standing gives
   it +1 Resolve.
5. **Phase 2 makes it Resolute.** The duels stop; now you race the Sacred Sword — interrupt it with
   a status or 25% of its HP in one turn for the big window, or eat it for up to 42%.
6. Save your ball. It does nothing until Keldeo falls to 1 HP.
