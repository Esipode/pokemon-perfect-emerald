## Genesect — The Weapon Platform

Genesect fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

> **Genesect answers whatever you last did. The attack that lands is filed and stops working, and
> the Drive it equips counters the way you fight. Only driving the machine past its own limits clears
> the file — and it will not re-equip until you change what you are doing.**

Two numbers run the fight, and you move both of them yourself.

---

### THE FILE — type analysis

Every damaging move that **lands** on Genesect has its type **filed**. A filed type does far less
damage from then on, and the fight names the type out loud each time.

```
  PHASE          SLOTS   FILED AT   RE-HIT HARDENS TO
  Drive System     2        50%            65%
  Combat Protocol  2        60%            75%
  Weaponized       3        70%            90%
```

Underneath the file sits a flat **86% damage reduction** that never moves except inside a cooling
window. So a filed type lands for roughly **4–7%** of what it would do unreduced, while an unfiled
type keeps the full **14%**. That gap *is* the fight.

The board is **FIFO**: with two slots held, filing a third type pushes the **oldest** one out, and
Genesect says which one it dropped. Hitting a type it already holds **hardens** it instead — one
step per phase, climbing toward a 90% ceiling. Spamming one move loses. Alternating two moves walls
both of them.

---

### THE MACHINE — Drives, Strain, and the Charge

The same hit that gets filed also tells Genesect **which Drive to equip**. The read is the move that
just landed:

| What landed on it | Drive | Slot 4 becomes | Field | Stat |
| --- | --- | --- | --- | --- |
| A **Fire** move | **DOUSE** | Bug Buzz | Rain | +Sp. Def |
| A **physical** move | **CHILL** | Ice Beam | — | +Def, and **−1 Speed on you** |
| A **special** move | **SHOCK** | Thunderbolt | Electric Terrain | +Speed |
| **Nothing at all** | **BURN** | Flamethrower | Harsh sunlight | +Atk, +Sp. Atk, and 6% chip on you |

Each Drive is a real form change, so the loadout is visible on the sprite. The stat bumps are +1 in
Phase 0 and **+2** from 50%.

- **Fire is checked first.** A Fire *physical* move equips Douse, not Chill. That is the rule, not a
  bug — the 4x weakness gets its own answer, which is the whole point of a machine that analyses.
- **Rain is Douse's answer to your best move.** Bug/Steel is 4x weak to Fire (capped at 2x here),
  and leading Fire is what arms the counter to Fire.
- **Electric Terrain is the answer to sleep.** A grounded Pokémon cannot be put to sleep on it, and
  Shock is exactly what equips against the special attackers who carry Spore and Hypnosis.
- **Burn is the anti-stall arm.** A turn where nothing reached Genesect — Protect, a miss, a sleep
  turn, a Substitute, a turn spent healing — configures it for attack and chips you for 6%.

#### SYSTEM STRAIN, 0–3

```
  a NEW Drive is equipped      ->  Strain +1
  a turn with no new Drive     ->  Strain -1
  Strain reaches 3             ->  SYSTEM OVERLOAD
```

**SYSTEM OVERLOAD** is what you are playing for. The whole file is wiped, the Drive is ejected back
to base Genesect, the Charge is lost, Genesect loses its next action, and the guard collapses from
**86 to 60 for two turns**.

Because Strain decays on any quiet turn, an overload costs **three consecutive turns** of attacking
the way Genesect is *not* configured for — paid in tempo, with the cannon charging the whole time.
It cannot be banked slowly.

#### THE WEAPON CHARGE, 0–5

```
  +1  Genesect's side took health off you this turn
  +1  ...and it cost you 25% or more of that Pokemon's max HP   (+2 from 50%)
      a knockout always counts as the heavy clause
```

The cannon is a clock **you** wind by standing still and taking hits.

At Charge 5 in Phase 0/1 it fires **TECHNO BLAST** for **16%** (Phase 0) or **20%** (Phase 1) of your
max HP — **30%** if you are locked. Then the weapon **overheats**: Charge to 0, the **newest**
adaptation vented off the board, and Genesect loses its next action. Firing is itself a window.

---

### The moveset

Slot 4 is the Drive slot and is rewritten by every re-equip. Slots 1–3 never change.

| Slot | Move | Why it is there |
| --- | --- | --- |
| 1 | **Techno Blast** | The signature. With no Drive *item* held it resolves as a 120 BP **Normal** special attack — the cannon firing uncharged. The charged shots are scripted |
| 2 | **X-Scissor** | Physical Bug STAB off base 120 Attack, so a special wall is not a free answer |
| 3 | **Metal Sound** | −2 Sp. Def on you. A machine that reads your defences and then removes them — ignore it and the scripted blasts land into a softened target |
| 4 | **Bug Buzz** → Ice Beam / Thunderbolt / Flamethrower | Whatever the equipped Drive arms. Starts as Bug Buzz; no Drive is equipped at battle start |

Its ability is **Download**: on entry it raises Attack or Sp. Attack from whichever of your defences
is lower. Every phase transition refills its PP.

---

### Phase 1 — COMBAT PROTOCOL (50%)

