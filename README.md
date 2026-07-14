# ORB Trailing Stop — C++ Port

A standalone C++ port of the quad trailing-stop logic from **Project Hail Mary**,
my live intraday algorithmic trading system (Python, Zerodha Kite Connect API)
running an Opening Range Breakout strategy on the BANKBEES ETF.

Main system: [Project Hail Mary](https://github.com/sree-ved-soma/Project-Hail-Mary)

## What this is

This is a direct, logic-for-logic port of the trailing-stop decision engine —
no live API calls, no I/O beyond console output, just the math and the state
machine that decides when and how far to ratchet a stop-loss forward as price
moves in the trade's favor.

The live system runs on 5–15 minute candles, where Python's execution speed
is not the bottleneck — API latency and development speed matter far more.
This port exists to explore what the decision logic would look like under a
lower-level, faster language, since execution speed becomes a genuine
engineering constraint at lower timescales in real trading infrastructure.
