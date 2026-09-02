## Uxie — The Being of Knowledge

Uxie fights behind the usual legendary package: a heavy damage reduction, immunity to OHKO,
fixed-damage, HP-swap and shared-KO moves, incoming type effectiveness capped at 2x, and flat Toxic
damage. It also **cannot be taken below 1 HP** until it is weakened, so you are not going to end
this one early or by accident.

Azelf asks whether you are hitting hard enough. Uxie asks something else entirely: **can it call
your move before you make it?**

You do not beat Uxie by being stronger. You beat it by being unpredictable.

### The core loop

```
Every turn, Uxie STATES OUT LOUD what it expects you to do.

    "Uxie foresees a blow struck with force."      it expects a PHYSICAL move
    "Uxie foresees a blow struck with will."       it expects a SPECIAL move
    "Uxie foresees that you will not strike."      it expects a STATUS move

Then it grades what you actually did.

    you did it     ->  "Just as Uxie foresaw."           KNOWLEDGE +1
                                                          and it FILES the type you hit it with
    you defied it  ->  "Uxie's prediction was wrong!"    KNOWLEDGE -1
                                                          and it LOSES its newest filed type
```

The read is not a script. It is the AI genuinely asking *"what would I do if I were holding that
Pokémon?"* — which, in practice, means **it expects your best move**.

### Knowledge

Knowledge runs 0 to 5. Every rung is announced, in both directions, and every rung buys Uxie the
same two things: **it hits harder and it moves sooner**. It never gets bulkier — with base 130
defenses on both sides it does not need to.

| Knowledge | The board it may hold | Sp. Atk | Speed |
| --- | --- | --- | --- |
| 0 | *nothing at all* | — | — |
| 1 | 1 type at 40% | +1 | — |
| 2 | 2 types at 45% | +1 | +1 |
| 3 | 2 types at 55% | +2 | +1 |
| 4 | 3 types at 60% | +2 | +2 |
| 5 | 4 types at 70% | +3 | +2 |

### The board

Every time Uxie calls your move correctly, it **files the type you hit it with** and takes far less
damage from that type from then on. It says so by name each time:

- *"Uxie files away the shape of {type}."* — a new type went onto the board.
- *"Uxie already knows {type}."* — you fed it the same type again; the reduction got worse.
- *"Uxie sets aside what it knew of {A} to make room for {B}."* — the board was full and the oldest
  entry fell off.
- *"Uxie loses its grasp on {type}!"* — you defied it, and the newest entry came straight back off.

**You never touch the board directly.** You decide whether Uxie is allowed to have one at all. Defy
every read and it fights you at Knowledge 0 with nothing filed. Play your obvious best move every
turn and by the second half you will be swinging into four types at 70% *on top of* the flat
reduction.

A foreseen **status** move raises Knowledge but files nothing — *"Uxie foresaw even your
hesitation."*

### THE TEST

Every fifth turn, Uxie asks instead of announcing:

```
Uxie challenges your knowledge!
It foresees a blow struck with force.  Will you prove it right?
                    > YES    NO
```

Now it is grading **your word against your deed**, not your move against its prediction.

| | |
| --- | --- |
| **You kept your word** | *"Your word and your deed agree."* Knowledge **+1**, and it files, exactly as a correct read |
| **You lied** | *"Uxie cannot reconcile your words with your actions!"* Knowledge **−2**, it drops its newest filed type — **and your Pokémon loses its next turn** to the backlash |

Lying is a wager, not a free lever. Telling the truth costs you nothing and hands Uxie a rung.
Both are real answers.

### Something it has never seen

Uxie is building a model of you, and that model has a hole in it: **a damage category you have
never once used all fight**.

The first time you use a category, Uxie says so — *"Uxie has never seen this from you!"* — and it
costs it an extra rung of Knowledge. Early on that is a small bonus and a large hint.

**Keep one category in your pocket.** You will want it later.

### Phases

| HP | Phase | What changes |
| --- | --- | --- |
| 100–50% | **STUDY** | Knowledge cannot climb past 3, so the board tops out at 2 types |
| 50–25% | **COMPLETE UNDERSTANDING** | Correct reads now pay **+2**, and the ceiling lifts to 5 |
| 25–10% | **OMNISCIENCE** | Knowledge is **pinned at 5** and defiance no longer lowers it |
| ≤10% | **WEAKENED** | The catch window |

### Breaking OMNISCIENCE

At 25% Uxie closes its eyes and stops needing to watch you. Ordinary defiance no longer moves the
meter — it only strips an adaptation. There are exactly two ways through:

1. **The card you held back.** Land a move of a damage category Uxie has never seen all fight.
   Instant.
2. **Three in a row.** Defy the live read three consecutive turns. A switch, a turn where Uxie takes
   no reading, or one correct read resets the count to zero.

Either one causes a **COLLAPSE**:

```
"Uxie cannot comprehend what just happened!"

  Knowledge 5 -> 0     the entire board, wiped
  its damage reduction collapses for THREE FULL TURNS
```

Every turn of that window it tells you the window is still open: *"Uxie is still reeling. It is not
reading you."* When it closes, Uxie re-forms at Knowledge 5 and you build another opening. It is a
second chance, not a rhythm to farm — the window is your best damage in the whole fight, so spend
all three turns on damage.

### Anti-cheese

**Sleep does not stick.** Uxie throws it off at the end of the turn — *"Uxie's mind cannot be
dimmed"* — and the attempt itself teaches it something. Burn, paralysis, poison and freeze all work
normally; it is Uxie's *mind* that cannot be dulled, not its body.

Ground moves do nothing — Uxie has **Levitate**.

One of your Pokémon fainting also teaches it a rung. The loss is data too.

### Its moves

**Future Sight**, **Psychic**, **Dazzling Gleam**, **Shadow Ball**.

Dazzling Gleam and Shadow Ball are aimed squarely at the Dark- and Ghost-types you will reach for,
those being two of Uxie's three weaknesses. It has **no answer to Bug at all** — which is exactly
why Bug is the type it will most want to file away.

### The catch

You cannot throw a Poké Ball at any point before the end. At **10% HP in OMNISCIENCE** Uxie's eyes
lose their focus, everything it built is dropped, and balls unlock.

### Summary

- It expects your **best** move. Winning means not using it.
- Its resistances exist only because you let it earn them.
- Keep one damage category unused for the whole fight.
- Lie to it at most when you can afford to lose a turn.
- The collapse window is three turns. Spend all three hitting it.
