## Dialga — The Temporal Pokémon

Dialga fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Every other boss surprises you. Dialga tells you.** Before each interval begins it says out loud
> what it has marked, how much of itself you must remove, and how many turns you have. Miss the
> deadline and those turns did not happen — your damage is erased. Make it and Dialga loses its
> footing.

This is a **stated damage quota against a visible clock**. Nothing else in the set is.

---

### TEMPORAL CHARGE

One meter runs everything. It is also Dialga's guard.

```
 TEMPORAL CHARGE   0          1          2          3          4          5
                 SETTLED   TICKING    FLOWING   ACCELERATED  UNMOORED   TIME MASTERY

 guard             82         85         88         90         92         94   % damage reduction
 demand            14         12         10          9          8          7   % of Dialga's max HP
 clock              4          4          3          3          2          2   turns per interval
```

The two columns that matter move against each other. A higher Charge means Dialga takes less damage
**and** gives you fewer turns — but the quota shrinks too, because a quota calibrated for an 82%
guard is impossible behind a 94% one. The fight stays winnable at Charge 5. It is just *tight*:
roughly two good turns, in two turns, with no margin.

```
 CHARGE RISES                                CHARGE FALLS
 +1  an interval collapses (quota missed)    -1  an interval holds (quota met)
 +1  one of your Pokémon faints
 floors: 2 from Temporal Acceleration, pinned at 5 in Origin Forme
```

There is exactly **one** lever you have on the meter, and it is the same thing the fight is already
asking for: **hit the quota.** The level is announced every time it changes, in both directions — a
hold that drops it speaks as loudly as a collapse that raises it.

| Level | Dialga says |
| --- | --- |
| **5** | *"TIME MASTERY. Dialga owns every moment on this field."* |
| **4** | *"TEMPORAL CHARGE: UNMOORED. Your turns are arriving before you have taken them."* |
| **3** | *"TEMPORAL CHARGE: ACCELERATED. Dialga is moving through a faster hour than you are."* |
| **2** | *"TEMPORAL CHARGE: FLOWING. The moments are running together."* |
| **1** | *"TEMPORAL CHARGE: TICKING. Something has started counting."* |
| **0** | *"Time is running the way it should."* |

---

### The loop

```
 TURN START                                     TURN END
 ----------                                     --------
 no interval open?  -> THE MARK                 the clock ticks down one
      records Dialga's HP percentage            clock reached 0?
      sheds any status Dialga is carrying          quota met    -> THE TIMELINE HOLDS
      states the quota and the clock               quota missed -> THE INTERVAL COLLAPSES
      (Acceleration+: AGE)
 interval live?     -> the countdown line
```

**THE TIMELINE HOLDS** — *"Dialga's grip on the moment slips."* Charge drops a rung (the guard drops
with it) and **Dialga loses its next action.** A free turn against a boss that just got softer is the
most valuable thing in this fight. Two holds in a row is how you actually win it.

**THE INTERVAL COLLAPSES** — *"Those turns did not happen."* Dialga's HP snaps back to the marked
percentage, **both sides are Hazed** (your setup goes, but so does anything it built on you), and
Charge rises a rung. From Temporal Acceleration onward a collapse also fires **THE ECHO**.

The countdown line prints **every turn an interval is live**, so you always know how long you have.

---

### Status, and what Dialga does about it

Dialga has **no status immunities at all.** Instead, *every time it marks a moment it returns to that
moment*, which includes shedding whatever status it is carrying.

So a sleep or a paralysis lands, works completely, and lasts up to a full interval — 4 turns at
Charge 0, 2 at Charge 5. **Status Dialga immediately after a mark and you get nearly the whole
interval out of it.** Status it the turn before a mark and you get one turn.

Toxic is bounded to a flat tick as usual. Evasion tricks do not buy a free interval: **Aura Sphere
never misses.**

---

### The phases

| Phase | HP | What changes |
| --- | --- | --- |
| **The Flow of Time** | 100–50% | The loop, clean. Nothing else. |
| **Temporal Acceleration** | 50–20% | Clocks one turn shorter (floor 2). Every mark AGEs you. Every collapse ECHOes. Charge floors at 2. |
| **Origin Forme** | 20–10% | Form change, Roar of Time, and TIME FRACTURE. Charge pinned at 5. The deadline stops. |
| **The Convergence** | 10–0% | Guard falls, everything is released, **Poké Balls unlock.** |

