## Zapdos — Storm Overload

### Location: Route 110 - Thunderstorm Weather

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
