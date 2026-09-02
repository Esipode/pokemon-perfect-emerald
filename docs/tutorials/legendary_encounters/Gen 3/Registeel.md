## Registeel — The Adaptive Machine

Registeel fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage.

Regirock is a wall you have to pick the right tool for. Registeel is a wall that **becomes the wrong
tool's wall** — it learns whatever you keep hitting it with, and the only fight it cannot solve is
one it has never seen.

### The core loop

```
ANALYSIS.  Every damaging move that lands on REGISTEEL is filed by
TYPE.  A filed type is an ADAPTATION: that type does far less
damage from then on.

  Phase 1  ANALYSIS               2 slots  -40%  one expires every 4th turn
  Phase 2  OPTIMIZATION           3 slots  -50%  one expires every 6th turn
  Phase 3  PERFECT CONFIGURATION  4 slots  -55%  nothing expires

Slots are FIRST IN, FIRST OUT.  A new type pushes the OLDEST one
out, and it tells you which one it dropped.  Hit an adaptation
with its own type again and it HARDENS instead.

So spamming your best move is the losing line.  Alternating two
moves is also the losing line.  The fight wants BREADTH - more
distinct attacking types than it has slots.
```

Only **one** filing happens per turn, no matter how many times a multi-hit move connected. A move
that missed, was blocked or did nothing at all teaches it nothing.

### Reading the board

Registeel never shows you a number. It narrates the board every single time it moves:

| The line | What just happened |
| --- | --- |
| *…plating reshapes itself against `<type>`-type attacks!* | That type is now filed. It resists it |
| *Registeel has already solved `<type>`. Its plating thickens further.* | You hit a filed type again — it **hardened**, and now resists it harder |
| *Registeel discards its `<A>` configuration to make room for `<B>`.* | The board was **full**. The oldest adaptation fell off |

That third line is the whole tutorial. The board has a **size**, and you are the one deciding what
falls off it.

While it is holding adaptations it also says so, once per tier, and again whenever the tier changes:

> *Registeel is holding a configuration against you.* → 1–2 filed
> *Registeel has an answer for almost everything you've shown it.* → 3–4 filed

### The dial: plating versus weapons

Everything Registeel puts into plating comes out of its weapons. It states the trade out loud every
time it changes:

| Filed | Stance | Its Attack / Sp. Atk | The line |
| --- | --- | --- | --- |
| **0** | **ASSAULT** | **+2** | *Registeel routes everything into its weapons!* |
| 1–2 | BALANCED | +1 | *Registeel rebalances itself.* |
| **3–4** | **FORTRESS** | **−1** | *Registeel routes everything into its plating.* |

This is the answer to *"what if I only have two attacking types?"* — **letting it wall you also
disarms it.** A full board is the long, safe fight; an empty board is the short, dangerous one.
Both are legitimate. Choose deliberately.

Its own Attack and Sp. Atk are governed entirely by this dial. **Clear Body** refuses everything you
throw at those stats — Intimidate, Growl, Charm, Parting Shot — so you cannot shortcut either end
of the trade.

### Two things open the board back up

**Status.** Any non-volatile status landing on Registeel makes it purge its **newest** adaptation —
the freshest one, the one you just watched it file:

> *Registeel's readings contradict each other!*
> *Its `<type>` configuration fails.*

This works **from turn one**, not just late. It is **once per application**, not once per turn: a
burn that is already there does nothing further. Getting a second purge costs you a second
application — another turn, another accuracy roll. Registeel is Steel, so poison and Toxic are off
the table; **burn, paralysis, sleep and freeze** are your levers.

Sleeping it is legal and the fight does not refuse it. It also does not accomplish much — a sleeping
Registeel still holds its whole board, the decay clock runs at the same speed, and you spent the
turn you could have spent rotating a type through.

**The decay clock.** Every fourth turn (every sixth after it optimizes), the **oldest** adaptation
expires on its own:

