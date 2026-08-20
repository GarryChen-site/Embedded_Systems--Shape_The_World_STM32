Mapping: 

- Physical **CH1** → Logic2 **Channel 0** → **PA5** (Ready)
- Physical **CH2** → Logic2 **Channel 1** → **PA6** (VT)

---

## Waveform Verification ✅

The behavior matches the 9-step pacemaker sequence perfectly:

**Channel 0 (Ready / PA5):**
```
HIGH ────┐         ┌─── HIGH ───┐         ┌── HIGH
         └── LOW ──┘            └── LOW ───┘
         ↑button    ↑VT done     ↑button    ↑VT done
```
- Starts **HIGH** (waiting for heartbeat) ✅
- Drops **LOW** when B1 pressed ✅
- Goes back **HIGH** only after the full VT pulse ends ✅

**Channel 1 (VT / PA6):**
```
LOW ────────────────┐     ┐
                    └─┘   └─┘   ← ~250ms pulses
```
- Stays **LOW** normally ✅
- Brief **HIGH pulse** (~250ms) after button release + 250ms A-V delay ✅

---

