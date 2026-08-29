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

---

## Mewtwo — The Perfect Weapon

Mewtwo fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a 2x
cap on incoming type effectiveness, and flat Toxic damage. It can't be caught until its final,
weakened phase. What makes this fight different: **Mewtwo rebuilds its body to match how you fight**,
and the only way through is to keep changing what you do.

The trap is the obvious play. Find your best move, use it every turn, and Mewtwo settles into the
form that answers it — permanently. The counter to the adaptation and the way to break its guard are
the *same action*.

### The adaptation

Mewtwo keeps a hidden tally of your last few turns. Physical moves push it one way, special and
status moves push it the other. It spends the first three turns doing nothing but watching, then
transforms — and re-evaluates on a cooldown from then on.

| What you've been doing | What it becomes | Why that's bad for you |
| --- | --- | --- |
| Mostly **physical** | **Mega Mewtwo X** — Psychic/Fighting | Attack 190 and a higher Defense; Psycho Cut and Brick Break are both STAB. Brick Break also shatters Reflect and Light Screen, so screening up doesn't save you. |
| Mostly **special or status** | **Mega Mewtwo Y** — Psychic | Sp. Atk 194 and a much higher Sp. Defense. Psystrike is special but hits your **physical** Defense, so a special wall doesn't save you either. |

It mirrors you rather than opposing you, because for Mewtwo those are the same thing — the stat
spread does the work.

Each Mega has a real soft spot, so baiting a form and then punishing it is legitimate play:

- **Mega Y's Defense is 70**, lower than base Mewtwo's. Bait it into Y, then hit it physically.
- **Mega X gives up 40 Sp. Atk** and picks up Fighting, so Fairy, Flying and Psychic moves suddenly
  bite.

Doing either of those pushes the tally back the other way, which is the point.

### Instability, and the window it opens

Every transformation costs Mewtwo something. So does shrugging off sleep or freeze. Watch its guard:

| State | Guard | The line that tells you |
| --- | --- | --- |
| Settled | 92% | "Mewtwo reassembles itself. Its guard is whole again." |
| Strained | 86% | "Mewtwo's outline flickers." |
| Badly strained | 82% | "Mewtwo's form ripples violently…" |
| **Overloaded** | **78%** | "MEWTWO'S GENETIC STRUCTURE ERUPTS!" |

After the **third** point of strain its body gives out. It collapses back to base Mewtwo, tears 8% of
its own HP off in the process, and its guard lowers to 78% for **two full turns**. That is your
damage window, and forcing more of them is how you win the fight.

A recurring line runs the whole time telling you whether Mewtwo is settled or straining — that's your
read on how close the next Overload is.

### Force and Focus

Whichever Mega is active builds a meter every time its attack lands on you. At three, it spends it:

- **Mega X — Annihilation.** 15% of your Pokémon's max HP, and Mewtwo's Attack rises permanently.
  Let this happen repeatedly and X gets genuinely lethal.
- **Mega Y — Focus.** 8% of max HP, plus your Sp. Atk and Speed each drop a stage. Y also builds the
  meter when you spend a turn on a **status move**, so setting up in front of Y is expensive.

**The meter resets to zero on every transformation.** Forcing a form change is how you deny the
punish — the same lever that generates Instability.

### What does and doesn't stick

- **Sleep and freeze last exactly one turn.** Mewtwo shrugs either off at the start of its turn — but
  the shrug costs it a point of strain, which is a third of an Overload window. Unlike the birds,
  status is a slow but genuinely *good* line of play here.
- **Mega Y's ability is Insomnia**, so sleep simply won't land while it's in Y form. Baiting it into
  X first *is* the setup for a sleep play.
- **Paralysis, burn and poison all stick normally.**
- Mewtwo's ability changes with its form — Pressure, then Steadfast as X, then Insomnia as Y. The
  ability popup is a free readout of which form it's in.

### Phases and catching it

