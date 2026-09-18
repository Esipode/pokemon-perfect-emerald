## Regigigas — The Colossal Titan

Regigigas fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** for the whole fight, so the catch window at the end
is guaranteed no matter how the damage race goes.

It keeps **Slow Start**. Its Attack and Speed are halved for the first five turns on top of
everything below, and the game will tell you so.

Every other legendary gets harder as it loses health. Regigigas gets harder **as you take longer**,
and it is the only one where you hold the dial.

> **Regigigas is asleep. Everything you do wakes it up a little. How fast is your choice — and it
> is not a free one in either direction.**

### The Awakening meter

One number, 0 to 5. **It never goes down.**

```
AWAKENING   0 ......... 1 ......... 2 ......... 3 ......... 4 ......... 5
          DORMANT   STIRRING    RISING     AWAKENED    POWER      FULL
                                                      RESTORED  AWAKENING

  Speed      -6         -4         -2          0          +1        +3
  Attack     -3         -2         -1          0          +2        +4
  Its guard  92         92         89          86         84        82
```

That guard row is the whole fight in one line:

> **A dormant Regigigas is nearly impossible to hurt and barely acts. An awake one is killable and
> lethal.**

Every rung is announced with a line of its own, so you always know where on the ladder you are.

### What moves it

Three things push the meter up. Nothing pushes it down.

| Lever | What it is |
| --- | --- |
| **The clock** | A free rung every **4th turn** — every **3rd** once it is below 50%. Unavoidable. |
| **Burst damage** | Take **12% or more of its HP** since the last rung and it gains one immediately. Your choice. |
| **Thresholds** | 50% forces it to at least AWAKENED. 25% pins it at FULL AWAKENING. Unavoidable. |

Both the clock and a burst can land on the same turn — an enormous hit on the turn it was going to
stir anyway gains it **two** rungs at once.

### Your one lever: stalling

On any turn you land **no damaging move** — a status move, a switch, a Protect, a turn you lost to
paralysis — Regigigas' awakening *falters* and the clock is pushed back two turns.

It is not free:

- The clock cannot be banked past the value it was armed at, so one quiet turn buys back one tick
  and no more.
- **Regigigas heals 3%** in the stillness. *"Its wounds knit closed."*
- **The meter itself never moves.** You are pacing the awakening, not preventing it.

### The Colossus Remembers

Here is the other half of the trade, and the fight will not tell you about it until it is too late
to take back.

While Regigigas is still **dormant** (Awakening 2 or less) it is silently recording three things.
The instant it reaches **AWAKENED**, it cashes every one it caught you doing:

| What it caught you doing while it slept | What it does about it |
| --- | --- |
| Raising a stat to **+2 or better** | It shakes your boosts off — and keeps doing it, **every time you set up again** for the rest of the fight |
| **Switching a Pokémon out** | Nothing leaves the field again. You are trapped. |
| Landing a **status** on it | It throws the status off and raises a **permanent Safeguard**. No status ever sticks again. |

Nothing is said while it is arming. You find out at Awakening 3.

> **Waking it fast arms fewer grudges.** Rush it and you fight a strong Regigigas that knows nothing
> about you. Nurse it for twenty-five turns and you fight a weaker one that has learned everything.
> Both lines work. Neither is free.

Status **does** work on a dormant Regigigas — sleep, paralysis, burn and Toxic all land. Using it is
a real option. It is also a tool you are spending permanently.

The Awakening ladder is **not** protected. Growl, Icy Wind, Intimidate and Charm all work on it, and
holding a rung down between transitions is a legitimate second lever. The next rung change rewrites
its stats outright, so nothing you do to it lasts past the next step.

### Phase 1 — SLUMBER (100% – 50%)

Guard 92, moving last against everything, and **about a third of turns it simply does not act**
(*"Regigigas does not stir."*).

Its moveset here is built so that a sleeping titan is still not a punching bag:

| Move | Why it still hurts |
| --- | --- |
| **Pound** | It doesn't. That's the point. |
| **Payback** | Doubles in power when it moves second — and at −6 Speed it *always* moves second |
| **Body Press** | Attacks with its **Defence**, so its crushed Attack stage is irrelevant |
| **Protect** | It will use it on exactly the turns you don't want it to |

### Phase 2 — THE TITAN MOVES (50% and below)

- The meter jumps by one and **never lands below AWAKENED** — so every grudge you armed cashes here
  at the latest.
- **Reflect, Light Screen and Aurora Veil are torn down.**
- The clock accelerates to a rung every third turn.
- At AWAKENED it trades Pound for **Heavy Slam** (420.0 kg — maximum power against nearly anything).
- From **POWER RESTORED**, about a third of turns it **acts before you do**, for 10% of your
  Pokémon's max HP. *"Regigigas moves before you can!"*
- At **FULL AWAKENING** it gains **Crush Grip** (power scales with your Pokémon's *current* HP, so
  sending in a fresh body to eat a hit is punished specifically) and **Earthquake** — losing Protect
  in the process. A fully awake titan does not defend.

#### CONTINENTAL FORCE

Arms the moment it reaches FULL AWAKENING, and fires **every five turns** from then on.

```
  top of the turn:  "Regigigas plants its feet. The ground begins to split."   <- ONE action
  end of that turn:  CONTINENTAL FORCE - 35% of your Pokemon's max HP
                     then it must recharge, and loses its next turn entirely
```

You get exactly one action between the warning and the blast. Heal, set up a sacrifice, or take it
deliberately.

> **Protect does not stop it.** Continental Force and the pre-emptive strike are scripted, not moves.

### Phase 3 — FULL AWAKENING (25% and below)

The meter is pinned at 5 and there is nothing left to pace. Now the fight starts working **for** you.

**Exhaustion** climbs by 1 every turn Regigigas acts at full power, and by another 2 every
Continental Force. Two lines mark the climb — *"movements are slowing"*, then *"breathing has turned
ragged"* — and at the top:

#### COLLAPSE

```
  Its stats crater:   Speed -6, Attack -3, Def -2, Sp. Def -2
  Its guard falls:    82  ->  55        <- less than half the damage reduction
  It loses a turn outright, then three more turns with everything on the floor
```

*"Regigigas' body still will not answer it."* runs every turn the window is open.

**This is the window you are meant to win in.** After it, Regigigas hauls itself upright, restores
everything, and the exhaustion cycle starts over. It is a safety valve, not a rhythm to farm — if
you cannot close 15% of its health bar in one collapse, you will get another, but you are fighting a
full-strength titan again in the meantime.

### Catching it

Poké Balls are **blocked** for the entire fight.

At **10% HP in the final phase**, the titan sinks to one knee. Everything the fight built comes off
— the trap, the Safeguard, its stat stages — and the balls unlock:

> *"The titan sinks back to one knee, and does not rise. Now — now is the moment to catch it!"*

From that point it cannot be knocked out, so take as many throws as you need.

### The short version

- Chipping keeps it dormant, safe and nearly unkillable — and lets it learn everything about you.
- Bursting kills it fast, into something lethal that knows nothing about you.
- The clock takes it either way. **There is no free pace.**
