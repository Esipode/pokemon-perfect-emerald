## Mesprit — The Being of Emotion

Mesprit fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** for the whole fight, so the catch window at the end
is guaranteed no matter how the damage race goes.

Azelf measures you. Uxie predicts you. **Mesprit reacts to you.**

It has no fixed personality — it has a **mood**, and you are the one writing it.

> **Mesprit becomes whatever you make it. Every mood is a different fight, and you are choosing
> which fight you get.**

### The core loop

Every turn, Mesprit reads what you did and files a **pull** — the emotion your conduct pushed it
toward. The strongest pull of the turn wins.

```
  WHAT YOU DID THIS TURN                              PULLS IT TOWARD

  nothing much (no damage, no heal, no status)   -->  CALM
  healed, set up, used a status move, switched   -->  JOY
  hurt it a little, or hit it super-effectively  -->  ANGER
  hurt it a LOT in one turn                      -->  FEAR
  statused it, or one of your Pokemon fainted    -->  SADNESS

  Strongest wins:  SADNESS > FEAR > ANGER > JOY > CALM
```

A pull does not change the mood on its own. It has to **repeat**:

- **First half of the fight** — a pull must land **two turns running** to change the mood.
- **Below 50% HP** — **one turn** is enough. *"Its moods begin to turn on a single moment."*

Mesprit announces every mood change by name, and repeats a short line every turn the mood is
running. If you are seeing *"Mesprit's anger has not cooled,"* you are in ANGER.

### The five moods

Each mood is a genuinely different opponent. It owns Mesprit's **guard**, its **stat stances**, its
**fourth move**, and the **contagion** it lays on your Pokémon.

| Mood | How hard it is to hurt | Its stance | 4th move | What it does to you |
| --- | --- | --- | --- | --- |
| **CALM** | normal | — | Swift | nothing |
| **JOY** | normal | Sp. Atk +1, Speed +1 | Draining Kiss | Attack +1, Sp. Atk +1 |
| **ANGER** | **easiest** | Atk +2, Sp. Atk +1, **Def −1** | Double-Edge | Attack +2, **accuracy −1** |
| **FEAR** | **hardest** | Def +1, Sp. Def +1, Speed +2 | Protect | Speed −2 |
| **SADNESS** | hard | Atk −1, Sp. Atk −1, Def +1 | Charm | Attack −1, Sp. Atk −1 |

That guard column is the whole strategy, and it is the thing the fight wants you to work out:

> **ANGER is the only mood you can really hurt it in — and it is the mood that hurts you most.
> FEAR is the mood where your attacks stop working.**

The pulls are measured **after** the mood's guard, which makes the loop self-correcting. Hit it hard
enough to frighten it and its guard closes up, your damage collapses, your pulls drop back to
ANGER-tier, and it drifts back to a mood you can fight. So frightening Mesprit is exactly what stops
you frightening it again.

**The contagion is not all bad.** JOY hands your Pokémon a free +1/+1. ANGER hands you +2 Attack and
takes your accuracy. You can shake a contagion off by switching — but switching is a JOY pull, so the
escape costs you a turn of the mood you wanted.

### EMOTIONAL OVERLOAD

A mood nobody disturbs **festers**. Spend four turns (three, below 50%) doing the same thing to it
and:

> **MESPRIT'S EMOTIONS BECOME OVERWHELMING!**

The mood goes **extreme**, and — this is the important part — **it stays extreme** until *your own
behaviour* moves the mood. You cannot wait it out.

| Extreme | What it does, every turn |
| --- | --- |
| **OVERJOYED** | Heals itself a chunk of its max HP every turn. It will out-heal you. |
| **RAGE** | Its guard **collapses** — it is softer than it has ever been — but it tears 10% off your Pokémon every turn on top of its move. |
| **PANIC** | Its guard is at its **absolute hardest**, and half the time it simply Protects. Nothing you do lands. |
| **DESPAIR** | Drains your Pokémon and heals itself for it — **and washes status off both sides**. |

An undisturbed **CALM** has no extreme. It drifts into JOY instead, and then into OVERJOYED — so a
player who never does anything loses by watching Mesprit heal.

**How to get out of each one:**

- **OVERJOYED?** Stop being gentle. Hit it hard (→ ANGER or FEAR).
- **RAGE?** Stop hitting it hard. Heal, set up, switch (→ JOY).
- **PANIC?** Ease off. A soft turn reads as CALM or JOY.
- **DESPAIR?** Do something decisive — anything that isn't a status move.

### Status is an option, not a lock

Mesprit is pure Psychic with **Levitate**, so Ground does nothing. Statusing it is a legitimate play —
it is the SADNESS pull — but it will not hold:

- **Every mood change washes its status off.** *"Mesprit's mood shifts, and the affliction slips away."*
- **Every emotional surge washes its status off**, and DESPAIR's clears status from **both** sides.

Between them a status lasts about three turns at most. Each SADNESS pull costs a **fresh**
application — a status just sitting there does not keep provoking it.

### EMPATHY — 20% HP

At 20% Mesprit **stops reacting** and keeps what it learned.

> *Mesprit reaches into your Pokémon's heart. It has stopped reacting to you.*

Unannounced, and running since turn one, is a single impression of how you fought it. Every ANGER or
FEAR turn pushed it one way; every JOY turn (heal, set up, switch, Protect) pushed it the other.
Whichever way it landed picks the mood you finish the fight against — **locked, extreme, and
inescapable**, with the recurring surge firing every turn.

| How you fought it | What it keeps |
| --- | --- |
| With fury — you hit hard and often | **RAGE**. Soft, but it lashes you every turn. |
| Cautiously — you healed, set up and switched a lot | **PANIC**. Almost untouchable. |
| Never really committing either way | **DESPAIR**. It drains you and cures itself. |

**You cannot switch and you cannot run** from this point. Whatever you taught it, you now have to
fight through — so it is worth deciding early which of the three endings you would rather have.

### The catch window — 10%

At 10% Mesprit settles, the mood machine shuts down, the contagion comes off your Pokémon, and only
then can you throw a Poké Ball. **Balls are blocked for the entire fight before this point.**

> *Mesprit's eyes go quiet, and it drifts down to you.*

It cannot be knocked out while you are trying to catch it.

### Practical notes

- **Moveset:** Psychic, Dazzling Gleam, Energy Ball, and a fourth slot that changes with the mood.
  Dazzling Gleam is there for the Dark-types you would naturally reach for.
- **Levitate.** Ground moves do nothing.
- **Its only weaknesses are Dark, Ghost and Bug** — and a hit from any of them is automatically an
  ANGER pull, whether you wanted one or not.
- **A quiet turn is never wasted.** Protect turns, setup turns and switches all read as real pulls,
  not as nothing happening.
