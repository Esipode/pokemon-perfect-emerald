## Deoxys — The Alien Organism

Deoxys fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage.

None of that is the fight. The fight is that **Deoxys is four Pokémon, and it decides which one to
be by reading you.**

### The core loop

Deoxys rebuilds itself on a timer. Each rebuild it looks at what you have been doing and becomes the
forme that answers it. Every *real* change costs it a point of a hidden resource called **Mutation
Stress** — and at 10 it comes apart.

```
Reconstruct  ->  read you  ->  become the forme that counters you  ->  +1 Stress
Stress 10    ->  CELLULAR INSTABILITY: it collapses to Normal Forme, cannot rebuild
                 for three turns, and takes ~2.3x the damage.  Then it resets and repeats.
```

So the way to hurt Deoxys is to **make it keep changing**, and the way to make it keep changing is
to **keep changing what you do**. The Defense Forme barrier teaches the same lesson a second time.

### The four formes

All four share 50 base HP, so the health bar never jumps. All four are pure Psychic with Pressure.
The moveset survives every change.

| Forme | What it does | Damage it takes |
| --- | --- | --- |
| **Normal** | Observes. No stance. Rebuilds fastest of the four | 1.00 (the reference) |
| **Attack** | 180/180 offences, plus the **Assault** meter | **1.31** — the glass cannon |
| **Defense** | 160/160 behind an **adaptive barrier** | 0.76, falling to **0.38** at a full barrier |
| **Speed** | 180 Speed, permanent Tailwind, **Blitz** every turn, and it can dodge | 1.15 |

The guard runs *backwards* relative to each forme's own bulk on purpose. Attack Forme behind a flat
90% reduction would evaporate in one hit; Defense Forme would be unkillable. So the reduction
compensates for the shape instead of reinforcing it, and what you feel is the table above.

Deoxys also carries an invisible defensive floor. It has the smallest health bar of any legendary in
the game — half Ho-Oh's — and Attack Forme's paper defences would otherwise let a single
super-effective STAB hit take nearly half of it. The floor is what makes the fight last; expect
roughly **9 super-effective hits in Attack Forme and 12 in Normal**, and considerably more if you
are attacking into the barrier.

### The selection ladder — this is where you have agency

At the top of a reconstruction turn you get:

> *Deoxys is changing its cellular structure!*
> *RECONSTRUCTING...*

It braces for that turn (a real Protect, plus a near-total guard) but **you are not locked out — you
act normally, and what you do is the input.** The next turn it resolves, walking four rules in order:

| # | If… | It becomes |
| --- | --- | --- |
| 1 | your active Pokémon **outspeeds** it | **SPEED FORME** |
| 2 | it lost **10% or more of its max HP** during the reconstruction turn | **DEFENSE FORME** |
| 3 | it is **below 50% HP** | **ATTACK FORME** |
| 4 | none of the above | **NORMAL FORME** |

Every rung is something you control:

- **Lead with something fast** → Speed Forme. Then nothing outspeeds 180 base, so rule 1 goes false
  next cycle and it leaves again. Speed Forme self-terminates.
- **Save your biggest hit for the telegraph turn** → Defense Forme, the wall.
- **Chip it gently below 50%** → Attack Forme, which takes almost half again the damage.
- **Do nothing notable above 50%** → it stays Normal, the state it is easiest to fight in.

And if the read comes back the same as what it already is:

> *Deoxys finds nothing worth changing.*

**That line is the whole tutorial.** It costs Deoxys nothing and buys you nothing — you played the
same way twice and it is telling you so. The bait falls straight out of the ladder: land a huge hit
to push it into Defense, then switch to a fast lead so the *next* cycle drags it into Speed, and you
have spent two Stress in four turns without ever letting it settle into the forme that counters you.

### Mutation Stress — the counter-tension

You never see the number. You see a tier line whenever it moves:

