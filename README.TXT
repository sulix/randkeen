===========================
Commander Keen 1 Randomiser
===========================

Author: David Gow <david@davidgow.net>
Version: 1.0c
Website: https://davidgow.net/keen/randomiser.html


About
-----

The Keen 1 Randomiser lets you play Keen1 with a twist: items, levels, and
enemies are all randomised.

The randomiser takes a copy of Keen 1, and rewrites the level files to give you
a new experience. It also creates a CK1PATCH-compatible patch file which loads
the replacement files and patches the random number seed in.

What does it change?
--------------------

- Entrances to all levels on the world map are randomised.
- Positions of key items (spaceship parts and pogo stick) are randomised.
- Enemy positions are shuffled (within a level).
- Point items (lollies) are shuffled within a level.
- Door/Keycard colours are randomised (cosmetic only).
- Some block colours are randomised.
- Yorp statues now hint at item locations.
- You can see the random number seed on the Status Screen (press Space)

Note that there are some things which don't change:
- Level layouts remain the same.
- Raygun ammo remains where it is in the original game.
- There is always a Pogo Stick at the bottom of the Vorticon Commander's Castle
- The Garg statue remains unhelpful.
- The world map looks identical (but the levels within are different)

How do I use it?
----------------

Just run RNDKEEN1.BAT: a differently-randomised version of the game will greet
you each time you play.

If you want to replay the same game (or have several people play the same
randomised version), you can pass a random number seed to the randomiser:
C:\RANDKEEN> RNDKEEN1 /SEED 12345
(Replace 12345 with the seed you want to play.)

You can check what seed you're running from the Status Screen by pressing SPACE.

If you want a somewhat easier or different game, you can use some of the
other options below. For example:
C:\RANDKEEN> RNDKEEN1 /STARTAMMO 15 /EXTRAPOGO 6

If you just want to generate the game, not play it, you can run RANDKEEN.EXE:
this will generate the randomised maps and patch file. Note you'll still need
to rename LEVEL81.CK1 and LEVEL90.CK1 to RNDLV81.CK1 and RNDLV90.CK1
respectively for the world map and ending screen to work (this is usually done
by the RNDKEEN1.BAT file).

RNDKEEN1.BAT and RANDKEEN.EXE accept some extra command-line parameters:
- /? -- Show a help screen listing these parameters.
  (Use this with RANDKEEN.EXE only, as the game starts anyway.)
- /SEED <seed> -- set the random number seed to <seed>
- /DEBUG -- show additional debug information (can contain spoilers!)
- /NOLEVELNAMES -- Make Yorp hints use level numbers instead of names
- /NOENEMYSHUFFLE -- don't shuffle enemy positions
- /STARTPOGO -- start with the pogo stick.
- /STARTAMMO <n> -- start with <n> ammo.
- /EXTRAPOGO <n> -- hide <n> extra Pogo sticks around the game.

The Keen 1 Randomiser requires a 386 or better to run, and probably needs a
megabyte or so of memory.

Hints!
------

Playing Keen 1 Randomiser is, for the most part, very similar to playing Keen1,
but there are a few things to be wary of.

- You'll probably need to play through levels which are not necessary to win
  the original game. Familiarise yourself with them.
- The Pogo Stick is much harder to get: you'll probably need to complete a
  lot of levels without it!
- Ship Parts and the Pogo Stick will always be either in their original
  locations, or right next to the exit. You can't miss them!
- Raygun Ammo is where you remember it from the original game, but because
  you'll be playing levels in a different order, you'll probably find ammo
  hard to find. Use it wisely (and get good at dodging!)
- Shrine levels may contain Yorp statues, which have hints. You'll want to know
  the level names[1] to take advantage of these. (Or, if you know the level
  numbers, you can use the /NOLEVELNAMES switch.)
- There's always a Pogo Stick in the Vorticon Commander's dungeon.
- If you're struggling without the Pogo Stick, you can try the /STARTPOGO
  option, which will make you always start with a Pogo, or the /EXTRAPOGO <n>
  option, which will hide an extra <n> Pogo Sticks around the game.
- Similarly, you can start with extra ammo by using the /STARTAMMO <n> option
  to begin with <n> shots.

[1]: http://www.shikadi.net/keenwiki/Keen_1_Levels

Building the Source Code
------------------------

If you want to modify Keen 1 Randomiser (or just see how it works), the source
code is included in this archive (randkeen.c), and is also available on GitHub:

https://github.com/sulix/randkeen

It's normally compiled with DJGPP, but can also be built for non-DOS platforms
(just compile randkeen.c with your favourite compiler: it should be reasonably
standard C99).

Make sure that CK1PATCH and CWSDPMI are copied to the Keen1 directory, as well
as the RNDKEEN1.BAT and RANDKEEN.EXE files. There's an included shell script
which compiles RANDKEEN.EXE and does this, though it has some hardcoded paths
in it.
