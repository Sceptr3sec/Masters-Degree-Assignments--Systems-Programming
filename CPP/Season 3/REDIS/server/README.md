# Welcome to My Cpp Redis Server
***

## Task
Build a Redis-compatible server that handles TCP connections, parses RESP protocol commands, and manages an in-memory data store with persistence

## Description
Implemented RESP protocol parser, command dispatcher for multiple data types, in-memory storage engine, expiry handling, and optional persistence to dump files

## Installation
make

## Usage
./main
./main --port 6380
./main --dump mydata.rdb

