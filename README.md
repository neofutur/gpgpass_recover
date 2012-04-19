gpgpass_recover
===============

need to recover your gnupg passphrase, here is the tool                               Rephrase
-------------

 forked from http://www.roguedaemon.net/rephrase/

Version 0.1


    README (what it does, what it doesn't, how to use it, how to complain)
    Copyright (C) 2003  Phil Lanch

    This file is part of Rephrase.

    Rephrase is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    Rephrase is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA


    $Id: README,v 1.3 2003/09/03 19:59:18 phil Exp $


About
-----

Rephrase is a specialized passphrase recovery tool for GnuPG[1].  If you
can nearly remember your GnuPG passphrase - but not quite - then
Rephrase may be able to help.

Tell Rephrase the parts of the passphrase you know, and any number of
alternatives for the parts you're not sure about; and Rephrase will try
all the alternatives, in all possible combinations, and tell you which
combination (if any) gives you the correct passphrase.  You could try
all the combinations yourself, of course, if there are just a handful of
them; but if there are more, that might be impractical.

On the other hand, if you need to try a huge number of possible
passphrases, Rephrase might be too slow; it is far from being an
efficient passphrase cracker.  E.g. Rephrase can try out about 2600
possible passphrases per minute on my 1GHz Athlon (with other processes
doing nothing very heavy at the same time).  How many passphrases
Rephrase can try depends on how long you are prepared to wait!  Rephrase
can reasonably be run for a long time; e.g. it *won't* use more memory
the longer it runs.

It would be a Bad Thing to leave your passphrase (or part of it, or your
guesses at it) lying around on your hard drive; since a passphrase is
supposed to be an extra line of defence if an attacker obtains access to
your secret keyring (which you presumably *do* keep on your hard drive).
That's why Rephrase keeps all the information about your passphrase that
you give it in secure memory (and then pipes each possible passphrase to
a child gpg process).  For this reason, Rephrase is likely to be more
secure than alternative solutions that involve generating a list of
possible passphrases in a file and then testing them.

[1]For more information about GnuPG, see http://www.gnupg.org/ .


Prerequisites
-------------

* GnuPG

* C compiler

* POSIX (i.e. a Unix-like system)

For portability issues, see below, under "Bugs".


Installation
------------

Um, did you download and unpack the tarball?

It takes 2 commands to install Rephrase.

(1) make

    There are 2 arguments you might need to add.

    (a) If gpg is not installed at /usr/local/bin/gpg, then you need to
        specify its full path e.g.

        make GPG=/usr/bin/gpg

    (b) If you might want to type in a pattern (patterns are explained
        below, under "Manual") longer than 512 characters, then you need
        (help and) to specify a maximum pattern length e.g.

        make PATTERN_MAX=1024

    So if both (a) and (b) apply, then this command could become e.g.

        make GPG=/usr/bin/gpg PATTERN_MAX=1024

    If you need to re-make with different arguments, then you first need
    to

        make clean

(2) make install

    You *must* run this command as root.

    There's 1 argument you might need to add.  If you don't want
    rephrase installed in /usr/local/bin, then specify an alternative
    installation directory e.g.

        make install BINDIR=/usr/bin


Manual
------

Usage:

    rephrase <key>

where <key> is the key whose passphrase you want to recover; you can
identify the key in any of the ways that GnuPG understands.  (To make
sure you're using a sensible value for <key>, you could first try

    gpg --list-secret-keys <key>

which should list exactly 1 key.)

You will be prompted to enter a pattern (the pattern is not echoed to
the screen as you type it).  So what's a pattern?  Suppose you know that
your passphrase was something like "super-secret", but you're not sure
if you changed some (or all) of the "e"s into "3"s, or any of the
consonants into upper case, or indeed changed the "c" into "k" or "K" or
even "|<", or changed the "-" into " " or just omitted it.  Then you
could enter this pattern:

    (s|S)u(p|P)(e|3)(r|R)(-| |)(s|S)(e|3)(c|C|k|K|\|<)(r|R)(e|3)(t|T)

The pattern is your passphrase - except that 4 characters have special
meanings.  Brackets - "(" and ")" - are used to group alternatives
wherever you're not sure what characters are correct; "|" is used inside
a pair of brackets to separate the alternatives; and "\" is used to
escape any of the 4 special characters when you need to use it
literally.

Rephrase will tell you if your pattern contains a syntax error.  That
happens if there are unbalanced brackets (i.e. they aren't in proper
pairs); or if the pattern ends with "\" (because then there's nothing
for it to escape).  It also happens (and these cases are limitations in
Rephrase's simple pattern parser) if you try to nest pairs of brackets;
or if you try to use "|" anywhere that's not inside a pair of brackets.

If the pattern contains no syntax errors, Rephrase will try each
possible passphrase matching the pattern in turn.  If the correct
passphrase is found, Rephrase won't actually tell you what it is (in
case someone's looking over your shoulder), but will tell you a string
of numbers: you can work out the correct passphrase from these numbers
and the pattern you entered.  E.g.

    2 1 2 1 2 1 1 5 1 2 2

The first number - 2 - means that at the first pair of brackets in the
pattern - "(s|S)" - you must take the second alternative - viz. "S".
The second number - 1 - means that at the seconds pair of brackets -
"(p|P)" - you must take the first alternative - viz. "p".  And so forth.
So in this case the correct passphrase is "Sup3r se|<r3T".

If the correct passphrase is not found from the pattern, Rephrase tells
you so.  (Note that you will also get this result if you specified <key>
incorretly; how to check that the value of <key> is OK is explained
above.)

Rephrase's exit status is 0 is the passphrase is found, 1 if it's not
found, or other values if an error occurs.


Security
--------

The good news is that Rephrase uses mlock() in order to keep the
information about passphrases that it's given as secure as possible.
The bad news is that using mlock() requires root privileges, so Rephrase
needs to be setuid root.  However, it does drop root privileges very
quickly, as soon as it has called mlock().

It's also debatable whether mlock() is a proper way to protect sensitive
information.  According to POSIX, mlock()ing a page guarantees that it
*is* in memory (useful for realtime applications), not that it *isn't*
in the swap (useful for security applications).  Possibly an encrypted
swap partition (or no swap partition) is a better solution.  Anyway,
GnuPG itself uses mlock(), which makes it sensible for Rephrase to
follow suit.


Bugs
----

Portability is untested: I have only used Rephrase on a GNU/Linux system
(Linux 2.4.21 and Glibc 2.3.2; building with GNU Make 3.79.1, bash 2.05
(as /bin/sh) and either GCC 2.95.3 or GCC 3.3).  I believe setreuid() is
a BSD-ism, so it may not exist on more SysV-like systems.  There are
probably many other issues.

If mlock() fails (probably because Rephrase is not setuid root),
Rephrase refuses to proceed: it would be better to issue a warning and
continue, since that's what GnuPG does.

Before it asks you to enter a pattern, Rephrase should check that the
<key> argument does refer to exactly 1 key and that that key is
available.

If you'd like Rephrase to be faster, then it's too slow.  (But if you're
happy with it, then it's fast enough.)

The standard --version and --help options are unimplemented.

Please send bug reports to me at

    Phil Lanch <phil@subtle.clara.co.uk>

I'm especially interested in reports of

* successes or failures on different operating systems (including full
  details of the system and what didn't work)

* anyone who cares about the other bugs listed above (if you care about
  them, I might fix them) 

* new bugs
