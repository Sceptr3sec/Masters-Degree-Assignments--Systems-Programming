# Welcome to My Mouse
***

## Task
The challenge is to find the shortest path through a maze given as a text file, where the maze layout, special characters (walls, empty space, path marker, entrance, exit) are all defined dynamically in the file's header — nothing is hardcoded. The difficulty lies in correctly parsing arbitrary character sets, validating the map, and applying a shortest-path algorithm with precise tie-breaking rules.

## Description
The program parses the maze header to extract dimensions and the 5 special characters, validates the map (exact row widths, exactly one entrance and one exit, only legal characters, must have a solution), then solves it using BFS (Breadth-First Search).
BFS guarantees the shortest path. Neighbours are explored in order up → left → right → down, which naturally implements the required tie-breaking: when two paths are equal length, prefer the one that goes up first, then left, then right, then down. Once the exit is reached, the path is reconstructed by walking backwards through a predecessor array, replacing empty cells with the path character along the way.
Invalid maps print  to stderr and the program moves on to the next file, exiting with code 1 if any map failed.

## Installation
How to install your project? npm install? make? make re? One would install it with make

## Usage
How does it work? It works with ./my_mouse map01