> *Registeel's `<type>` configuration has degraded.*

This is why a two-type attacker is never hard-locked. Wait, and the board opens. Waiting costs
turns, and turns are what Registeel is spending on Iron Head.

### Phase 2 — Optimization (55% HP)

> *Registeel's processing accelerates.*
> *It's filing far more than it was.*

Three slots at 50%, and the decay **slows down** to every sixth turn. This is the first phase where
a normal four-move Pokémon can be comprehensively answered — and the first where the FORTRESS rung
is easy to reach.

### Phase 3 — Perfect Configuration (25% HP)

> *PERFECT CONFIGURATION!*
> *Registeel stops choosing between its plating and its weapons.*

Four slots at 55%, **nothing decays**, and the stance is pinned to **ASSAULT** regardless of how
much it is holding. For the first time it has the wall *and* the weapons at once, and status is the
only thing that opens the board at all — which is what makes the lever worth having carried for
twenty turns.

A **fixed five-turn clock** is now running, and nothing you do speeds it up or slows it down. This
is a siege to survive, not a window to farm.

### CRITICAL OVERHEAT

> *CRITICAL OVERHEAT!*
> *Registeel's configurations burn away, and it can't file anything new.*

The clock lands and everything goes at once. **The whole board is wiped**, Analysis goes offline for
**three full turns**, and its flat damage reduction — which has not moved once all fight — collapses
from 84% to 60%. Every type you own is doing well over twice its usual damage, and there is nothing
it can do about it.

You are told, every turn, that the window is still open:

> *Registeel's systems are still cooling.*

Then it closes, and the clock re-arms:

> *Registeel's systems come back online.*

The re-arm is a **safety valve, not a rhythm** — it exists only so a player who cannot close the
last stretch inside one window is not deadlocked against a boss holding a permanent full board. If
you are still fighting on the second overheat, the first one went badly.

### Building for it

Registeel is **80 / 75 / 150 / 75 / 150 / 50**, pure Steel — the only Regi with *identical* offences
and *identical* defences. No category is the safe one. That symmetry is the machine the fight wants.

**Bring breadth, not power.** More distinct attacking types than it has slots is the whole plan.
Four different attacking types on one Pokémon beats one enormous move; a team with four different
attackers beats that. Coverage moves you would normally never click are the point here.

**Attacking into an adaptation is a bad choice, never an impossibility.** A filed type does roughly
half what an unfiled one does, and a hardened one roughly a third. If you are down to one usable
attacking type you can still finish — slowly, while it sits at −1 offence and lets you.

**Its moveset punishes the obvious answers:**

| Move | Why it's there |
| --- | --- |
| **Iron Head** | Physical Steel STAB with a 30% flinch — and flinching matters enormously against a boss you are trying to outlast |
| **Flash Cannon** | Special Steel STAB. Same type, other category — one defensive wall does not hold |
| **Earthquake** | **Ground is super effective on Fire.** Fire is Steel's headline weakness, so the obvious lead into Registeel is exactly what this punishes. It also catches Electric and Rock answers |
| **Thunderbolt** | Covers the Water and Flying resists that Ground and Steel both miss |

**Status is a tool here, not a trap.** Registeel has no status immunity at all — deliberately,
because Force Reconfiguration is the fight's main counterplay. A move that inflicts burn or
paralysis earns its slot on your team for this fight even if it does no damage.

### Catching it

You cannot throw a Poké Ball for almost the entire fight. The window opens only in the final phase,
at **10% HP or less**, and Registeel tells you when:

> *Registeel's core dims, and its plating goes slack.*
> *It's worn out — now is the moment to catch it!*

Everything releases at once: the board is wiped, its stances reset to neutral, every clock stops,
the damage reduction and the type-effectiveness cap come off, and Poké Balls are unlocked. From that
point the game keeps the catch target alive for you — you cannot accidentally KO it out of the
window.
