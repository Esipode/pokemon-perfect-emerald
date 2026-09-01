## Groudon — The Living Continent

Groudon fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also refuses to be paralysed.

Sleep, however, works — and that is deliberate. Nothing in this fight runs on Groudon's turn.

Kyogre was a fight over a **water level** that moved both ways on its own. Groudon is a fight over a
**ratchet**. The land only ever climbs by itself. It comes back down for exactly one reason, and you
have to keep prying it down for the entire battle.

### The core loop

```
The LANDMASS runs from 0 to 5.  It rises on its own, it rises when
GROUDON lands a Ground or Fire move, and it rises when you burn it
with FIRE.

It falls for exactly ONE reason: a damaging WATER move hitting GROUDON.

The Land does three things at once:
  - GROUDON gets harder to damage
  - from SCORCHED PLATEAU up, the ground burns your whole party
    every single turn
  - and it sets the QUAKE FUSE

The fuse is the fight.  Every few turns the ground goes off, and how
hard it hits is read off the LANDMASS at the moment it lands - a light
tremor at the bottom, a party-ending shockwave at the top.  Higher land
also makes the fuse SHORTER.  At PRIMAL LAND it fires every turn.

Only ONE move-driven land change lands per turn, in either direction.
Groudon is fast enough to spend it first.
```

### Reading the land

Groudon never tells you a number. It tells you how high the land is, and it tells you **every time
that changes**, in either direction:

| The line | Where the land is | What it means |
| --- | --- | --- |
| *(no line)* | **Barren Ground** | Groudon's softest guard, longest fuse |
| *The ground is pushing upward around your POKéMON.* | **Rising Earth** | Same guard, but it is climbing |
| *HIGHLANDS. Groudon is harder to reach across the risen rock.* | **Highlands** | Guard up, and the quakes get real |
| *SCORCHED PLATEAU. The ground is too hot to stand on!* | **Scorched Plateau** | Your whole side starts burning every turn, and the fuse shortens |
| *CONTINENTAL RISE. The land is closing in around you!* | **Continental Rise** | Harder guard, heavier burn, and quakes now strip your screens |
| *PRIMAL LAND. The battlefield has become part of Groudon!* | **Primal Land** | The worst of everything, and a quake **every turn** |

Every individual step is announced as it happens:

> *The land swells higher!* → the land gained a step
> *The land around Groudon erodes away!* → you took one back

From Scorched Plateau up, a line repeats every turn as your party burns:

> *The scorching ground sears your POKéMON!*

### Moving the land

**It rises:**

- On its own, every third turn at first — and **every turn** once Groudon reverts to its Primal form.
- Whenever Groudon lands a **Ground** or **Fire** move. Two of its four moves do this; the other two
  do not.
- Whenever a **Fire** move hits it. *Groudon drinks in the flame. The ground rises to meet it!*
- Whenever you try to paralyse it. *Groudon will not be slowed!*

**It falls:**

- Whenever a damaging **Water** move hits Groudon. That is the entire list.

**The catch — and this is the whole fight:** only **one** move-driven land change lands per turn, in
either direction, and it goes to whoever moves first. If your Surf lands first, you spend it on a
step back and the turn nets zero against the natural rise. If Precipice Blades lands first, it
spends it on a step forward, **your erosion does not happen at all**, and the turn nets **two steps
against you**.

Groudon's Speed is 90. The top two quake tiers drop your Speed a stage every time they land, so
falling behind makes falling further behind easier.

### The quake

There is always a quake coming, and you are always told how close it is:

> *The ground trembles beneath you.* → still a few turns out
> *The earth begins to crack!* → next turn but one
> *Groudon is preparing a devastating earthquake!* → **it lands at the end of this turn**

You always get at least one turn of warning, at every tier. When it goes off, its size is read off
the landmass **at that moment** — not at the moment the fuse was set. Eroding one step in the turn
before a quake genuinely makes that quake smaller.

