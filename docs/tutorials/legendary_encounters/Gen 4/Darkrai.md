## Darkrai — The Pitch-Black Nightmare

Darkrai fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until the very end, so the catch window is guaranteed
no matter how the damage race goes.

It keeps its own ability, **Bad Dreams** — a sleeping Pokémon of yours loses health every turn just
for being asleep — and its whole moveset is built to cash that in.

Every other legendary in the set is honest with you about what just happened. Darkrai is not.

> **Half of what this fight prints on screen did not happen. There is exactly one tell, it never
> varies, and once you have learned it nothing behind it can hurt you.**

---

### The two meters

```
 NIGHTMARE  0 ......... 1 ......... 2 ......... 3 ......... 4 ......... 5
            unease    stirring   distortion  HALLUCIN.   TERROR    NIGHTMARE
                                                                     REALM
  guard    84 ---- 86 ---- 90 ---- 92 ---- 94 ---- 96   -> 80 for three turns
                                                             after WAKE UP
  lies         -       -    Ph 1+   Ph 0+   Ph 0+    Ph 0+
  copies       -       -       -       -    Ph 1+    Ph 1+
```

**Darkrai's guard is a direct read of the Nightmare level.** "Can I actually hurt this thing" and
"am I winning the meter war" are the same question. Let the dial sit at 5 and you are hitting a 96%
wall with no counter-play; hold it at 1–2 and you are fighting something you can genuinely beat —
and at 0 it is softer than any other legendary in the set.

Nothing on screen ever shows you the number. What it shows you is **the dark**, and the dark
describes the number honestly, in both directions, every step of the way:

| | The dark says |
| --- | --- |
| **5** | *"THE DARK IS COMPLETE. There is no field here any more — only the dream."* |
| **4** | *"Shapes move at the edge of the field that have no business being there."* |
| **3** | *"You are no longer certain the battle in front of you is the battle you are in."* |
| **2** | *"The colours on the field have gone slightly wrong."* |
| **1** | *"Something feels wrong."* |
| **0** | *"The air clears. The field is just a field again."* |

Underneath the crossings, a short line keeps running roughly every other turn once the dial is at 3
or above — *"The dark presses in a little closer."*

### What moves it

| Raises NIGHTMARE (+1) | Raises WAKEFULNESS (+1) |
| --- | --- |
| **Turns passing** — every 3 turns, then every 2 from Phase 1 | A **Fighting, Bug or Fairy move** landing on Darkrai — *while Darkrai is still Dark-typed* |
| **One of your Pokémon fainting** | **Switching a Pokémon in** |
| **Any status landing on Darkrai** — once, on application | |
| **Any status standing on your Pokémon** — once, on application | |
| **Using the same move category twice** — every second repeat | |

**Wakefulness is the only thing that pushes Nightmare back.** At **3** it spends itself: the meter
resets to 0, Nightmare drops a level, and any illusion standing on the field comes down. You can
only gain one Wakefulness per turn, whatever the source.

Type is read off the **move's printed type**, not what it hit as — a Normalize'd, Electrified or
Tera'd attack still counts as whatever the move data says it is. The category read is the same deal:
it sees the category, not the move, so alternating two different physical attacks still reads as a
repeat. The fight is asking you to vary *how* you attack, not which button you press.

---

### THE LIES — and the tell

From Nightmare 3 (Phase 1: Nightmare 2; Absolute Nightmare: always), Darkrai starts printing battle
events that did not happen. There are four of them, and **every single one opens with the same two
beats:**

```
    "The dark ripples."
    [ the Nightmare animation plays over Darkrai ]
```

That is the tell. It never varies, it is never used for anything else, and **nothing that is
actually happening to you is ever introduced by it.** A real event arrives the way the engine has
always delivered it — *"Foe Darkrai used Hex!" / "It's super effective!"*

| The lie | What it prints | How you catch it |
| --- | --- | --- |
| **FALSE COLLAPSE** | *"Darkrai's strength is failing!"* — and the bar **really drops a quarter** | It is put back at the start of the next turn: *"Nothing was ever wrong with it."* |
| **FALSE STATUS** | *"…was badly poisoned!"* with the poison animation | No status icon in the health box, and no chip damage at end of turn |
| **FALSE BOOST** | *"…'s Sp. Atk rose!"* | The stat-up animation the engine always plays is **missing**, and your damage does not change |
| **FALSE VANISH** | *"Darkrai sank into its own shadow."* | It is still there, still targetable, and attacks normally next turn |

**The FALSE COLLAPSE is the only one with teeth, and it can only bite you if you act on it.** The
health bar genuinely falls — so a finisher, an X-item turn or your last Full Restore spent on a
Darkrai that looks nearly dead is a real, expensive mistake. It is never rolled while Darkrai is
below 40% health, and a phase change or the catch window can never be triggered off the false
number.

The other three cost you nothing but attention. Four out of five hallucinations in this fight are
pure theatre.