#### AGE — *"Time accelerates around your Pokémon!"*

From Temporal Acceleration, every mark:

- cuts your active Pokémon's **Attack, Sp. Attack and Speed by 15% each** — of the *raw stat it was
  built with*, not a stat stage. It is invisible on the summary screen, it is **not** undone by Haze,
  and it **compounds** (0.85, 0.72, 0.61, 0.52 …) on a Pokémon that stays in;
- shorts out its held item for 3 turns (Embargo);
- strips your screens, Safeguard, Mist, Tailwind and Lucky Chant, and clears the weather.

The counter is in the rule itself: **raw stat decay resets the moment a fresh Pokémon switches in.**
The fight is telling you to rotate your team, in the only language a damage number speaks. Dialga is
never given a trapping effect before Origin Forme, so that door is always open — but the fight is
counting how often you use it.

#### THE ECHO

A collapse in Temporal Acceleration or later tears a strike out of the erased interval for **12% of
your Pokémon's max HP**. Because the countdown is public, you know exactly which turn it is coming
on. **Protect blunts it to 4%.**

#### Origin Forme

At 20% Dialga becomes **Dialga-Origin** — still Steel/Dragon, with 20 points moved out of Attack into
Sp. Defense, so it gets harder to wear down and no harder to survive. It gains **Roar of Time** in
place of Draco Meteor; Roar of Time's own recharge turn is a gift back to you.

The deadline machinery stops here. What replaces it is **TIME FRACTURE**.

---

### TIME FRACTURE — the ledger picks your ending

Dialga has been keeping count all fight. At Origin Forme it names whichever thing you leaned on
hardest, checked in this fixed order:

| Checked | Future | What it does |
| --- | --- | --- |
| Switched out **3+ times** | **THE LOCK** | *"There is no longer a moment in which you leave."* You are trapped, and AGE now fires **every turn** instead of every mark. The switching that answered aging is gone. |
| Ended **6+ turns** without committing a damaging move | **THE CLOSING** | *"The hours you were counting on are already spent."* Heal Block, re-armed every turn — and any turn you end without attacking costs you **10% of your max HP**. Stalling stops working. |
| Otherwise (the default) | **THE RECOIL** | *"The moment you struck comes back around!"* Every turn you damage Dialga, it answers with a fixed **10%** bite. The glass-cannon answer stops being free. |

Aggression is the default because it is what a damage-reduced boss most reliably produces.

---

### The Convergence — the catch window

At 10% HP, in Origin Forme, everything comes off: the trapping, the Heal Block, the guard, the
immunities, the type cap, the flat Toxic, the survive clamp. Dialga tells you plainly:

> *"Only this moment is left — and Dialga is barely holding it. Now — now is the moment to catch it!"*

**Poké Balls are blocked for the entire fight until this line prints.** From here the engine keeps
Dialga alive against a stray hit, so you can throw freely. You catch it in Origin Forme — the thing
you actually beat.

---

### Dialga's moveset

| Move | Why it is there |
| --- | --- |
| **Draco Meteor** | The nuke, off a 150 base Sp. Atk. Its own Sp. Atk drop gives the fight a rhythm — the turn after a Meteor is your turn. Replaced by Roar of Time at Origin Forme. |
| **Flash Cannon** | Reliable Steel STAB with no drawback. |
| **Earth Power** | Coverage against the Steel and Fire walls you bring to a Steel/Dragon fight. |
| **Aura Sphere** | Never misses. Evasion does not buy you a free interval. |

Ability: **Pressure.** AI: **smart trainer** — it plays properly, and it can read the damage
reduction, so do not expect it to be predictable about what it leads with.

---

### How to actually win

1. **Read the quota line, then commit.** The clock and the price are both stated. Work out whether
   your best two or three turns clear the bar *before* you spend them, not after.
2. **Bank a hold early.** Charge 0 gives you four turns for 14% — the most forgiving deadline in the
   fight. Holds there keep the guard low and stack free turns.
3. **Status right after a mark, never right before one.**
4. **Rotate your team once aging starts.** A Pokémon that has been aged three times is at roughly
   61% of its offence. A fresh one is at 100%. The Flight ledger charges you for it — three switches
   buys THE LOCK — so spend them, but count them.
5. **Save Protect for the collapse you can see coming.** The countdown tells you which turn the ECHO
   lands on.
6. **Attack at least once per turn if you can.** Six lazy turns buys THE CLOSING, which punishes the
   exact thing that earned it.
