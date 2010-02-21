#!/usr/bin/tclsh
#
# Run this TCL script to generate the "main.mk" makefile.
#

# Basenames of all source files that get preprocessed using
# "translate" and "makeheaders"
#
set src {
  add
  allrepo
  bag
  blob
  branch
  browse
  captcha
  cgi
  checkin
  checkout
  clearsign
  clone
  comformat
  configure
  construct
  content
  db
  delta
  deltacmd
  descendants
  diff
  diffcmd
  doc
  encode
  file
  finfo
  graph
  http
  http_socket
  http_transport
  info
  login
  main
  manifest
  md5
  merge
  merge3
  name
  pivot
  pqueue
  printf
  rebuild
  report
  rss
  rstats
  schema
  search
  setup
  sha1
  shun
  skins
  stat
  style
  sync
  tag
  th_main
  timeline
  tkt
  tktsetup
  undo
  update
  url
  user
  verify
  vfile
  wiki
  wikiformat
  winhttp
  xfer
  zip
  http_ssl
}

# Name of the final application
#
set name fossil

puts {# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl >main.mk"
# to regenerate this file.
#
# This file is included by linux-gcc.mk or linux-mingw.mk or possible
# some other makefiles.  This file contains the rules that are common
# to building regardless of the target.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)

}
puts -nonewline "SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  \$(SRCDIR)/$s.c"
}
puts "\n"
puts -nonewline "TRANS_SRC ="
foreach s [lsort $src] {
  puts -nonewline " \\\n  ${s}_.c"
}
puts "\n"
puts -nonewline "OBJ ="
foreach s [lsort $src] {
  puts -nonewline " \\\n \$(OBJDIR)/$s.o"
}
puts "\n"
puts "APPNAME = $name\$(E)"
puts "\n"

puts {
all:	$(OBJDIR) $(APPNAME)

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

$(OBJDIR):
	-mkdir $(OBJDIR)

translate:	$(SRCDIR)/translate.c
	$(BCC) -o translate $(SRCDIR)/translate.c

makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o makeheaders $(SRCDIR)/makeheaders.c

mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o mkindex $(SRCDIR)/mkindex.c

# WARNING. DANGER. Running the testsuite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(APPNAME)
	$(TCLSH) test/tester.tcl $(APPNAME)

VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest
	awk '{ printf "#define MANIFEST_UUID \"%s\"\n", $$1}' \
		$(SRCDIR)/../manifest.uuid >VERSION.h
	awk '{ printf "#define MANIFEST_VERSION \"[%.10s]\"\n", $$1}' \
		$(SRCDIR)/../manifest.uuid >>VERSION.h
	awk '$$1=="D"{printf "#define MANIFEST_DATE \"%s %s\"\n",\
		substr($$2,1,10),substr($$2,12)}' \
		$(SRCDIR)/../manifest >>VERSION.h

$(APPNAME):	headers $(OBJ) $(OBJDIR)/sqlite3.o $(OBJDIR)/th.o $(OBJDIR)/th_lang.o
	$(TCC) -o $(APPNAME) $(OBJ) $(OBJDIR)/sqlite3.o $(OBJDIR)/th.o $(OBJDIR)/th_lang.o $(LIB)

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:	
	# noop

clean:	
	rm -f $(OBJDIR)/*.o *_.c $(APPNAME) VERSION.h
	rm -f translate makeheaders mkindex page_index.h headers}

set hfiles {}
foreach s [lsort $src] {lappend hfiles $s.h}
puts "\trm -f $hfiles\n"

set mhargs {}
foreach s [lsort $src] {
  append mhargs " ${s}_.c:$s.h"
  set extra_h($s) {}
}
append mhargs " \$(SRCDIR)/sqlite3.h"
append mhargs " \$(SRCDIR)/th.h"
append mhargs " VERSION.h"
puts "page_index.h: \$(TRANS_SRC) mkindex"
puts "\t./mkindex \$(TRANS_SRC) >$@"
puts "headers:\tpage_index.h makeheaders VERSION.h"
puts "\t./makeheaders $mhargs"
puts "\ttouch headers"
puts "headers: Makefile"
puts "Makefile:"
set extra_h(main) page_index.h

foreach s [lsort $src] {
  puts "${s}_.c:\t\$(SRCDIR)/$s.c translate"
  puts "\t./translate \$(SRCDIR)/$s.c >${s}_.c\n"
  puts "\$(OBJDIR)/$s.o:\t${s}_.c $s.h $extra_h($s) \$(SRCDIR)/config.h"
  puts "\t\$(XTCC) -o \$(OBJDIR)/$s.o -c ${s}_.c\n"
  puts "$s.h:\theaders"
#  puts "\t./makeheaders $mhargs\n\ttouch headers\n"
#  puts "\t./makeheaders ${s}_.c:${s}.h\n"
}


puts "\$(OBJDIR)/sqlite3.o:\t\$(SRCDIR)/sqlite3.c"
set opt {-DSQLITE_OMIT_LOAD_EXTENSION=1}
append opt " -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4"
#append opt " -DSQLITE_ENABLE_FTS3=1"
append opt " -Dlocaltime=fossil_localtime"
puts "\t\$(XTCC) $opt -c \$(SRCDIR)/sqlite3.c -o \$(OBJDIR)/sqlite3.o\n"

puts "\$(OBJDIR)/th.o:\t\$(SRCDIR)/th.c"
puts "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th.c -o \$(OBJDIR)/th.o\n"

puts "\$(OBJDIR)/th_lang.o:\t\$(SRCDIR)/th_lang.c"
puts "\t\$(XTCC) -I\$(SRCDIR) -c \$(SRCDIR)/th_lang.c -o \$(OBJDIR)/th_lang.o\n"
