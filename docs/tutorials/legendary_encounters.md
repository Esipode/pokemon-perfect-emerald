# Legendary Encounters

A player-facing reference for scripted legendary fights — what each one's mechanics actually do,
for anyone who wants to understand a fight rather than just muscle through it. Every legendary
encounter gets its own section here once it's implemented.

---

## Common to every legendary

- **One-hit-KO and fixed-damage moves are shut out** (Sheer Cold, Seismic Toss, Endeavor, and so
  on), incoming type effectiveness is capped at a double weakness, and Toxic damage stays flat
  instead of ramping. These fights are meant to be won on skill, not on one lucky roll.
- **Poké Balls stay blocked until the fight's final, weakened phase.** Each encounter's section
  says exactly when.
- **Once Poké Balls are unlocked, the legendary takes almost no damage** — a safety net so a
  stray hit, a status tick, or your Pokémon's ability can't knock out the one you're trying to
  catch. The net drops the moment the legendary recovers any HP (it healed, so it's no longer on
  the brink), and comes back once it's low again. If a legendary in its catch phase keeps healing
  itself, you have to answer the heal before you can safely wear it back down.
- **A legendary only leaves for good once you own it.** Knock it out or run away and it's still
  standing there for another try. Catch it and it's gone — but release or trade it away and it
  reappears where you found it, ready to be caught again. This applies to every scripted
  legendary in the game, not just the ones documented below.

---

## Articuno — The Frozen Battlefield

