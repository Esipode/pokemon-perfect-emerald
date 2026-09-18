## Regice — The Frozen Clock

Regice fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage.

It has **no status immunity at all**. Sleep it, paralyse it, freeze it — every one of those is a
legal play, and none of them will help you. Nothing in this fight is Regice's turn.

Every other legendary so far argues with you about what your turn is *worth*. Regice takes the turn
away.

### The core loop

```
The CHILL runs from 0 to 5, and it rises on a CLOCK.

Nothing you do to REGICE stops that clock.  Only the TEMPERATURE does.

  CHILL 1-2   Both sides stiffen.  You and REGICE lose Speed together.
  CHILL 3-4   The field starts eating an ACTION each turn - yours or
              REGICE's, at random - and REGICE stops feeling the cold.
  CHILL 5     ABSOLUTE ZERO.  Your side loses its action outright.
              REGICE does not.

REGICE also hardens as it freezes.  A stalled fight is a fight you
are losing.

FOUR THINGS THAW IT:
  Fire move on REGICE .......... -2
  Fighting move on REGICE ...... -1
  Harsh sunlight at turn's end . -1
  A heavy blow (12%+ in a turn)  -1

REGICE answers the sun by smothering the sky and putting its HAIL
back.  Sun buys exactly ONE thaw per cast.
```

### Reading the cold

Regice never tells you a number. It tells you every single time the Chill moves:

> *The cold deepens.* → the Chill went up a rung

Each thaw prints its own line, so you always know when one landed:

> *The flames drive the cold back!* → a Fire move (worth two rungs)
> *The ice cracks under the weight of that blow!* → a Fighting move, or a heavy hit
> *The sunlight burns the frost away.* → harsh sunlight was standing at the end of the turn

And it names the tier as you cross into it:

| The line | Where the Chill is | What it means |
| --- | --- | --- |
| *(no line)* | **0** | Regice's softest guard |
| *The air is thickening. Everything is getting slower.* | **1–2** | Both sides lose Speed. Guard climbing |
| *The battlefield has almost stopped moving.* | **3–4** | Actions start disappearing. Guard climbing hard |
| *ABSOLUTE ZERO.* | **5** | You lose the turn. Regice doesn't |

While actions are being eaten, a line repeats every turn:

> *The cold presses in from every side.*

And the single most important line in the fight, said once, the first time the Chill reaches 3:

> *The cold no longer troubles Regice.*

That is the warning. Up to that point the Speed loss was symmetric and mostly cosmetic — you were
still faster. From that point on, only **you** keep slowing down.

### The clock speeds up

| Phase | When | The Chill rises | Ceiling |
| --- | --- | --- | --- |
| **The Frozen Clock** | 100%–50% | every 3rd turn | **4** — Absolute Zero can't happen yet |
| **Deep Freeze** | ≤ 50% | every 2nd turn | 5 |
| **Frozen Tomb** | ≤ 20% | **every turn** | 5 |
| **Weakened** | ≤ 10% | stopped | — |

The first phase is a teaching phase. You will meet the Speed drop, the random lost actions and all
four thaw levers before Absolute Zero is ever on the table. Use it.

### Absolute Zero

At Chill 5, on the turn it happens:

- Your Pokémon loses its **attack** for the turn. (Switching, items and running still work — the
  recharge only eats a move.)
- Regice does not lose anything.
- Regice heals a slice of its HP — the cold preserving it.
- The Chill then falls back to **2**, not 0. You get a recoverable position, not a reset.

That drop to 2 is also your escape hatch in the last phase — see below.

### The Frozen Tomb

Below 20%, at **Chill 3 or higher**, the ice seals the arena:

> *The ice closes in. There's nowhere to go!*

You cannot switch and you cannot run. Thaw back to Chill 2 and it opens again, every time:

> *The ice splits apart! You can move again.*

This matters more than it looks. **Switching out is what clears the Speed stages the cold has piled
on you** — stat stages are per-Pokémon and reset when one leaves. The Tomb confiscates exactly that.
The loop you want in the last phase is: *thaw to break the seal, switch to shed the stiffness, thaw
again.* And because Absolute Zero drops you to Chill 2, it hands you that switch window right after
the worst turn in the fight.

Two things walk out of the Tomb for free, and they are ordinary rules of the game:

- A **Ghost-type** ignores escape prevention entirely.
- A **Shed Shell** works exactly as it always does.

### Building for it

Regice is **80 / 50 / 100 / 50 / 100 / 200**, pure Ice. That 200 Special Defense is the highest of
any legendary in this game by a distance, and its HP pool is the smallest. It is far easier to break
physically than specially.

**Bring Fire.** It is super effective, and it is the only lever worth two rungs. A Fire attacker is
the clean answer to the clock — but it is not a free one:

> **Regice knows Ancient Power.** Rock is super effective on Fire. Bringing the counter it fears
> most is exactly what its coverage is waiting for. Ancient Power also has a rare chance to raise
> every one of Regice's stats at once.

Its other three moves are **Icy Wind** (Ice STAB that pushes the field further toward stillness),
**Ice Beam** (the reliable one) and **Thunderbolt** (for the Water and Steel answers Ice can't
touch).

**Don't bother trying to slow it.** Regice has **Clear Body**. Icy Wind, Sticky Web, Cotton Spore,
Intimidate — nothing you throw at its stats sticks. The only thing in the entire battle that moves
Regice's Speed is the Chill it is generating itself.

**Hail is up the whole fight and it is permanent.** Regice is pure Ice, so it stands in its own
weather untouched while everything you send out chips away. You cannot wait this fight out. If you
clear the sky, Regice puts it straight back at the start of the next turn:

> *Regice smothers the sky, and the frost comes back.*

That re-freeze is also the price of the sun lever. Set harsh sunlight, and it pays out one rung at
the end of that turn — then Regice tears it down before you act again. One cast, one thaw. That is
the whole conversation.

**The heavy-blow lever is the one people miss.** Take 12% or more of Regice's max HP off in a single
turn and the Chill drops a rung, regardless of how many hits it took or whether they were physical
or special. Against 200 base Special Defense that is the lever a special attacker leans on hardest.

**Haze works.** It clears the cold off both sides at once. So does a well-timed switch.

### Catching it

You cannot throw a Poké Ball for almost the entire fight. The window opens only in the final phase,
at **10% HP or less**, and Regice tells you when:

> *Regice's core has gone dark, and the cold with it.*
> *It's worn out — now is the moment to catch it!*

Everything releases at once: the Tomb opens, the Chill goes to 0 and every Speed stage it was
holding comes back to both sides, the hail lifts, the damage reduction and the type-effectiveness
cap come off, and Poké Balls are unlocked. From that point the game keeps the catch target alive for
you — you cannot accidentally KO it out of the window.
