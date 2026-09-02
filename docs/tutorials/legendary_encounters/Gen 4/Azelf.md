## Azelf — The Being of Willpower

Azelf fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. On top of that it **cannot be taken below 1 HP** for almost the whole fight — you are not
going to overkill it, and you are not going to end it early.

Every other legendary asks you to find a tool. Azelf asks a harder question: **are you hitting hard
enough?**

### The core loop

```
Every turn, the fight MEASURES how much of Azelf's health you took
off, and gives you a verdict.

    over THE BAR   ->  "Azelf's resolve falters!"    RESOLVE -1
    under the bar  ->  "Your blows only steel it!"   RESOLVE +1
    no damage      ->  nothing at all

THE BAR:  Phase 1  a real, committed hit
          Phase 2  noticeably more than that

RESOLVE runs 0 to 5.  It is the whole fight.
```

Chipping it down safely — the default, correct way to fight a boss that shrugs off most damage — is
the **losing** strategy here. Half-measures feed the meter. A turn where you do nothing at all is
free; a turn where you try and fall short is not.

### The ladder

Resolve is not a punishment track. It is five named rungs, and **every one of them turns back off**
the moment you push the meter back down. Azelf announces every change in either direction.

| Resolve | State | What it does |
| --- | --- | --- |
| 1 | **UNYIELDING** | Throws off any status at the end of the turn — *and gains Resolve for it* |
| 2 | **UNDIMINISHED** | Stat drops on its Attack, Sp. Atk and Speed do not stick |
| 3 | **UNBOWED** | *It is holding something back.* (see SECOND WIND) |
| 4 | **UNMATCHED** | Speed +2 — it moves before you |
| 5 | **UNBREAKABLE** | Attack and Sp. Atk +2, **and it lashes back every turn you damage it** |

While it is at UNMATCHED or above it keeps saying so, every turn: *"Azelf's will still burns
white-hot."* If you are seeing that line, you are losing the meter.

**Status is a trap.** Putting Azelf to sleep, burning it or poisoning it is a good idea at Resolve 0
and a bad idea from Resolve 1 onward — from there it shrugs the status off at the end of the turn
*and the attempt itself steels it*. The line tells you the first time it happens.

### What raises Resolve

| Source | Amount |
| --- | --- |
| A turn where you damaged it but stayed under the bar | +1 |
| Azelf drops past 70% | +1 |
| Azelf drops past 45% (**RESOLUTE**) | +1 |
| One of your Pokémon faints | +1 |
| It shrugs off a status | +1 |

The two HP-threshold gains are free and unavoidable, so the meter always climbs at least twice on
its own. If you never push back, you arrive at the decision point with UNBOWED armed.

### Your two levers

| Lever | How | Amount |
| --- | --- | --- |
| **BURST** | Clear the bar — take a big enough bite out of it in one turn | −1 |
| **ENDURE** | Weather its **Extrasensory** and still be standing | −1 |

Each fires at most once per turn, so a turn where you eat Extrasensory *and* clear the bar is worth
**−2**. That is the best turn in the fight, and it is how you get under Resolve 3 before 30%.

### SECOND WIND — the fight is decided at 30%

When Azelf drops to 30% HP, it reaches for its will. What it finds there is entirely up to you.

**Resolve 3 or higher:**

> **AZELF REFUSES TO FALL!**
> *Its will floods back, brighter than before.*

It heals back to **45%** — most of your second-half progress, gone — and jumps straight to
**UNBREAKABLE**, lashing you every turn from there.

**Resolve 2 or lower:**

> *Azelf reaches for its will — and finds nothing there.*
> *It has spent everything it had.*

Nothing happens. Walk it down.

This is what the entire meter exists for. Everything you did in the first half of the fight is
settled in one moment.

### LAST STAND — 20%

The finale, and it is endurance rather than a damage race.

- **Four full turns.** You cannot shorten them; Azelf cannot be taken below 1 HP.
- **Resolve is pinned at 5.** Nothing you do moves the meter, and the lash is live every turn.
- **You cannot switch and you cannot run.** *Azelf plants itself between you and the way out.*
- Its guard **tightens** further.

> *Azelf will not fall while it still has a will to fight!*

Bring healing, bring HP, and bring a Pokémon that can absorb four turns of a boosted Azelf plus a
per-turn lash. This is the part of the fight to prepare for.

### WILL BROKEN, and the catch window

When the clock runs out:

> *Azelf's will finally breaks.*
> *Its body slumps. Whatever was holding it up is gone.*

The ladder collapses to nothing and its guard **falls apart** — your next hit is worth many times
what it was a moment ago. It cannot be overkilled on the way down, so just keep swinging.

At **10%** it settles to the ground, and only then can you throw a Poké Ball at it. Balls are
blocked for the entire fight before that point.

### Practical notes

- **Moveset:** Extrasensory, Psychic, Dazzling Gleam, Shadow Ball. The last two are there to punish
  the Dark- and Ghost-types you would naturally reach for.
- **Bug is unanswered.** Azelf covers two of its three weaknesses and deliberately not the third.
- **Levitate.** Ground moves do nothing.
- **Nothing heals it except Second Wind**, and Second Wind happens once.
