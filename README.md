# Course Computer Graphics

A brief course to understand why and how simple mathematics is a very powerful tool on code running on computers. Smart code was found during several decades by countless of people. Today we use them as a fundation to build other one.

OS: Windows
Tools needed: MSVC Compiler from visual studio community (any version)

`listing_1` has the squeleton for the "student" to implement the two algorithms:
- by rounding (naive way)
- Beresenham one octant only (-1 < m < 1)

`listin_2` has possible solutions of both algorithms. Beresenham has on (-1 < m < 1), because the algorithm can be very hairy for no extra revelation. One octant is clean and easier to understand.

`listin_3` test both algorithms in order to understand the efficiency of Beresenham over the naive one

`listing_4` (to be made) Beresenham all octants ?

# how to build the exe files

on command line -> go inside the root folder then execute `build`.
You can build individually by executing `build_single`

Each build will create two executanle -> `listing_X_rm.exe` and `listing_X_dm.exe`.
`_rm` is for release (optimized)
`dm` is for debug (with `.pdb` files to be used inside a debugger)
