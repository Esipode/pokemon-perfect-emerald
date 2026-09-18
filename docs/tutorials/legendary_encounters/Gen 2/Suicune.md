## Suicune — The Purifier

### Location: Route 103 - Saturday

Suicune fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a
2x cap on incoming type effectiveness, and flat Toxic damage. It can't be caught until its final,
weakened phase. What makes this fight different: **everything you put on the battlefield is fuel for
Suicune, and it will come and collect it.**

Raikou is a number you fight downward. Entei is a bomb timer you cannot defuse. Suicune is a
resource you are constantly tempted to hand it. It tracks a hidden **Flow** from 0 to 5, and Flow is
fed by exactly one thing: **Suicune finding something on the battlefield to wash away.** Not by
attacking, not by the clock, not by you playing badly. Only by cleaning up after you.

So the whole fight is one question, asked over and over: *is this effect worth giving Suicune power?*

### Flow — the meter you feed

Every few turns Suicune performs a **PURIFICATION** and sweeps the field. Each separate thing it
finds and removes is **+1 Flow** (capped at 5):

| Impurity | What it is |
| --- | --- |
| Weather | Rain, sun, sand, snow — yours or anyone's |
| Terrain | Any of the four terrains |
| Screens | Reflect, Light Screen, Aurora Veil, Safeguard, Mist, Tailwind, Lucky Chant, on either side |
| Entry hazards | Spikes, Stealth Rock, Toxic Spikes, Sticky Web, Steelsurge, on either side |
| Substitute | Anyone's |
| Status on Suicune | Any status you managed to land on it |

Hazards and Substitutes are swept together and count as **one** point between them however many are
out, so a sweep pays at most 5.

**Stat stages are not on that list, on either side.** Boost your own Pokémon as freely as you like —
Suicune neither gains Flow from it nor Hazes it away. The impurity list is *board state*, not your
build-up.

A single sweep that finds four of these pays Suicune four points of Flow at once.

The other direction is slower and quieter: **a turn that purifies nothing and ends with a completely
clean field drains 1 Flow.** That's it. That's the only way it comes down. Once you've held a clean
field for a couple of turns you'll see the standing line *"The water around Suicune is perfectly
still"* — that's the cue you're winning the resource war.

Higher Flow means Suicune is harder to hurt (*"The current thickens"*) and mends more each turn.
Lower Flow means the opposite (*"The current slackens"*). Those two lines re-fire every time the
number behind them actually moves, so you can read the whole meter off the dialogue.

### Why playing clean isn't free

The obvious answer — never touch the field, starve it to 0 — is correct, and the fight is built to
make it cost you. Suicune's kit is chosen to create problems you'd normally solve with exactly the
tools that feed it:

- **Scald** burns. A burn is the sort of thing you'd answer with a screen or a Safeguard — and both
  of those are impurities.
- **Bulldoze** grinds your Speed down turn after turn, and you lose initiative over a long fight.
  Boosting it back is free, but it costs you the turn, and the undertow below is doing the same
  thing from the other direction.
- **Ice Beam** punishes the Grass answer; **Extrasensory** answers the Poison and Fighting bodies
  you'd otherwise park in front of a Water attacker.

You are squeezed between taking the chip and paying for the cure. That's the fight.

From **The Cleansing** (50% HP) on, every sweep also drags a stage of Speed off your active Pokémon
(*"The undertow drags at your POKéMON!"*). These **stack** — two sweeps is -2, three is -3 — and
nothing in the fight resets them for you. It's Suicune's only hold on your stat line, and the answer
is simply to spend turns undoing it or to out-pace the sweeps.

### The healing, and how to shut it off

From **The Cleansing** (50% HP) onward Suicune starts putting health back on, and this is where most
runs stall out. There are two separate heals, and the important thing is that **both of them are
gated on Flow**. Flow isn't just the guard meter — it's the healing meter too, and draining it turns
off both at once.

**1. Per-turn mending.** At the end of every turn, if Flow is high enough, Suicune mends:

| Flow | Above 50% HP | 50%–20% HP |
| --- | --- | --- |
| 0 | nothing | nothing |
| 1 | nothing | 1% |
| 2 | 1% | 1% |
| 3 | 1% | 3% |
| 4 | 3% | 3% |
| 5 | 3% | 3% |

Notice the whole column shifts one rung left when it drops below 50% — the same Flow that cost it
nothing in the first phase now pays. But the top row doesn't move: **at Flow 0 it mends nothing, in
either phase.** If Suicune is out-healing your damage, your Flow is too high. That's the entire
diagnosis.

**2. Sacred Water** — a **10%** burst. This one has a hard gate: it fires **only at Flow 5**. Not
"more likely at high Flow" — it is literally impossible at Flow 4 or below. So there are two ways to
deal with it, and the first one is much cheaper.

#### Answer A: never let it reach Flow 5

Flow 5 is a wall you build for it. Every impurity you leave lying around is a brick. Keep it at 4 or
lower and Sacred Water simply never happens, all fight. This is the answer you should be aiming for.

#### Answer B: break the draw — and the timing is tight

If Flow does hit 5, Suicune commits. Read this sequence carefully, because the window is not where
it looks:

