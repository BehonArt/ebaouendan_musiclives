<!-- DRAFT for review. Title: -->
# recomp.discover: 0 functions when a program needs more than 4 refuse passes

## What happens

`python -m ndskit.recomp.discover` on Elite Beat Agents (USA, AOSE, SDK 3.1,
no overlays) prints:

    0 functions (0 ARM, 0 Thumb), 0 instructions

with no error, and `python -m ndskit.recomp` emits an empty ARM9 module.
The ARM7 of the same cartridge, and two other cartridges of the same
developer (Ouendan, AOSJ; Ouendan 2, AO2J), discover normally.

## Why

`Program.run()` (recomp/discover.py) loops `for _ in range(4)`. Each pass
refuses the guessed entries whose descent met data, then clears `entries`,
`funcs`, `code`, `data` and the rest so the next pass starts clean. If the
fourth pass still finds bad guesses, it refuses them, clears everything, and
the loop ends there: the program is left with nothing.

This ARM9 needs 6 passes. Over them 15 guessed entries are refused, and the
7th pass ends with no bad guess: 4,517 functions, 0 conflicts.

## A fix

Loop until a pass finds no new bad guess. That ends: a refused entry is
never guessed again, so each pass that goes on refuses at least one more
of a finite set. Failing that, never clear after the last pass, so a
program needing more passes than the cap keeps what it found, conflicts
and all, instead of nothing.

What we run locally (a cap of 40, never clearing after the last pass, and
the number of passes printed):

```diff
@@ class Program:
         self.refused = set()
-        for _ in range(4):
+        self.refuse_passes = 0
+        passes = 40
+        for i in range(passes):
             self.guessed = set()
             self._rounds(max_rounds)
             bad = {a for a in self.guessed if a in self.funcs and self.funcs[a].conflicts}
-            if not bad:
+            # never wipe after the last pass: a game needing more passes than
+            # the cap keeps what it found (with its conflicts) instead of nothing
+            if not bad or i == passes - 1:
                 break
+            self.refuse_passes += 1
             self.refused |= bad
             self.entries, self.crossing, self.switches = {}, set(), {}
             self.literal_loaded, self.funcs, self.code, self.data = set(), {}, {}, set()
@@ def main(argv=None):
         text, conf, unres = prog.summary()
         print(text)
+        if prog.refuse_passes:
+            print("refused %d guessed entries over %d passes" % (len(prog.refused), prog.refuse_passes))
```

## Checked

On the three cartridges above (ARM9 and ARM7 of each, and Ouendan 2's seven
overlays, one at a time): only Elite Beat Agents' ARM9 needs refuse passes (6);
the others need none and discover exactly as before. With it, Elite Beat
Agents' 4,517 functions include all 25 entries we had found by hand
(a vtable's 10 methods among them), and the program emits, passes `selftest`
(1,428 ARM9 functions with 21,402 vectors, 0 failures) and runs in `ndsrun`
from its boot into gameplay. Not checked on Moon, Nostalgia, Lufia or the
two cartridges of ndskit's own discovery check.