1. **Analysis** (turns 1–3) — base Mewtwo. It isn't adapting yet, but it *is* counting.
2. **Adaptation** — it transforms on a three-turn cooldown, driven by your tally.
3. **Perfect Adaptation** (25% HP or below) — its Attack and Sp. Atk both climb, the cooldown drops to
   a single turn, and it **can no longer stop transforming** — it flips every turn whether your tally
   asked for it or not. Every flip is another point of strain, so Overload windows start arriving on
   their own. The thing that made it unbeatable is what finishes it.
4. **Weakened** (10% HP or below) — it reverts to base Mewtwo, every guard and immunity drops, and
   **Poké Balls work**. The only catch window in the fight. Its catch rate is generous here, and the
   catch-phase safety net (see [Common to every legendary](#common-to-every-legendary)) keeps Mewtwo
   alive through your hits. You always catch a plain **Mewtwo**, never a Mega.

---

## Mew — The Genetic Wonder

Mew fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a 2x cap
on incoming type effectiveness, and flat Toxic damage. It can't be caught until its final, weakened
phase. What makes this fight different: **Mew punishes a habit, not a resource.** It is not draining
a meter or building toward a burst — it is watching *how* you play, and it rewards you for keeping it
interested.

The trap is the obvious play. Find your best move and use it every turn. Mew gets bored, and a bored
Mew hardens back up and starts making trouble.

### Wonder — the guard that runs backwards

Every time you do something **different in kind** from the turn before, Mew leans in to watch and
forgets to defend itself. "Different in kind" means:

- a move of a **different category** than your last move (physical → special, special → status, …), or
- **switching Pokémon**.

The more it watches, the less it guards:

| How absorbed Mew is | Guard | The line that tells you |
| --- | --- | --- |
| Just watching | 92% | "Mew drifts back out of reach, watching politely." |
| Leaning in | 88% | "Mew floats in closer. It wants a better look!" |
| Not defending | 84% | "Mew has stopped shielding itself…" |
| **Completely absorbed** | **76%** | "MEW IS COMPLETELY ABSORBED IN YOU!" |

The ladder moves **both ways**, and a recurring line tells you whether Mew is still watching closely
or has drifted off — that's your read on your own damage.

### Boredom — why spam goes backwards

Repeat a move category and Mew's **boredom** climbs. You'll get one turn's warning ("Mew's attention
starts to drift…"). At the third repeat, Mew does one **Mischief** roll and its absorption drops a
step — so spamming doesn't just stall, it actively rebuilds the guard you were tearing down.

### Mischief — the chaos table

When Mew gets bored (and every turn once Genesis begins) it rolls one of these. You learn the *set*,
never the order:

| Roll | What Mew does |
| --- | --- |
| **Copycat** | Copies your fighting spirit — its Attack and Sp. Atk each rise a stage. |
| **Whim** | Rearranges the sky — random weather (sun, rain, or sandstorm). |
| **Recovery** | Heals itself ~10% of its max HP. |
| **Tag** | Tags your side for ~8% and drops your Speed a stage. |
| **Generosity** | Heals *your* active Pokémon ~15% and clears the weather. Mew isn't malicious. |
| **Transform** | (Playtime onward, replaces some Copycat rolls) — see below. |

### Transform — it fights you with your own team

From **Playtime** (60% HP) on, Mew takes on the **shape of your active Pokémon** — its stats, stat
changes, types, ability and moves. It keeps its own HP bar, and it keeps the name MEW on the
healthbox (that's your tell). It re-copies every time you switch, cheerfully following you. Whatever
you brought is what you fight.

Because it wears your Pokémon's typing, a 4x weakness on your own mon becomes a 2x weakness Mew has
to carry — the type-effectiveness cap follows it there.

### "Show me something new!"

At **30% HP** Mew stops attacking for one turn. Its offense collapses ("Mew is waiting…") and it asks
to be *shown* something — not hit harder.

- **Answer it** — spend that turn on a **status move** or a **switch** — and Mew is *delighted*: it
  heals your active Pokémon ~30%, and it stays fully absorbed (guard 68) for the rest of the fight.
- **Ignore it** — just attack, or do nothing — and Mew is *disappointed*: it chips your side ~6%
  every turn for the rest of the fight.

Either way the fight then enters **Genesis**: Mew's raw Attack and Sp. Atk rise 20%, and it rolls
Mischief every single turn.

### What does and doesn't stick

- **Everything works.** Unlike the birds and Mewtwo, Mew does **not** shrug off sleep, freeze or
  paralysis — they land and they hold.
- But a status-stall *is* repetition. Boredom climbs, Mischief fires, and one of the rolls is
  Recovery — a stalled Mew out-heals a stalled player. Status is a fine tool if you keep varying
  what you do around it; it's a treadmill if you don't.

### Phases and catching it

1. **Curious** (above 60% HP) — the guard ladder is live, the first time Mew sees each kind of
   action it reacts to it, and Mischief only fires on boredom.
2. **Playtime** (60% HP or below) — Mew transforms into your active Pokémon and re-copies on every
   switch; Transform joins the Mischief table.
3. **Genesis** (30% HP or below) — the "show me something" turn resolves, then raw offense +20% and
   Mischief every turn.
4. **Weakened** (10% HP or below) — Mew reverts to its own shape, every guard drops, and **Poké Balls
   work**. The only catch window. Its catch rate is generous here, and the catch-phase safety net
   (see [Common to every legendary](#common-to-every-legendary)) keeps Mew alive through your hits.
   You always catch a plain **Mew**, whatever shape it was wearing.

---

## Raikou — The Hunting Thunder

Raikou fights behind a heavy damage reduction, the standard one-hit-KO/fixed-damage immunities, a 2x
cap on incoming type effectiveness, and flat Toxic damage. It can't be caught until its final,
weakened phase. What makes this fight different: **Raikou's guard is tied to its tempo, and its
guard runs *upward*.** The faster it is moving, the harder it is to hit. Let it build momentum and
you will be chipping at a wall.

The whole fight is one sentence: *never let it get going.* The counter-tension is that every turn you
spend slowing it down is a turn you aren't spending on its HP.

### Velocity — the number that runs everything

Raikou tracks a hidden **Velocity** from 0 to 5. It is a *tempo* resource, not an elemental one: it
is fed by getting to act first, and drained by anything that takes the initiative away.

| Velocity goes **up** when | Velocity goes **down** when |
| --- | --- |
| Raikou moved before you this turn (+1) | Your **Ground** move hits it (−2; −1 once the Hunt begins) |
| Raikou knocked out one of your Pokémon (+2) | Raikou **never got to act** at all this turn (−2) |
| You **switched** while it had you marked (+1) | It shakes off **sleep or freeze** (−2) |
| The storm feeds its stride (+1, on a storm roll) | Its **Speed stage** is below neutral (−1 every turn it stays there) |
| | Its quarry **stood its ground** for three turns (−2; −3 in the Hunt) |
| | You **read** a telegraphed Lightning Dash (−2) |

Velocity also moves Raikou's raw Speed up and down with it, so a fast Raikou really is fast.

### The guard ladder

| Velocity | Guard | The line that tells you |
| --- | --- | --- |
| 0 — grounded | 82% | "Raikou's stride breaks — you can see it again!" |
| 1–2 — running | 85% | |
| 3–4 — a blur | 88% | "Raikou blurs — your attacks are barely finding it!" |
| 5 — full stride | 90% | "Raikou hits full stride — it's everywhere at once!" |
| **Hunt broken** (1 turn) | **76%** | "Raikou overshoots — for a moment it's wide open!" |
| **Wild discharge** (2 turns, final phase) | **70%** | "It's blown itself off its feet — now!" |

The ladder moves **both ways** and announces itself every single time it changes, in either
direction — that pair of lines is your read on how much your attacks are actually worth right now.

There are exactly **two** windows where its guard genuinely drops, and both must be earned: breaking
the Mark, and the final phase's discharge. Everything else is the ladder.

### The Mark — and why running from it is the wrong answer

At Velocity 3 or higher (and always, once the Hunt begins) Raikou picks a target: *"Raikou's eyes
lock onto your Pokémon!"* The marked Pokémon then takes about **6% of its max HP** at the end of that
turn and the end of the next one.

You have two answers, and they are opposites:

- **Switch out.** The chip stops — but *"Raikou gives chase!"*, Velocity rises, the mark re-lays on
  whoever came in, and from the Hunt on the newcomer is struck for ~10% the moment it arrives.
  Running feeds the hunt.
- **Stand your ground.** If the marked Pokémon is still out at the start of the third turn, *"Its
  quarry refused to break!"* — Raikou overshoots, loses 2 Velocity (3 in the Hunt), and its guard
  drops to **70% for one full turn**. While it is prowling it also can't re-mark for two turns.

This is the inversion the fight is built on. Protecting the hunted Pokémon and baiting Raikou into
overextending are the same decision, and the second one is what opens it up.

### Lightning Dash — the telegraph

At full stride Raikou winds up: *"Raikou coils, and the air behind it goes still…"* On the **next**
turn it genuinely evades — *"Raikou vanished in a flash!"* — and everything aimed at it does nothing,
your Ground move included.

You get one full turn of warning. Spend the dash turn on setup, healing, a status move, or a switch
instead of throwing an attack into a wall, and Raikou's charge finds nothing to strike: **−2
Velocity**. Attack into it and you have simply wasted the turn.

### The storm

From Velocity 3 up, thunder starts rolling across the field — roughly once every other turn. When it
does, one of four things happens:

| Roll | What happens |
| --- | --- |
| ~30% | A bolt falls on your active Pokémon for ~9% of its max HP |
| ~25% | The storm feeds Raikou's stride — Velocity +1 |
| ~25% | Rain lashes sideways — your side's Accuracy drops a stage |
| ~20% | Static hangs in the air — Raikou finds another gear (raw Speed up) |

It is not weather, and nothing you do to the weather affects it. It stops entirely if you get Raikou
back under Velocity 3.

### What does and doesn't stick

- **Paralysis does nothing.** Raikou is an Electric type.
- **Sleep and freeze land, but Raikou tears itself free at the start of its next turn.** They cannot
  lock the fight — but shedding one costs it **2 Velocity**, which is the single largest tempo break
  in the game. A well-timed sleep is worth the turn it costs you.
- **Any turn Raikou never acts costs it 2 Velocity**, however that happened — a flinch, a recharge,
  Taunt, a move that failed outright. It never double-counts with the status shed.
- **Burn, poison and confusion all stick normally.** Burn is a genuine answer to Extreme Speed and
  Crunch.
- **Speed control is the quiet lever.** Any negative Speed stage on Raikou — Icy Wind, Electroweb,
  String Shot, Bulldoze, Rock Tomb — drains **1 Velocity every turn it stays there**, no matter what
  Raikou does. Its own raw Speed growth never cancels it out.
- **Ground moves are both your best damage and your brake** — Raikou is pure Electric. But it carries
  **Aura Sphere** precisely because your Ground answer is usually Ground/Rock or Ground/Steel. Bring
  the lever, expect it to be targeted.

### Phases and catching it

1. **Prowl** (above 50% HP) — the ladder is live from 0, it only marks at Velocity 3+, and breaking
   the mark buys a two-turn reprieve from being marked again.
2. **The Hunt** (50% HP or below) — *"It has stopped circling. Now it's hunting."* Velocity can never
   fall below **2** again, the mark becomes permanent and re-lays instantly, switching draws an
   immediate strike, and the Ground lever weakens to −1. In exchange, breaking the mark is now worth
   −3 Velocity **and** the 70% window. That trade is your objective for the rest of the fight.
3. **Lightning Incarnate** (25% HP or below) — Velocity locks at 5, Raikou's Speed and offense jump,
   and loose current arcs across your whole team for ~10% every turn. Its body can't hold the charge
   it's carrying: after a few turns it **discharges wildly**, hurting itself for ~12% and cratering
   its guard to **55% for two full turns**. Then it rebuilds and does it again. Watch for *"Raikou's
   coat stands on end"* — that is your one-turn warning. This is where the fight is won.
4. **Weakened** (10% HP or below) — the thunder dies in its throat. Every guard drops, every mechanic
   switches off, and **Poké Balls work**. The only catch window. Its catch rate is generous here, and
   the catch-phase safety net (see [Common to every legendary](#common-to-every-legendary)) keeps
   Raikou alive through your hits.

---

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

## Suicune — The Purifier

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
