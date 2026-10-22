<!-- DRAFT for review. Title: -->
# recomp.discover: the rule of opposite conditions is applied across a call (`bne ...; bl _dcmpeq; beq ...`)

## What happens

On Ouendan 2 (AO2J, SDK 3.2), `python -m ndskit.recomp` reports two jumps
to targets that are not entries:

    ARM9: target 02023540 (from 020234B8) is not an entry of ARM9
    ARM9_OV6: target 020B5134 (from 020B50CC) is not an entry of ARM9_OV6

and emits them as `arm_jump` to a fixed address:

```cpp
    c.r[14] = 0x0202353Cu; ARM_POLL(c); f_02028248(c); ARM_RET(c, 0x0202353Cu);  // 02023538 bl 0x02028248
    if (c.z) { goto L_0202357C; }  // 0202353C beq 0x0202357C
    arm_jump(c, 0x02023540u); return;
```

The code from 0x02023540 on is ordinary code of the same function, cut off.

## Why

The function at 0x020234B8 is MSL's `strtod` (by its shape: `__strtold`
with a string reader, `*endptr` set, then `errno = ERANGE` from range
checks against 0.0, `DBL_MIN` and `DBL_MAX`):

    02023520  cmp r2, #0
    02023524  bne 0x02023570          ; overflow: errno = ERANGE
    02023528  mov r0, #0
    0202352C  mov r1, r0
    02023530  mov r2, r4
    02023534  mov r3, r5
    02023538  bl 0x02028248           ; the soft-float double compare, result in the flags
    0202353C  beq 0x0202357C          ; value == 0.0: return
    02023540  mov r0, r6              ; <- never walked
    ...

0x02028248 is CodeWarrior's soft-float double equality compare: it ends in
`cmp r1, r3; cmpeq r0, r2; ...; bx lr` and its callers branch on the flags
it returns.

In `_walk_once`, `ended` (the condition of the last conditional transfer,
"while the flags hold") is cleared only by an instruction with `s` set or a
compare. A `bl` leaves it, so `beq` after the call is taken as the
opposite of the `bne` before it "on the same flags", the block ends at the
`beq`, and the fall-through is never walked. Overlay 6's 0x020B50CC is the
same pattern with the sibling compare at 0x020281BC (`bne ...; ldm; mov;
mov; bl 0x020281BC; beq ...`).

At run time the `arm_jump` to a non-entry sets `c.pc` and returns; the
caller's `ARM_RET` then sees the wrong pc, and `arm_bad_return` stops the
machine (`returned to 02023540`), for `strtod` whenever the value it read is
not 0.0. That is from reading the runtime, not seen: a build without the fix
plays our route from the boot into a song and its fail screen without
stopping (`strtod` is never called on it; overlay 6's function is, from the
title screen on, without taking the lost path). It is the emitted code that
is wrong either way.

## A fix

A call ends the stretch of same flags: clear `ended` at `bl`/`blx`, and
after a call through a register that returns to the next instruction
(`mov lr, pc; bx r12`):

```diff
@@ output (the compiler of NitroSDK games) and the SDK's hand-written parts:
    when it is set). Two conditional transfers in a row with opposite conditions on
-   the same flags (`bxpl r12` then `popmi {..., pc}`) end the block:
+   the same flags, no call between them (`bxpl r12` then `popmi {..., pc}`;
+   a call may return in the flags, `bl _dcmpeq; beq`), end the block:
    nothing runs after them. That holds on a straight line only: when the
@@ class Program:
                 if cond != 14 and not go_on:
                     ruled.append(list(since))
-                if ins.s or op in arm.COMPARES:
+                if ins.s or op in arm.COMPARES or op in ("bl", "blx"):
+                    # a call's flags are not the ones before it: CodeWarrior's
+                    # soft-float compares return their result in them (`bl _dcmpeq; beq`)
                     ended, since = None, []
@@ class Program:
                         if not go_on:
                             break
                         ended, since = cond, []
+                    else:
+                        ended, since = None, []     # returns here, with the callee's flags
                     pc = nxt
                     continue
```

## Checked

Function lists before and after (`--out`, then compared entry by entry) on
14 programs: Elite Beat Agents (AOSE) ARM9 and ARM7, Ouendan (AOSJ) ARM9
and ARM7, Ouendan 2 (AO2J) ARM9 and ARM7 and its seven overlays one at a
time. The only differences are the two functions above, grown to their
whole length (0x020234B8 from 164 to 212 bytes, 0x020B50CC from 124 to 136);
no function lost, split or shrunk, 0 conflicts on all 14, and the other two
cartridges' lists are identical. After it, every target `python -m
ndskit.recomp` reports as not an entry on Ouendan 2 is a function of one of
its overlay modules (179 of 179). Ouendan 2 passes `selftest` (1,866 of its
functions, 28,690 vectors, 4,690 of them through the overlay dispatch, 0
failures) and plays from its boot into gameplay. Not checked on Moon,
Nostalgia, Lufia or the two cartridges of ndskit's own discovery check.