| Landmass | What lands | Extra |
| --- | --- | --- |
| Barren / Rising | *TREMOR! The field shudders underfoot!* | A light chip |
| Highlands / Scorched Plateau | *QUAKE! The ground bucks and throws your POKéMON down!* | Your side's Speed drops a stage |
| Continental Rise | *MAJOR QUAKE! The shockwave tears everything loose!* | Speed drop, **and your Reflect / Light Screen are torn down** |
| Primal Land | *CONTINENTAL BREAK! The whole landmass comes apart at once!* | The heaviest hit in the fight, Speed drop, screens stripped |

High land does not just make the quake bigger — it makes the **fuse shorter**. At the bottom of the
ladder a quake is every third turn. At Primal Land it is every single turn.

### The sun

Groudon's Drought is permanent. Under it, Solar Beam charges in one turn and Fire moves are boosted.

For the first half of the fight, **your weather moves work normally**. A Rain Dance is a real,
rewarding play here: it takes Eruption's boost away and puts Solar Beam back on a charge turn.

From the Primal Reversion onward, it stops working:

> *The land will not be watered. The sky burns clear again!*

Water moves still work perfectly. Only the weather is refused.

### Phase 2 — Primal Reversion (50%)

> *PRIMAL REVERSION*
> *Groudon's ancient power erupts! The land answers it now without being asked!*

Four things change:

1. **Groudon becomes Ground/Fire.** Your Water moves stay your best tool, Grass coverage stops being
   super effective, and Fire is now resisted on top of feeding the land.
2. **The natural rise goes from every third turn to every single turn.** Everything you were already
   doing still works. You just have to do it three times as often — and a perfect erosion turn now
   only buys you holding even.
3. **Eruption is replaced by Fire Blast.** Eruption was fading as Groudon's HP dropped; Fire Blast
   does not.
4. Groudon hits harder, and the sun can no longer be changed.

### Phase 3 — Continental Collapse (25%)

> *Groudon's power begins tearing its own continent apart!*

The mechanic turns on its owner. The land Groudon spent the fight building starts caving in, on a
fixed clock, damaging **Groudon and your party together**:

> *CONTINENTAL COLLAPSE! The ground caves in under everything standing on it!*

Groudon cannot be knocked out during this phase, so the collapse always runs its course. Your job is
simply to still be standing when the continent finishes falling — and a line repeats every turn to
tell you it is working:

> *The land will not hold. Groudon is being crushed under its own weight!*

This is **not** a phase where you can stop playing. The landmass ladder, the natural growth and the
quake fuse all keep running underneath it. At Primal Land that is a full-strength quake *plus* the
scorch *plus* the collapse, every single turn. The only way to make the phase survivable is to keep
eroding right through it.

Roughly three collapses in, the land gives out and breaks Groudon on its own.

### Phase 4 — Weakened (10%)

> *The continent sinks away, and Groudon folds back into its true shape.*
> *It's spent - now is the moment to catch it!*

Groudon reverts, the sun goes out, the landmass resets, and every mechanic in the fight goes quiet.
**This is the first and only point in the battle where a Poké Ball can be thrown.**

### What to bring

- **Water attackers**, and fast ones. They are your *only* way to push the land back, and they only
  work if you move first.
- **More than one of them.** Solar Beam charges instantly under Groudon's sun and is 4x into exactly
  the bulky Water/Ground Pokémon you would bring to erode. A single erosion bot gets removed.
- **Speed.** The whole fight is a Speed race, and Groudon attacks it directly — the top two quake
  tiers drop your Speed every time they land.
- **Party-wide bulk.** The scorch and the quakes hit your **whole team**, not just the active
  Pokémon. Benched Pokémon are not safe.
- **No Fire.** Every Fire move you use hands Groudon a free step up the ladder.
- **Not Thunder Wave.** Paralysis is cured immediately and the land climbs for your trouble. Burn
  and Toxic both still work, and burn is genuinely good against a boss whose best move is physical.
- **Screens with your eyes open.** They help right up until a Major Quake tears them down.
