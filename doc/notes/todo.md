# List of features to be implemented or fixed

* Issue #324: Fix `tsswitch --delayed-switch --receive-timeout`.

* Rewrite sinfl.h and sdefl.h with clean code. There are multiple issues
  with that code. Some of them were fixed but Claude refuses to perform
  a security audit on it, assuming that it can be used to lauch an attack.
  This is probably a symptom that many security problems exist.