| Line | Stress |
| --- | --- |
| *Deoxys' cells are holding their shape.* | 0–3 |
| *Deoxys' cells are straining to hold together - and hitting harder for it.* | 4–7 |
| *Deoxys' body is barely holding its shape, and everything it does is violent!* | 8–10 |

| It gains Stress when… | Gain |
| --- | --- |
| a reconstruction lands on a **different** forme | +1 |
| a **Hybrid Mutation** fires (Phase 2) | +1 |
| it **shrugs off sleep or freeze** at turn open — **but only while Stress is 7 or below** | +1 |

Stress does **not** touch the guard. It scales the **stance payloads**:

| Stress | Assault strike | Blitz chip | Barrier it enters Defense Forme with |
| --- | --- | --- | --- |
| 0–1 | 6% | 2% | 0 |
| 2–3 | 8% | 3% | 0 |
| 4–5 | 10% | 4% | 0 |
| 6–7 | 12% | 5% | 1 |
| 8–9 | 14% | 6% | 1 |
| 10 | 16% | 8% | 2 |

So forcing transformations is **playing with fire**: you are stacking the thing that will eventually
break it, and being hit harder every step of the way for doing so.

### Rebuilding costs you HP off the bar

A reconstruction that lands on **Normal or Attack Forme heals Deoxys 15% of its max HP** — rebuilding
its body from scratch is also repairing it. Speed and Defense Forme get no heal; they pay for their
stance instead.

Both Hybrid Mutations heal the same 0%.

This is the other half of the trade. Baiting it through forme after forme drives Stress toward the
instability window, but every Normal or Attack landing you cause hands back a chunk of the bar you
just chewed through. Forcing a rebuild is never free, and the fastest kill is rarely the one with
the most transformations in it.

### Cellular Instability — your damage window, and it repeats

> *CELLULAR INSTABILITY!*
> *Deoxys collapses out of its forme, and cannot rebuild!*

Stress resets to 0, the barrier and Assault meter are wiped, the Tailwind is stripped, it collapses
to Normal Forme, and for **three turns** it cannot reconstruct and takes roughly **two and a third times** the
damage it has been taking. A standing line runs while it lasts — *Deoxys' cells are still coming
apart* — and it ends with *Deoxys' cells have stabilized.*

Because Stress **resets rather than latching**, this happens again and again. It is a rhythm you
learn to drive, not a one-time phase.

Note the cap on the status clause: sleeping or freezing it every turn can carry Stress most of the
way, but never past 8. **The last two points have to be forced out of mutations.**

### Defense Forme — the adaptive barrier

The barrier does not care about super-effective or critical hits. It cares about **what kind of
attack last landed**, sorted into five buckets: Bug, Ghost, Dark, anything-else-physical, and
anything-else-special.

| You do this | It does this |
| --- | --- |
| hit it with the **same bucket** as last time | *Deoxys' cells harden against that attack!* — barrier +1, up to 3 |
| hit it with a **different bucket** | *Deoxys' cellular defenses collapse!* — barrier straight back to 0 |

| Barrier | Damage it takes |
| --- | --- |
| 0 | 0.76 |
| 1 | 0.62 |
| 2 | 0.50 |
| 3 | 0.38 |

One adaptation per turn, so a multi-hit move counts once. And **it remembers the last attack type in
every forme**, not just this one — nuke it with one move on the way in and the barrier starts
growing from the first turn of Defense Forme. Counterplay costs nothing but a moveslot.

### Speed Forme — survive the blitz

Three separate expressions of "it moved first", none of them faked:

- **Blitz.** Every turn, *before your move resolves*: 2–8% of your Pokémon's max HP, straight
  through Protect and screens. *Deoxys strikes before you can even move!*
- **Permanent Tailwind** on its own side, plus 180 base Speed.
- **A 25% chance to dodge** — a real Protect, so Feint and never-miss moves work exactly as they
  should. *Deoxys is moving too fast to be hit!*

It also carries **Extreme Speed**. In exchange it has the spec's "extremely low defensive tolerance":
1.11x damage, and it self-terminates because nothing outspeeds it once it is there.