- **Turn N, before anyone moves** — Suicune sweeps the field clean, *then* announces
  *"Suicune draws upon the purest water!"* The sweep comes first on purpose: whatever you had down
  is already gone, so anything on the field afterwards is unambiguously something you just did.
- **Turn N is your window.** Your action on this turn is what the check will see. There is no later
  chance.
- **Turn N+1, before anyone moves** — it checks the field and resolves.

That middle point is the one that catches people out. The draw resolves at the *start* of turn N+1,
**before you get to act that turn**. If you wait until N+1 to respond, it has already healed. You
have to dirty the field on the same turn the telegraph prints.

What counts as dirt is the impurity list above: a screen, a hazard, a Substitute, a weather or
terrain move, or a status landed on Suicune. **A stat boost will not do it** — stat stages aren't
impurities, so Swords Dance on this turn is a wasted turn. Substitute and a one-turn weather move are
usually the cheapest, since neither needs to connect. And you're already paying the maximum guard at
Flow 5, so dirtying the field that turn costs you nothing you weren't already paying.

Get it right and:

> *The water clouds over — Suicune's concentration is broken!*

No heal, its guard falls to **76%** for that turn, and **Flow crashes from 5 to 2** — which also
knocks the per-turn mending back down to 1% and puts Sacred Water four sweeps away instead of one.
It can't draw again while it's broken, and it rests a further turn after any draw resolves before it
can start another.

The catch is that the impurity you just used is still sitting there, and the next sweep will collect
it. Breaking the draw doesn't come free — it sells you a turn of damage and a Flow reset for one
point of Flow back. That's a good trade, but it is a trade, and it's why Answer A is the better
habit.

### Sacred Beast — the door that only opens from empty

At **20% HP** Suicune stops attacking for a moment and stands perfectly still. No roar, no
animation. *"Suicune has reached perfect purity."*

From here:

- It **purifies at the end of every single turn**, not on a timer.
- It **sheds any status instantly**, at both the start and the end of every turn.
- It mends 2% every turn regardless of Flow.
- Its guard climbs to **95%** at high Flow. You are not getting through that.

But the guard is a ladder, not a wall, and it runs the other way too. At **Flow 0**, Sacred Beast's
guard is **80%** — the softest un-broken state anywhere in the fight. Draining Flow is real progress
on its own.

And at Flow 0, the door opens. Hit Suicune with a **Grass or Electric move**, or land a **critical
hit** of any kind, and:

> *Suicune's purity has been disrupted!*

Its guard collapses to **72%** for **two full turns**, and Flow is knocked to 0. That's your damage
window, and you can do it again — the shatter is repeatable, once per turn, as long as you can keep
dragging Flow back to empty.

The puzzle of the final phase is therefore: **starve it, then break it.** Bringing the counter-type
isn't the answer by itself; getting to Flow 0 so the counter-type *works* is.

### What does and doesn't stick

- **Suicune has no blanket status immunity until Sacred Beast** — but statusing it is close to
  self-defeating. A status on Suicune is an impurity, so the next purification cures it **and pays
  Suicune a point of Flow for the privilege.** From Flow 4 it sheds status at the top of every turn
  on its own; from Sacred Beast it sheds at both ends of every turn.
- **Stat stages are yours to keep.** Boosting up costs no Flow and the sweep won't Haze it off, on
  either side — so setup is a real strategy here, unlike almost everything else you might reach for.
  Debuffing Suicune is safe for the same reason. The counterweight is the undertow and Bulldoze
  grinding your Speed down, which nothing resets for you.
- **Toxic is bounded** (flat damage, no ramp) and OHKO/fixed-damage moves are shut out entirely.
- **Grass and Electric are its weaknesses**, and both are exactly what its own moveset is built to
  punish — Ice Beam for the Grass, Bulldoze for the Electric. Bring them anyway; you need them for
  the shatter.
- **Weather and terrain are traps in this fight specifically.** They're strong tools in most
  legendary fights and pure Flow here.

### Phases and catching it

1. **The Cleansing Current** (above 50% HP) — Purification every 3 turns. Guard runs **84%** at
   Flow 0 up to **93%** at Flow 5. Mends 1% from Flow 2, 3% from Flow 4. No Sacred Water yet.
2. **The Cleansing** (50% HP or below) — the rite comes every 2 turns instead of 3, and every sweep
   also takes a stage of your Speed. Guard **86%** to **94%**. The mending thresholds each drop a
   rung (1% from Flow 1, 3% from Flow 3), and **Sacred Water becomes available**. See
   [The healing, and how to shut it off](#the-healing-and-how-to-shut-it-off) — this is the phase
   most runs stall in.
3. **Sacred Beast** (20% HP or below) — purification at the end of every turn, instant status
   shedding, and a flat 2% mending every turn that Flow can no longer switch off. Guard **95%** at
   Flow 5 but **80% at Flow 0**, and shatterable from there. Sacred Water still fires at Flow 5.
   Suicune cannot be knocked out during this phase, so the catch window is guaranteed to open.
4. **Weakened** (10% HP or below) — the water falls away. Every guard drops to 78%, every mechanic
   switches off, and **Poké Balls work**. The only catch window. Its catch rate is generous here, and
   the catch-phase safety net (see [Common to every legendary](#common-to-every-legendary)) keeps
   Suicune alive through your hits.

---
