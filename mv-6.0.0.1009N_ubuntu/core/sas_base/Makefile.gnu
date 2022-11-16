#
# (C) Copyright	2000-2003
# Wolfgang Denk, DENX Software Engineering,	wd@denx.de.
#
# See file CREDITS for list	of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it	under the terms	of the GNU General Public License as
# published	by the Free	Software Foundation; either	version	2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY;	without	even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See	the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public	License
# along	with this program; if not, write to	the	Free Software
# Foundation, Inc.,	59 Temple Place, Suite 330,	Boston,
# MA 02111-1307	USA
#

ifndef LOKI_TOPDIR
TOPDIR?= $(shell cd ../..; pwd)
LOKI_TOPDIR?= $(shell cd ..; pwd)
include $(TOPDIR)/include/config.mk
include $(LOKI_TOPDIR)/config.mk
endif

#include $(LOKI_TOPDIR)/config.mk

LIB:= lib$(shell pwd | sed -e 's/.*\///').a
OBJS	= sas_error.o sas_handler.o sas_init.o sas_util.o

all:	.depend	$(LIB)

$(LIB):	$(OBJS)
	$(AR) crv $@ $(OBJS)

#########################################################################
clean:
	find . -type f \
		\( -name 'core'	-o -name '*.bak' -o	-name '*~' \
		-o -name '*.o'	-o -name '*.a'	\) -print \
		| xargs	rm -f
.PHONY: clean		

clobber:	clean
	find . -type f \
		\( -name .depend -o -name '*.srec' -o -name '*.bin' \) \
		-print \
		| xargs rm -f
#########################################################################

.depend:	Makefile.gnu $(OBJS:.o=.c)
		$(CC) -M $(CFLAGS) $(OBJS:.o=.c)	> $@

sinclude .depend

#########################################################################

