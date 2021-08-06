# Busy Beaver Finder

This program searches for Busy Beavers programs in (a variant of) the [2L](https://esolangs.org/wiki/2L) language.

## Language

The language is two-dimensional.
The instructions are put on a 2D grid whose size is limited.
The directional program pointer (PP) moves across this grid, executing instructions as it encounters them.
It starts just below the left-most cell at the bottom of the grid, moving upwards.

There is also a one-dimensional, infinite tape that acts as data.
All cells of the tape are initially filled with zeros.
The data pointer (DP) points to one of these cells.

The language consist of only two symbols:

*   **TURN**, visualised by a Black circle

    When PP would enter a cell with a TURN instruction, it changes direction to prevent this.
    The direction it turns to depends on the value that DP points to:

    *   When the value is zero, PP turns ninety degrees counter-clockwise.
    *   Otherwise, PP turns ninety degrees clockwise.

*   **DATA**, visualised by a White circle

    What happens when PP enters a cell with a DATA instruction depends on the direction of PP:

    *   When it is moving up, it increases the value at DP by one.
    *   When it is moving down, it decreases the value at DP by one.
    *   When it is moving right, it moves DP one position to the right.
    *   When it is moving left, it moves DP one position to the left.

Program cells can also be empty, representing a NOOP instruction;
PP simply keeps moving in its current direction.

The program terminates when PP leaves the board.

### Variations

The differences of the above language with respect to the original 2L language are:
*    the data tape does not have cells for I/O operations
*    the data tape is infinite in both directions
*    the mapping of PP direction to the actual operation is changed for the DATA symbol (to be more logical)
*    the program space is bounded

## The Busy Beaver Challenge

Find a program that executes for as long as possible, yet terminates.

## More information

For a detailed description of the search algorithm and its current results check my write-up
[The Quest for 2LBB Busy Beavers](https://bonsma.home.xs4all.nl/BusyBeavers2L/index.html).
