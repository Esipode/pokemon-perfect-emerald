## Celebi — The Guardian of Time

Celebi fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It is Psychic/Grass and would otherwise be **4x weak to Bug**, so the effectiveness cap is
doing real work here.

None of that is the fight. The fight is that **damage to Celebi isn't permanent until you make it
permanent.**

### The Echo — the number the whole fight runs on

Celebi keeps one number: an HP percentage it has **recorded**. Call it the Echo. Every few turns it
takes a fresh recording, and every few turns it snaps its own health bar back to it.

> *Celebi begins to record this moment...*

That line is the record window opening. Two turns later:

> *TIME REWIND!*

and Celebi's HP jumps back up to whatever the health bar read when the window opened. Everything you
did in between is gone.

The health bar climbing back to a number you saw two turns ago is the loudest teacher in the fight.
Watch it once and you have the mechanic.

### The Anchor — how you make damage stick

The recording is not a photograph taken once. **It only ever moves down, and it is re-taken the
moment you damage Celebi while the window is open.**

So if you land a hit on the turn the record line prints:

> *You struck as the moment set — this instant is anchored!*

The mark drops to the new, lower value. That damage can never be rewound away, and you have banked
an **anchor** (see [TIME COLLAPSE](#time-collapse--what-the-anchors-were-for) — they matter again at
the very end).

Note the wording of the anchor test: it fires when the recorded value **actually moves down**. A
miss, an immunity, or a status move that does no damage does not anchor, and the window is spent for
that turn either way. One attempt per turn.

### The cycle, and how to play it

Phase 1 runs on a three-turn loop:

| Turn | What happens | Is your damage permanent? |
| --- | --- | --- |
| 1 | **Record window opens.** Hit now and you anchor. | Yes |
| 2 | Nothing. A free turn — for Celebi. | **No** |
| 3 | **TIME REWIND** at the top of the turn, then you act. | Yes (the next window records it) |

Two turns in three, your damage counts, and *you* choose which two. The whole optimisation is:
**burst on the record turn, coast on the free turn.** Setup, healing, status, a switch, PP you don't
mind losing — put them on turn 2, where the rewind was going to erase your damage anyway.

Playing it wrong isn't fatal, just slow: grind evenly and about a third of everything you do is
thrown away. Playing it right roughly halves the fight.

Two things the rewind takes beyond HP:

- **Stat stages reset to neutral on both sides.** Your set-up goes with Celebi's.
- **Any status you landed on Celebi is cured.** Celebi has no blanket status immunity — it just
  doesn't need one, because the rewind cures it on a timer Celebi controls. Sleep and paralysis are
  real, they're just rented.

And one direction people miss: the rewind moves the health bar **to** the mark, not upward from it.

### Future Sight — the mind game

At **60% HP** Celebi stops reacting to you and starts reading ahead.

> *It has stopped reacting to you. It is reading ahead.*

From here, every turn opens with Celebi telling you what it has foreseen about the move you are
about to pick:

| Line | It expects |
| --- | --- |
| *Celebi has seen a fierce blow coming...* | a **physical** move |
| *Celebi has seen a gathering of strange power...* | a **special** move |
| *Celebi has seen a moment of stillness coming...* | a **status** move |
| *Celebi looks ahead, and finds the future clouded.* | nothing — free turn |

This is not scripted. It is the battle AI's genuine prediction of what you would do in your position,
printed out loud. Which means it is usually right, and which means **it is beatable on purpose.**

**Play the foreseen category** and it lands:

> *Celebi was already there.*

You take a chunk of damage, and the next rewind arrives a turn sooner.

**Play anything else** and the future breaks:

> *The future Celebi saw did not come to pass!*

The pending rewind is **cancelled outright** — that cycle's damage all survives — and Celebi's guard
falls hard for two turns. This is the single fastest thing you can do in the fight, and it costs you
nothing but the discipline to pick your second-favourite move.

The category is what matters, not the move. If Celebi calls a physical blow, any special or status
move breaks it. Switching sidesteps the whole exchange, and gets you nothing.

### Temporal Paradox — the rewind that doesn't rewind

From Future Sight on, roughly one rewind in three is replaced by something else:

> *Celebi distorts the flow of time!*

A paradox **replaces** that turn's rewind, so your damage all survives — but whatever you did during
the cycle comes back around, in this order:

1. **You healed since the recording** — Celebi copies the recovery and heals itself 10%.
   *"The moment of your recovery repeats — for Celebi."*
2. **You have an Attack, Sp. Atk or Speed boost up** — Celebi copies the ascent, taking +1 Sp. Atk
   and +1 Speed. *"The moment of your ascent repeats — for Celebi."*
3. **Neither** — the blow returns. *"Your own blow returns out of the past!"* and your active
   Pokémon takes 12% of its max HP.

It reads the same board the rest of the fight does, so the paradox is another reason to think about
*when* you heal and *when* you set up, not just whether.

### Temporal Collapse — the last quarter

At **25% HP** the fight tightens:

> *Time itself is fracturing around Celebi!*

- The record/rewind loop shortens from three turns to **two** — record, then rewind. Only half your
  damage sticks now unless you anchor.
- **Rain falls** — "the rain of a day long past". It's atmosphere and it's a wall: it blunts the Fire
  answer to a Grass type. Bring a second angle.
- Celebi **sheds status at both ends of every turn** on its own, not just when it rewinds.
- Celebi **cannot be knocked out** during this phase, so the finale below is guaranteed to happen.

### TIME COLLAPSE — what the anchors were for

At **13% HP**, one last rewind, and it is the anchors you banked all fight that decide how much of it
lands.

| Anchors banked | What happens |
| --- | --- |
| **3 or more** | **The collapse fails.** *"The anchored moments hold — Celebi cannot reach past them!"* No heal at all, and Celebi is left wide open for **three turns** — the softest it is at any point in the fight. This is the win condition. |
| 1–2 | Partial. Celebi restores to the Echo and no further, and is open for one turn. |
| 0 | Full. Celebi restores to the Echo, heals a further 20%, resets both sides' stat stages, cures itself, and hits your active Pokémon for 20%. |

Anchors are capped at 5, and you only need 3. That is roughly three well-timed hits across the entire
fight — but they have to be *timed*, and if you never noticed the record line you will have zero.

### What does and doesn't stick

- **Status is temporary by construction.** Every rewind cures Celebi, and in Temporal Collapse it
  sheds at both ends of every turn. Statusing it isn't wasted, it's just rented — plan around the
  cycle.
- **Stat stages are erased on both sides by every rewind**, and copied onto Celebi by a paradox.
  Setting up is a real option — on the record turn, so you get value out of it before it goes.
- **Toxic is bounded** (flat damage, no ramp) and OHKO/fixed-damage/Pain Split/Perish Song are shut
  out entirely.
- **Bug is Celebi's biggest weakness and it is capped at 2x**, not 4x. Fire, Ice, Poison, Flying,
  Ghost and Dark are all 2x as well — the rain blunts the Fire route specifically in the last phase.
- **Ancient Power is the move that punishes your counters.** Rock hits Bug, Flying, Fire and Ice
  super effectively, which is four of the seven types you would naturally bring.
- **Future Sight is on Celebi's own moveset too**, landing two turns after it's chosen. It will
  sometimes arrive on the same turn as a rewind. Track both.

### Phases and catching it

1. **The Guardian of Time** (above 60% HP) — guard **84%**. The record/rewind loop runs on three
   turns. No prediction yet.
2. **Future Sight** (60% HP or below) — guard **88%**. Every turn opens with a stated prediction;
   matching it hurts and speeds the rewind up, defying it cancels the rewind and drops the guard to
   **55%** for two turns. Temporal Paradox starts replacing some rewinds.
3. **Temporal Collapse** (25% HP or below) — guard **92%**. The loop shortens to two turns, rain
   falls, and Celebi sheds status at both ends of every turn. It cannot be knocked out here, so
   **TIME COLLAPSE** at 13% always plays. Hold it off with three anchors and the guard falls to
   **45%** for three turns.
4. **Weakened** (10% HP or below, after the collapse) — Celebi drops out of the flow of time. Every
   mechanic switches off, the guard drops to 75%, and **Poké Balls work**. The only catch window.
   Its catch rate is generous here, and the catch-phase safety net (see
   [Common to every legendary](#common-to-every-legendary)) keeps Celebi alive through your hits.

---
