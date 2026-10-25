# Design

The engine owns orders and books. The gateway will own sessions; the publisher
will own the feed. Queues carry requests and reports between them.

Open: the resting order cumulative fill is not updated yet.
