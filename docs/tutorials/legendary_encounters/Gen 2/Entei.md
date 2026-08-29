## Entei — The Walking Volcano

Entei fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a 2x
cap on incoming type effectiveness, and flat Toxic damage. It can't be caught until its final,
weakened phase. What makes this fight different: **Entei's guard is a locked door, not a slider —
and the only key is an eruption.**

Raikou is a number you fight downward. Entei is a bomb timer you cannot defuse. It tracks a hidden
**Pressure** from 0 to 5, and Pressure **never falls on its own**. There is no cooling move, no
withholding play, no status lock that stops it. It only ever goes up, and it is only ever *spent* —
by erupting. You do not decide whether the volcano goes off. You decide **when**, **how big**, and
**what you are standing on** when it does.

### Pressure — the number you cannot turn down

| Pressure goes up when | By |
| --- | --- |
| A damaging move of yours hits Entei | +1 (once per turn) |
| Entei uses a **Fire** move | +1 (once per turn) |
| Entei is carrying **any status** at the start of a turn | +1, **every turn it lasts** |
| One of your Pokémon faints | +1 |
| The ground is at full Scorch at the end of a turn | +1 (and Entei heals 3%) |
| Any turn at all, once the eruption cycle begins | +1, unconditionally |

Nothing on the other side of that table exists. Watch the two lines it prints — *"The heat under the
battlefield climbs"* at 1–2, *"Heat is pouring off Entei's back in waves!"* at 3–4 — and count.

### The eruption, and the two turns of warning

Pressure reaching 5 doesn't erupt. It **lights a fuse**, and the fuse is loud:

```
Pressure hits 5   ->  "The earth begins to tremble beneath you!"
next turn's end   ->  "Magma is bubbling up through the cracks!"
next turn's end   ->  "The ground splits open - something is coming!"
next turn's start ->  ERUPTION
```

You get two full turns of warning — **one turn** once the Cataclysm begins. Pressure stops
accumulating entirely while a fuse is lit, so the countdown is honest: once it starts it can be
neither accelerated nor stalled.

When it lands, in order:

1. **20–35% of max HP** off your active Pokémon, set by how scorched the ground is. This is
   percentage HP loss — it **ignores typing entirely**, so a bulky Water type walls Entei's moveset
   and nothing at all walls the volcano.
2. Every stat change on the field is scoured off, both sides.
3. The ground slams back to full Scorch.
4. Entei is **thrown awake** by its own blast — any status it was carrying is gone.
5. Entei takes 8% recoil.
6. **Its guard drops to 65% for two full turns.** This is your window.

### Scorch — the field between eruptions

Separate from Pressure, the ground tracks **Scorch**, 0 to 3. It never chips you on a timer. It is a
**transition tax** and the eruption's **fuel gauge**:

| Scorch | Switching in costs | End of turn | Next eruption hits for |
| --- | --- | --- | --- |
| 0 | — | — | 20% |
| 1 | 5% max HP | — | 25% |
| 2 | 9% max HP | — | 30% |
| 3 | 14% max HP | Entei heals 3%, Pressure +1 | 35% |

Every eruption slams Scorch to 3. Your job between eruptions is to cool it back down before the next
one lands, and there are exactly two levers:

- **A Water move that hits Entei** — Scorch −1. It still feeds Pressure; being the cooling lever
  doesn't exempt it from the rule.
- **Rain** — Scorch −1 at the start of every turn it's up, for free.

Rain is the single most valuable thing you can bring to this fight. In the **Cataclysm** phase the
field locks at Scorch 3 and both levers stop working.

### Pressure Break — why turtling backfires

If you're holding **any positive stat stage** while Entei is loaded (Pressure 3 or more, no fuse
lit), it vents early: *"Entei releases a wave of volcanic heat!"* — Pressure −2, 12% off your active
Pokémon, every stat change on the field scoured away, Scorch +1, and a **1-turn window at 78%**. It
can't do this again for three turns.

That is the central trade of the fight:

- **Turtle up** and you force small, early, weak eruptions — a small window each time, but you never
  face a full one.
- **Stay lean** and Pressure climbs to 5 on its own — a devastating eruption, but the biggest window
  in the fight attached to it.

### The guard is a door, not a slider

| State | Guard | Duration | The line that tells you |
| --- | --- | --- | --- |
| **Dormant** | 90% | the whole phase | |
| **Eruption cycle** | 92% | the whole phase | "The heat haze thickens — Entei is harder to reach!" |
| **Cataclysm** | 94% | the whole phase | " |
| **After an eruption** | **65%** | 2 turns | "Entei is spent from the blast — it's wide open!" |
| **After a Cataclysm** | **55%** | 3 turns | " |
| **After a forced early vent** | **78%** | 1 turn | "Entei is off balance for a moment!" |

Nothing you do moves the flat value. Your damage window is not something you maintain — it is
something you **survive into**. *"Entei still hasn't recovered its footing!"* means the window is
still open; *"Entei plants its feet — the moment is gone"* means it has closed.

### What does and doesn't stick

Entei has **no status immunities at all**. That is deliberate, and it is a trap.

- **Burn does nothing** — it's a Fire type. **Freeze does nothing** — Fire types can't be frozen.
- **Sleep works.** And it is the worst thing you can do. A sleeping Entei still gains **+1 Pressure
  every turn** (*"Entei can't move, and the pressure has nowhere to go!"*), the fuse still burns, and
  **the eruption fires whether or not Entei can act** — it's a field event, not a move. Then the
  eruption **wakes it up**. Sleep buys you a few quiet turns and charges you a full eruption on a
  scorched field for them.
- **Paralysis and poison stick normally**, and both feed Pressure every turn they last. Toxic is a
  genuine trade here rather than a free win.
- **Water is your lever twice over** — best damage and the only way to cool the ground.
- **Ground and Rock are dead weight against its typing**, and Entei carries **Bulldoze** for the
  Rock/Steel bodies you'd normally bring. **Extrasensory** answers Fighting.
- **Eruption gets weaker as the fight goes on** — its power scales with Entei's current HP. Its own
  fire runs out; the mountain takes over.

### Phases and catching it

1. **Dormant** (above 50% HP) — Pressure builds only from what you and it actually do. Guard 90%.
2. **Eruption cycle** (50% HP or below) — *"The mountain has started to move."* Pressure can never
   fall below **2** again and now climbs **+1 every turn** on its own, so the eruptions come whatever
   you do. Entei's Attack and Sp. Atk rise a stage; guard hardens to 92%.
3. **Cataclysm** (25% HP or below) — *"There is nowhere left that isn't burning."* The field locks at
   Scorch 3 and cooling stops working, Pressure floors at **3** and still climbs every turn, and the
   fuse shortens to **one turn of warning**. Every eruption is now a **Cataclysm**: 35% off your
   active Pokémon and **three full turns at 55%** — the biggest window in the fight, arriving roughly
   every third turn. Guard 94% between them. This is a race, and it is where the fight is won.
4. **Weakened** (10% HP or below) — the fire under Entei gutters out. Every guard drops, every
   mechanic switches off, and **Poké Balls work**. The only catch window. Its catch rate is generous
   here, and the catch-phase safety net (see [Common to every legendary](#common-to-every-legendary))
   keeps Entei alive through your hits.

---
