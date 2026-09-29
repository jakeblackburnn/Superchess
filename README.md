# Superchess 

Superchess is a superset of the chess ruleset (see superchess.md). This project seeks to implement superchess as 
a C library with a easy to use UI made in Python to learn and get practice writing C code. Once the core 
infrastructure is in place, I hope to develop a AlphaZero-style Superchess engine (SuperAlphaZero) to learn more about RL.

Update: Sept 29 2026

Training infrastructure is implemented! Unfortunately it would take several years to actually train a 
decent chess bot on my measly GPU (calculated based on an estimate of total flops used to train AlphaZero) 
let alone a tabula-rasa superchess bot, so no self play model :(

In future I want to try to bootstrap a decent chess bot using datasets to pre-init the policy net
and see if decent bots become practical.