---

### NIGHTMARE COPIES

From **Phase 1, at Nightmare 4 or above**, Darkrai stops pretending and simply **becomes your
Pokémon** — species, stats, stat stages, types, ability and moveset — for three turns, then cannot
do it again for three more.

> *"A nightmare wearing …'s face rises out of the dark."*

Its health bar keeps ticking down from where it was, it is still the boss, and it still catches as
Darkrai. But it fights with **your** moves, at **your** stats, and there is one consequence the
fight never mentions:

**A copy is not Dark-typed.** Your Fighting / Bug / Fairy answer stops paying Wakefulness for as
long as the face is up, and switching is the only lever you have left — which is also the thing that
ends the copy early. The mechanic and its counter are the same action.

---

### Sleep, status, and why they are not free

Darkrai's answer to being statused is not an immunity. **It is a price.**

- **Every status you land costs a Nightmare level** on the turn it applies — *"Darkrai takes the
  affliction the way it takes everything else."* After that it works completely and permanently.
  Toxic will not run away with the fight; its damage is held flat.
- **A status standing on your own Pokémon costs a level too.** Letting Hypnosis or Dark Void through
  is expensive twice over, because Bad Dreams is already chipping you.
- **Darkrai does not sleep.** Sleep lands, costs it the rest of that turn, and is scrubbed at end of
  turn for the same Nightmare level. *"Darkrai does not sleep. It is what sleeps in you."*

A status is a real, usable tool here. It is just worth a step in the wrong direction on the only
meter that matters.

---

### The phases

| | |
| --- | --- |
| **100–60% — The Pitch-Black Nightmare** | Nightmare climbs every 3 turns. Lies from Nightmare 3. Moveset: Dark Pulse / Hypnosis / Hex / **Nasty Plot**. |
| **≤ 60% — THE NIGHTMARE DEEPENS** | +1 Nightmare on entry, and the clock shortens to 2 turns. Lies from Nightmare 2. **Nightmare Copies unlock** at Nightmare 4. Nasty Plot gives way to **Dark Void**. |
| **≤ 30% — ABSOLUTE NIGHTMARE** | **Darkrai Mega Evolves.** The Nightmare **pins at 5** and the Wakefulness spend switches off — the dial only moves via WAKE UP now. Dark Void out, **Dream Eater** in. |
| **≤ 10%** | The catch window. |

**The Mega is a wall you outrun.** Def/Sp. Def 90 → 130 and Sp. Atk 135 → 165 behind a 96% guard is
the hardest point in the fight. Base Speed **125 → 85** is the compensation, and the fight tells you
so: *"It is heavier now. Whatever else this thing has become, it is slower."* That is the one honest
advantage the climax hands you, and it is worth more than it looks.

There is **no trapping** in this fight. Switching is a Wakefulness source and Wakefulness is the win
condition; closing that door would close the fight.

---

### WAKE UP

In Absolute Nightmare, Wakefulness no longer spends itself. It charges to **3** and you are asked a
question instead:

```
                            WAKE UP?
                        [  YES  ]  [  NO  ]
```

**YES** shatters the dream:

- Every illusion comes down. Any copy ends.
- Nightmare → **0**, and Darkrai's guard falls **96 → 45 for three turns**.
- It loses **two Sp. Atk stages** and **this turn's action**.
- **Your own Pokémon's status is cured** — the sleep goes with the dream.

**NO** costs you nothing. Wakefulness is not spent; the prompt just goes quiet for three turns.
Declining is a timing decision, not a punishment — and if you are one turn from a fresh Pokémon or a
setup move landing, it is often the right one.

When the window closes — *"The dark closes over you again."* — the Nightmare snaps back to 5 and the
guard back to 96. **Rebuild Wakefulness to 3 and the prompt comes back.** Phase 2 is a race between
how fast you can re-open that window and how long a 165-Sp.-Atk Mega body takes to grind you out.

---

### Catching it

Poké Balls are **blocked for the entire fight** until Darkrai is in Absolute Nightmare **and** at
10% health or less. At that point the Mega comes apart, everything false in the battle comes down at
once, its guard and immunities are lifted, and the message tells you plainly:

> *"The dark thins, and what is left of Darkrai is a small thing that has been holding all of this
> up on its own. Now — now is the moment to catch it!"*

A Darkrai caught there is an ordinary Darkrai.

---

### The short version

1. **Vary your attacking category.** Leaning on one is a free Nightmare level every other turn.
2. **Bring a Fighting, Bug or Fairy move** and use it on Darkrai. It is your main Wakefulness source.
3. **Switch when the meter stalls** — especially while a copy is up, when it is the only source left.
4. **Learn the ripple.** After *"The dark ripples,"* believe nothing until the next real message.
5. **Never act on a collapse.** If the bar fell right after a ripple, it will be back next turn.
6. **In the Mega phase, build to 3 and say YES.** Then use all three turns of the window.