### Attack Forme — the Assault meter

A counter climbs one per turn while it is in Attack Forme. You get a warning at 2:

> *Deoxys' offensive cells are massing.*

and at 3 it spends:

> *Deoxys' assault reaches critical mass!*

6–16% of your Pokémon's max HP depending on Stress, again through Protect and screens. This is the
reason not to simply park in front of Attack Forme and race it — it is the forme you most want it
in, and the one that punishes you hardest for staying there.

### The phases

| # | Name | Band | What changes |
| --- | --- | --- | --- |
| 0 | *The Organism* | 100–50% | Reconstructs every 3 turns (2 in Normal Forme). Opens in Normal Forme |
| 1 | *Rapid Mutation* | < 50% | **Every 2 turns** — the structural floor. Selection rule 3 comes online |
| 2 | *Perfect Adaptation* | < 25% | Two **Hybrid Mutation** beats |
| 3 | *Cellular Collapse* | ≤ 10% | The catch window |

#### Hybrid Mutation (Phase 2)

Twice, and only twice. *Deoxys combines two formes at once!*

| At | Banner | What it is |
| --- | --- | --- |
| **28% HP** | **ASSAULT VELOCITY!** | Attack Forme's offences behind Speed Forme's turn order — Tailwind, +80% raw Speed, a 12% hit, and a 5% heal |
| **15% HP** | **ARMORED RETALIATION!** | Defense Forme's bulk with offences that threaten — +60% raw Atk and Sp. Atk, two-thirds of a barrier already up, a 10% hit, and a 5% heal |

Both cost it a point of Stress, so reaching Phase 2 with Stress already high hands you an
instability window mid-climax. The raw stat boosts survive until the next ordinary reconstruction
wipes them.

#### Cellular Collapse — the Final Twist

> *Deoxys' cells begin to destabilize!*

It rips through all four formes in seconds — Attack, Defense, Speed, Normal — and then:

> *DEOXYS' CELLS CAN NO LONGER ADAPT!*

Everything stops. It is locked in Normal Forme, its Defense and Sp. Defense are cut by 30%, the
guard drops to 80%, and the move immunities come off.

### Catching it

Poké Balls are blocked for the entire fight until the collapse at **10% HP**:

> *Deoxys settles back into its first shape, unable to build another.*
> *It's spent - now is the moment to catch it!*

**The collapse deliberately ends on Normal Forme**, because the forme Deoxys is in when you catch it
is the forme you keep. You will never be handed an Attack Forme by accident.

### What it fights with

| Move | Why |
| --- | --- |
| **Psychic** | the reliable STAB, and the only sane output Defense Forme has |
| **Psycho Boost** | its signature, 140 BP — and self-limiting, since the Sp. Atk drop means even Attack Forme cannot spam it |
| **Extreme Speed** | priority, and the AI's answer to you trying to finish it off at low HP |
| **Superpower** | Fighting coverage into the Dark and Steel types you will bring against a Psychic boss, and terrifying off Attack Forme's 180 |

It runs the smart-trainer AI, and **Pressure** burns your PP the whole way.

### Anti-cheese

- **Sleep and freeze are refused, not wasted.** They cost Deoxys Stress each time it re-forms around
  them — but capped at Stress 7, so status alone can never buy you an instability window.
- **Poison, burn and paralysis all stick** and matter normally. Toxic stall is bounded by flat
  damage rather than immunity, so chip strategies stay legal but cannot outscale a long fight.
- **Setting up is an input to the fight.** Boosting Speed pulls it into Speed Forme, where it
  outspeeds you anyway and chips you every turn before you act.
- **One-move spam** is answered by the adaptive barrier — that is the entire point of it.
- **Protect and screens** stop none of the Blitz, the Assault or either hybrid strike.
- **Stalling the timer** does not work either: doing nothing notable makes it choose Normal Forme,
  which reconstructs *faster* than the others.
