# Redis Server

A simplified Redis-compatible server implementation in C++.

## Contents

- `main.cpp` — server executable with RESP request parsing, response serialization, in-memory datastore, persistence, and TCP socket handling.

## Summary

This folder contains a small Redis-style server that accepts RESP protocol commands over TCP, stores string/list/hash values in-memory, and optionally persists data to a dump file. It includes basic command dispatch, expiry handling, and network server logic.