Articuno fights behind a heavy damage reduction, a full set of one-hit-KO/fixed-damage immunities,
a cap on incoming type effectiveness (its Ice/Flying typing is quad-weak to Rock, but a Rock move
can't exceed a double-weakness hit), and flat Toxic damage (no stacking it up over a long fight)
from the opening bell, and it can't be caught until it enters its final, weakened phase. Its whole
fight is built around one resource: **Frost**.

### The Frost loop

- Every Ice-type move Articuno uses raises Frost by 1 (caps at 4).
- Every Fire-type move you use — hit or miss, damaging or status — lowers Frost by 1. This is
  always available and never fails, whatever phase the fight is in (unless Frost is locked; see
  Absolute Zero below).
- Once the fight reaches its second phase, Articuno's **Ice Barrier** regrows every turn Frost is
  1 or higher. How much it blunts your hits scales with Frost: at low Frost it's thin and brittle
  and barely better than its normal guard, at high Frost it cuts incoming damage to a trickle.
  Holding Frost down with Fire moves is what keeps the barrier weak. The only way to bring it down
  outright is to land a hit while it's up — doing so shatters it and unleashes an icy shockwave
  that hits your whole team, scaled by how high Frost had climbed. When Frost locks (it hits 4, or
  Absolute Zero begins) it freezes at its current level — the barrier is then sealed at whatever
  strength you left it and keeps regrowing at that strength for the rest of the fight.

| Frost | Effect | Barrier when up | Shockwave on shatter |
| --- | --- | --- | --- |
| 0 | — | (doesn't form) | 3% max HP |
| 1 | Snow begins falling | thin — barely helps | 6% max HP |
| 2 | Your team's Speed drops a stage | thin | 9% max HP |
| 3 | The ground turns to sheet ice — switching in costs HP (see below) | solid | 12% max HP |
| 4 | Articuno enters its **Frozen Domain**: its Sp. Atk rises and its guard hardens further | full | 15% max HP |

Letting Frost climb makes Articuno hit harder and puts more of your team at risk from the next
shatter — but spending a turn on a Fire move to bring it down is a turn not spent attacking. That
trade-off is the whole fight.

Switching in a new Pokémon while Frost is 3 or higher costs it HP immediately, and if the Speed
drop from Frost 2 is still active, the newcomer gets it too (stat drops don't carry over on a
switch on their own — Articuno re-applies it).

### Phases

1. **Opening** — no barrier yet. This is the window to learn the Frost rules before they start
   punishing mistakes.
2. **Frozen Battlefield** (Articuno at 75% HP or below) — the barrier loop switches on. Keep Frost
   low here and the barrier stays thin; let it climb and the barrier becomes near-impenetrable.
3. **Absolute Zero** (50% HP or below) — permanent snow, a rise in evasion, Sp. Defense and
   Defense, and Frost gets locked in place: **Fire moves no longer lower it.** The Ice Barrier is
   sealed at whatever strength your Frost level supports at this moment — enter Absolute Zero with
   Frost low and the barrier stays weak (or never forms) for the rest of the fight; enter with it
   high and you're stuck with a strong barrier every turn. Fire now instead answers a recurring
   "the temperature plummets" warning — ignore it and your team takes a heavy hit at the end of
   the following turn; answer it and Articuno takes a small bite of its own instead.
4. **Weakened** (10% HP or below, while in Absolute Zero) — Articuno's own guard drops and
   **Poké Balls work again.** This is the only point in the fight you can catch it. From here the
   catch-phase safety net (see [Common to every legendary](#common-to-every-legendary)) keeps
   Articuno alive through stray hits — but Articuno can still recover HP off its own effects, and
   any recovery drops the net until you bring it back down.

### What does and doesn't stick

Articuno shrugs off sleep the instant its turn starts — sleep-locking it doesn't work. Burn,
poison, and paralysis all stick normally, and burning it is thematically appropriate given what
lowers its Frost.

### Catching it

Poké Balls are blocked for the entire fight until Articuno visibly falters in the Weakened phase.
Once you see that message, its catch rate is generous — that's the intended moment to throw. You
don't need to be gentle with it here; the catch-phase safety net absorbs your hits. The one thing
to watch for is Articuno healing itself back up, which switches the net off until it's low again.

---

## Zapdos — Storm Overload

Zapdos fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a
2x cap on incoming type effectiveness, and flat Toxic damage, and it can't be caught until its
final phase. Everything about the fight hangs off one hidden number: **Charge**, from 0 to 4.

### The Charge loop

Charge **rises** when:

- Zapdos uses an Electric move.
- One of *your* Electric moves hits it (you're told the first time — feeding it is not a bug).
- Any other move hits it — a 1-in-4 chance each time.
- Every third turn, on its own.

Charge **falls** when:

- You hit it with a **Ground move** — that drops Charge by **2**. Ground can't damage Zapdos (it's
  part Flying), so a discharge turn is a turn you deal no damage — that's the price of the lever.
- It shakes off sleep or freeze (drops Charge by 1 — see below).

What Charge controls:

| Charge | Storm | What it does |
| --- | --- | --- |
| 0 | Calm | Base guard. |
| 1–2 | Charged | Guard tightens slightly; Zapdos's Sp. Atk climbs a stage per Charge. |
| 3 | Severe Storm | Permanent **rain** (Thunder never misses), guard tightens further, Sp. Atk still climbing, and Zapdos starts calling **lightning strikes** (below). |
| 4 | **Overload** | See below. |

There's no number on screen for the guard, so watch the messages: Zapdos's guard is called out
every time it hardens or slackens, in either direction.

### Overload and Burnout

At Charge 4 Zapdos **overloads**: a massive Sp. Atk and Speed spike and a team-wide lightning bolt
at the end of *every* turn for three turns — but its guard drops to its softest of the whole fight.
Then it
**burns out**: Charge crashes to 0, it damages itself, and for one turn its guard collapses
completely. That crash window is the best damage opportunity in the fight.

So there's a real choice: let Charge climb and ride the overload down (fast, dangerous), or hold
Charge at 0 with Ground moves (safe, slow). And if Zapdos is already overloaded, a **Ground move
forces an immediate discharge** — the remaining strikes are cancelled and the burnout crash is
pulled forward to next turn. Spending one turn to skip two team-wide bolts and reach the soft
window early is the strongest play available.

### Lightning strikes

While the storm is Severe (Charge 3) and Zapdos isn't overloaded, it **marks** one of your
Pokémon. The bolt lands at the **start of the turn after next** — you get a full turn of warning
("the air above your Pokémon crackles"). Two ways to answer it:

- **Switch** the marked Pokémon out (while Charge is 3+ the incoming Pokémon takes a smaller toll,
  but far less than the strike).
- **Discharge** with a Ground move — earthing the charge also grounds the bolt.

Do nothing and the strike hits your whole team hard.

### What does and doesn't stick

- **Paralysis does nothing** — Zapdos is an Electric type.
- **Sleep and freeze work, but only for one turn.** Zapdos shakes either off at the start of its
  turn — and the jolt costs it a Charge, exactly like a Ground move would. It's a legitimate second
  discharge lever, just not a lock.
- **Burn, poison, and confusion all stick normally.** Burn is a solid answer to Drill Peck.

### Phases

1. **Storm cycle** — the loop above runs freely: charge, overload, burn out, repeat.
2. **Last Stand** (25% HP or below) — Zapdos locks into **permanent overload**: a strike every
   turn, no burnout, and its guard hardens back up. There's no soft window here — this phase is a
   straight damage race.
3. **Weakened** (10% HP or below, during Last Stand) — the storm dies, every guard and immunity
   drops, and **Poké Balls work**. The only catch window in the fight. The catch-phase safety net
   (see [Common to every legendary](#common-to-every-legendary)) keeps Zapdos alive through your
   hits from here.

---

## Moltres — The Everlasting Flame

Moltres fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a
2x cap on incoming type effectiveness (its Fire/Flying typing is quad-weak to Rock, but a Rock move
can't exceed a double-weakness hit), and flat Toxic damage. It can't be caught until its final,
weakened phase. The whole fight is one escalating number: **Flame Intensity**, from 0 to 5.

The inversion to keep in mind: as Intensity climbs, Moltres hits **harder** but its guard gets
**weaker**. Hurting it feeds it; the only way to cool it is to stop hurting it — which costs you the
turn. And past a point, even that stops working.

### The Intensity loop

Intensity **rises** when:

- Any damaging move hits Moltres.
- A **Fire-type move** hits it — and this also **heals Moltres** and does *not* count as hurting it.
  Attacking it with Fire is strictly worse than doing nothing.
- Moltres uses a Fire-type move.
- Moltres sheds sleep or freeze (see below) — trying to status-lock it stokes the fire.

Intensity **falls** by 1 at the end of a turn **only if Moltres took no damage that turn** — and
only while Intensity is 3 or lower. There is no move that cools it; withholding damage is the sole
lever.

Watch the messages: Moltres's guard is called out every time it slackens or hardens, and while the
heat is up a recurring line reminds you it hasn't let up.

| Intensity | Guard | State |
| --- | --- | --- |
| 0 | 90% | Decays at end of turn if Moltres took no damage. |
| 1 | 87% | Embers. |
| 2 | 84% | Moltres's Fire attacks are empowered (its Sp. Atk climbs a stage per Intensity). |
| 3 | 79% | **Flame aura** — every move that damages Moltres burns its user for a chunk of HP. The ground starts burning (see below). |
| 4 | 72% | **Battlefield engulfed** — permanent harsh sunlight, a bigger field burn, Moltres consumes 5% of its own HP every turn, and **Intensity never falls again**. |
| 5 | 64% | **Everlasting Flame** — only reachable after Rebirth. |

### The field burn

At Intensity 3+, your active Pokémon takes end-of-turn chip damage from the burning ground (4% / 6%
/ 8% max HP at Intensity 3 / 4 / 5). **Fire-type Pokémon are immune to it** — bringing one is a real,
if small, reward for reading the fight.

### The Intensity-4 sun

Permanent harsh sunlight halves your Water damage and boosts Moltres's Fire output — a genuine
punishment for letting the fire reach 4. Rock and Electric answers are untouched. The sun also makes
**Solar Beam a one-turn move for Moltres** (no charge turn) — which is your clearest signal that the
rules have changed. Below Intensity 4, Solar Beam charges for a turn and gives you a free window.

### Rebirth

The first time Moltres would be knocked out, it isn't. It falls, the battle goes quiet, and it
**rises from its own ashes** — once. After Rebirth:

- HP is restored to ~55%.
- Intensity resets to 2 and **never drops below 2 again**.
- The **flame aura is permanent** (every damaging hit burns its user, regardless of Intensity).
- **Harsh sunlight is permanent** from the moment it rises.
- Moltres's Speed rises a stage.

There is no third life. The second death is real.

### What does and doesn't stick

- **Burn does nothing** — Moltres is a Fire type.
- **Sleep and freeze work for exactly one turn.** Moltres sheds either at the start of its turn —
  and the shed *raises* Intensity. Locking it down actively makes the fight worse.
- **Paralysis and poison stick normally.** Paralysis is your legitimate answer to Moltres's Speed.
- At Intensity 4+, the encounter's own sun also blocks fresh freezes.

### Phases and catching it

1. **First life** — Intensity 0–4, the loop above.
2. **Reborn** — Intensity 2–5, aura and sun permanent, more aggressive.
3. **Weakened** (10% HP or below, after Rebirth) — the everlasting flame dims, every guard and
   immunity drops, and **Poké Balls work**. The only catch window in the fight. Its catch rate is
   generous here, and the catch-phase safety net (see
   [Common to every legendary](#common-to-every-legendary)) keeps Moltres alive through your hits.
