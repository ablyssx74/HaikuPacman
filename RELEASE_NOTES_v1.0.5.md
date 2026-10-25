# HaikuPacman v1.0.5

## Highlights
- **The version comes from the `VERSION` file.** The program's version text (what the update check compares with the `VERSION` file on GitHub), the resource and the package all take it from there now, as hDesktop and Cricket do, so a release changes one file. The number was typed into the code, the Makefile and the resource file before, and one of them could be left behind.
- **The update check looks for a newer version, not a different one.** It compared the version on GitHub with its own as two pieces of text and offered an update whenever they were not the same, so it would also have offered one for an older version. It compares the numbers now (1.0.4 against 1.0.5), as the other programs do.
- The resource's version numbers were in the wrong fields (the third number in the second place); they are right now.

## Install
Install `HaikuPacman-1.0.5-1-x86_64.hpkg` on 64-bit Haiku, or `HaikuPacman-1.0.5-1-x86_gcc2.hpkg` on 32-bit Haiku, for the whole system.

## Tested on
- Haiku R1/beta6: the 64-bit and 32-bit packages were built (on 64-bit Haiku and on 32-bit Haiku) and show v1.0.5 in the program and in the package. They were not run.
