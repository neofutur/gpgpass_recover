# Makefile (for compiling and installing, or building a distribution)
# Copyright (C) 2003  Phil Lanch
#
# This file is part of Rephrase.
#
# Rephrase is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2.
#
# Rephrase is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

# $Id: Makefile,v 1.1 2003/09/03 05:02:24 phil Exp $

SHELL		= /bin/sh

program		= rephrase
version		= 0.1

what		= $(program)-$(version)

GPG		=
ifneq (,$(GPG))
gpg_def	= -DGPG=\"$(GPG)\"
else
gpg_def	=
endif

PATTERN_MAX	=
ifneq (,$(PATTERN_MAX))
pattern_max_def	= -DPATTERN_MAX=$(PATTERN_MAX)
else
pattern_max_def	=
endif

CFLAGS		= -Wall -DVERSION=\"$(version)\" $(gpg_def) $(pattern_max_def)

BINDIR		= /usr/local/bin

files		= COPYING Makefile README $(program).c

all: $(program)

dist: $(what).tar.gz $(what).tar.bz2

install: all
	mkdir -p $(BINDIR)
	rm -f $(BINDIR)/$(program)
	cp $(program) $(BINDIR)/$(program)
	chmod 4711 $(BINDIR)/$(program)

$(what).tar: $(files)
	rm -f $@
	rm -rf TREE
	mkdir TREE
	mkdir TREE/$(what)
	cp -p $(files) TREE/$(what)
	cd TREE && { tar -c -f ../$@ $(what) || { rm -f ../$@ && exit 1; }; }

$(what).tar.gz: $(what).tar
	rm -f $@
	gzip -c -9 $< > $@ || { rm -f $@ && exit 1; }

$(what).tar.bz2: $(what).tar
	rm -f $@
	bzip2 -k -9 $< || { rm -f $@ && exit 1; }

clean::
	rm -f $(program) $(program)-*.tar
	rm -rf TREE

distclean::
	$(MAKE) clean
	rm -f $(program)-*.tar.gz $(program)-*.tar.bz2