- The board now files at **60%**, hardening to 75%
- Drive stat bumps **double**
- **SYSTEM PURGE**: a status is vented at the top of every turn instead of being carried
- **TARGET LOCK** goes live (see below)
- Heavy hits charge the cannon **twice as fast**, and Techno Blast hits for 20%

#### TARGET LOCK

From 50%, once the Charge reaches 3, Genesect locks your active Pokémon: **it cannot switch or flee
for three turns**, and Techno Blast hits a locked target for 30% instead of 20%. It is announced
before you pick a move.

Switching cannot be the answer when the lock's whole content is that you may not switch. **Two things
break it early:**

1. **Make it change Drives** — the analysis re-targets, and the lock goes with it.
2. **Overload it.**

It also expires on its own after three turns, and then sits on a short cooldown. Ghost-types walk out
regardless, exactly as they do from Mean Look.

---

### Phase 2 — WEAPONIZED ADAPTATION (20%)

- The board widens to **3 slots at 70%**, hardening to the 90% ceiling
- **DUAL DRIVE**: a re-equip no longer discards the previous Drive. Its stat bump is re-applied and
  its field effect is left standing, so the machine runs two loadouts at once. Which two is whatever
  your own last two attacks produced — two weather Drives still overwrite each other, but Shock
  genuinely pairs with anything
- The cannon **no longer fires** at Charge 5

#### THE OVERCLOCK

Instead of firing, Charge 5 starts a **three-turn destabilisation**, warned on every one of those
turns. If it completes, **TECHNO BLAST: OVERCLOCK** deals **38% of your max HP** plus a battlefield
effect chosen by the equipped Drive (Burn → sun, Douse → rain, Shock → Electric Terrain, Chill → −2
Speed across your side).

**Two ways to interrupt it** — and they are the two verbs the fight has spent the whole battle
teaching you:

1. **Damage**: take **18% or more of Genesect's max HP** off inside the window.
2. **Strain**: force a **third System Strain** while the overclock is running.

Either one catastrophically overheats the weapon: file wiped, Drive ejected, Charge and Strain zeroed,
**two** actions lost, and the guard collapsed to **50 for three turns**. That is the largest window in
the fight and the only place the guard goes below 60.

> The overclock can KO. On a nearly-dead team, letting it complete can lose the fight there.

---

### The last stand, and the catch

`Survive: True` carries the fight to 1 HP. There Genesect fires **one last shot** for 10% of your
side, and only then do the file, the Drive, the trap, the guard, the immunities and Survive all come
off, and Poké Balls unlock at a catch rate of 30. The engine's own catch-window guard keeps Genesect
alive while you throw, and it reverts to its **base form** first, so a caught Genesect is plain
Genesect. **This is the only point in the fight where a ball works at all.**

Catching it hands over all four Drives — Douse, Shock, Burn and Chill.

---

### What does and does not work on it

| Plan | What happens |
| --- | --- |
| Spamming your best move | The board files that type, then hardens it by name. Your best move stops working and the fight tells you so |
| Rotating two attacking types | Both get walled at 2 slots. You need more distinct types than it has slots, or an overload |
| A 4x Fire hit | Capped at 2x — and Fire is the one type with a dedicated answer. It equips **Douse**, and the rain halves Fire from then on |
| Sleep | Lands in Phase 0 and is a real play. The **Shock** Drive's Electric Terrain stops it outright, and Shock is what special attackers arm |
| Burn / paralysis / freeze / Toxic | All land. Poison is refused by Steel typing on its own; from 50% every status is vented at turn open, so status control costs a move *every* turn instead of once |
| Protect / Substitute / healing loops | A turn where nothing reaches Genesect equips **BURN**: both attack stats up, plus 6% chip. Stalling makes the machine hit harder |
| Lowering its stats | Works — it has Download, not Clear Body. Nothing is immune here |
| Taunt, Disable, trapping it | All work |
| One-hit KO, Super Fang, Pain Split, Destiny Bond | All refused |
| Waiting for the board to decay | **It never decays.** Registeel's board opens on its own; this one does not. Only an overload or a Techno Blast's vent clears anything |
| Switching out | Free — until TARGET LOCK, which is the one thing that takes it away |
| PP stalling it out | Every phase transition refills its moves |
| Throwing a ball early | Blocked until 1 HP |

---

### The short version

1. **Change what you are throwing.** The type that just worked is the type about to stop working,
   and it says so by name.
2. **Every change you make also changes its Drive, and every new Drive is a point of Strain.**
   Three changes in a row overload it: file wiped, guard 86 → 60, an action gone.
3. **Strain decays on any turn you do not force a change**, so the overload has to be three
   *consecutive* off-profile turns.
4. **Standing still winds the cannon.** Charge 5 fires for 16–30%, and a locked target takes the
   worst of it. Stalling instead arms BURN and chips you.
5. **From 50% it vents status, doubles its bumps, and traps you.** Break the lock by making it
   change Drives.
6. **From 20% the cannon stops firing and starts overclocking.** Break it with 18% of its HP inside
   three turns, or with a third Strain, for the biggest window in the fight.
7. Save your ball. It does nothing until Genesect's systems fail at 1 HP.
