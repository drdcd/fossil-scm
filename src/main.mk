# DO NOT EDIT
#
# This file is automatically generated.  Instead of editing this
# file, edit "makemake.tcl" then run "tclsh makemake.tcl"
# to regenerate this file.
#
# This file is included by primary Makefile.
#

XTCC = $(TCC) $(CFLAGS) -I. -I$(SRCDIR)


SRC = \
  $(SRCDIR)/add.c \
  $(SRCDIR)/allrepo.c \
  $(SRCDIR)/attach.c \
  $(SRCDIR)/bag.c \
  $(SRCDIR)/bisect.c \
  $(SRCDIR)/blob.c \
  $(SRCDIR)/branch.c \
  $(SRCDIR)/browse.c \
  $(SRCDIR)/captcha.c \
  $(SRCDIR)/cgi.c \
  $(SRCDIR)/checkin.c \
  $(SRCDIR)/checkout.c \
  $(SRCDIR)/clearsign.c \
  $(SRCDIR)/clone.c \
  $(SRCDIR)/comformat.c \
  $(SRCDIR)/configure.c \
  $(SRCDIR)/content.c \
  $(SRCDIR)/db.c \
  $(SRCDIR)/delta.c \
  $(SRCDIR)/deltacmd.c \
  $(SRCDIR)/descendants.c \
  $(SRCDIR)/diff.c \
  $(SRCDIR)/diffcmd.c \
  $(SRCDIR)/doc.c \
  $(SRCDIR)/encode.c \
  $(SRCDIR)/event.c \
  $(SRCDIR)/export.c \
  $(SRCDIR)/file.c \
  $(SRCDIR)/finfo.c \
  $(SRCDIR)/graph.c \
  $(SRCDIR)/http.c \
  $(SRCDIR)/http_socket.c \
  $(SRCDIR)/http_ssl.c \
  $(SRCDIR)/http_transport.c \
  $(SRCDIR)/import.c \
  $(SRCDIR)/info.c \
  $(SRCDIR)/login.c \
  $(SRCDIR)/main.c \
  $(SRCDIR)/manifest.c \
  $(SRCDIR)/md5.c \
  $(SRCDIR)/merge.c \
  $(SRCDIR)/merge3.c \
  $(SRCDIR)/name.c \
  $(SRCDIR)/pivot.c \
  $(SRCDIR)/popen.c \
  $(SRCDIR)/pqueue.c \
  $(SRCDIR)/printf.c \
  $(SRCDIR)/rebuild.c \
  $(SRCDIR)/report.c \
  $(SRCDIR)/rss.c \
  $(SRCDIR)/schema.c \
  $(SRCDIR)/search.c \
  $(SRCDIR)/setup.c \
  $(SRCDIR)/sha1.c \
  $(SRCDIR)/shun.c \
  $(SRCDIR)/skins.c \
  $(SRCDIR)/sqlcmd.c \
  $(SRCDIR)/stash.c \
  $(SRCDIR)/stat.c \
  $(SRCDIR)/style.c \
  $(SRCDIR)/sync.c \
  $(SRCDIR)/tag.c \
  $(SRCDIR)/th_main.c \
  $(SRCDIR)/timeline.c \
  $(SRCDIR)/tkt.c \
  $(SRCDIR)/tktsetup.c \
  $(SRCDIR)/undo.c \
  $(SRCDIR)/update.c \
  $(SRCDIR)/url.c \
  $(SRCDIR)/user.c \
  $(SRCDIR)/verify.c \
  $(SRCDIR)/vfile.c \
  $(SRCDIR)/wiki.c \
  $(SRCDIR)/wikiformat.c \
  $(SRCDIR)/winhttp.c \
  $(SRCDIR)/xfer.c \
  $(SRCDIR)/zip.c

TRANS_SRC = \
  $(OBJDIR)/add_.c \
  $(OBJDIR)/allrepo_.c \
  $(OBJDIR)/attach_.c \
  $(OBJDIR)/bag_.c \
  $(OBJDIR)/bisect_.c \
  $(OBJDIR)/blob_.c \
  $(OBJDIR)/branch_.c \
  $(OBJDIR)/browse_.c \
  $(OBJDIR)/captcha_.c \
  $(OBJDIR)/cgi_.c \
  $(OBJDIR)/checkin_.c \
  $(OBJDIR)/checkout_.c \
  $(OBJDIR)/clearsign_.c \
  $(OBJDIR)/clone_.c \
  $(OBJDIR)/comformat_.c \
  $(OBJDIR)/configure_.c \
  $(OBJDIR)/content_.c \
  $(OBJDIR)/db_.c \
  $(OBJDIR)/delta_.c \
  $(OBJDIR)/deltacmd_.c \
  $(OBJDIR)/descendants_.c \
  $(OBJDIR)/diff_.c \
  $(OBJDIR)/diffcmd_.c \
  $(OBJDIR)/doc_.c \
  $(OBJDIR)/encode_.c \
  $(OBJDIR)/event_.c \
  $(OBJDIR)/export_.c \
  $(OBJDIR)/file_.c \
  $(OBJDIR)/finfo_.c \
  $(OBJDIR)/graph_.c \
  $(OBJDIR)/http_.c \
  $(OBJDIR)/http_socket_.c \
  $(OBJDIR)/http_ssl_.c \
  $(OBJDIR)/http_transport_.c \
  $(OBJDIR)/import_.c \
  $(OBJDIR)/info_.c \
  $(OBJDIR)/login_.c \
  $(OBJDIR)/main_.c \
  $(OBJDIR)/manifest_.c \
  $(OBJDIR)/md5_.c \
  $(OBJDIR)/merge_.c \
  $(OBJDIR)/merge3_.c \
  $(OBJDIR)/name_.c \
  $(OBJDIR)/pivot_.c \
  $(OBJDIR)/popen_.c \
  $(OBJDIR)/pqueue_.c \
  $(OBJDIR)/printf_.c \
  $(OBJDIR)/rebuild_.c \
  $(OBJDIR)/report_.c \
  $(OBJDIR)/rss_.c \
  $(OBJDIR)/schema_.c \
  $(OBJDIR)/search_.c \
  $(OBJDIR)/setup_.c \
  $(OBJDIR)/sha1_.c \
  $(OBJDIR)/shun_.c \
  $(OBJDIR)/skins_.c \
  $(OBJDIR)/sqlcmd_.c \
  $(OBJDIR)/stash_.c \
  $(OBJDIR)/stat_.c \
  $(OBJDIR)/style_.c \
  $(OBJDIR)/sync_.c \
  $(OBJDIR)/tag_.c \
  $(OBJDIR)/th_main_.c \
  $(OBJDIR)/timeline_.c \
  $(OBJDIR)/tkt_.c \
  $(OBJDIR)/tktsetup_.c \
  $(OBJDIR)/undo_.c \
  $(OBJDIR)/update_.c \
  $(OBJDIR)/url_.c \
  $(OBJDIR)/user_.c \
  $(OBJDIR)/verify_.c \
  $(OBJDIR)/vfile_.c \
  $(OBJDIR)/wiki_.c \
  $(OBJDIR)/wikiformat_.c \
  $(OBJDIR)/winhttp_.c \
  $(OBJDIR)/xfer_.c \
  $(OBJDIR)/zip_.c

OBJ = \
 $(OBJDIR)/add.o \
 $(OBJDIR)/allrepo.o \
 $(OBJDIR)/attach.o \
 $(OBJDIR)/bag.o \
 $(OBJDIR)/bisect.o \
 $(OBJDIR)/blob.o \
 $(OBJDIR)/branch.o \
 $(OBJDIR)/browse.o \
 $(OBJDIR)/captcha.o \
 $(OBJDIR)/cgi.o \
 $(OBJDIR)/checkin.o \
 $(OBJDIR)/checkout.o \
 $(OBJDIR)/clearsign.o \
 $(OBJDIR)/clone.o \
 $(OBJDIR)/comformat.o \
 $(OBJDIR)/configure.o \
 $(OBJDIR)/content.o \
 $(OBJDIR)/db.o \
 $(OBJDIR)/delta.o \
 $(OBJDIR)/deltacmd.o \
 $(OBJDIR)/descendants.o \
 $(OBJDIR)/diff.o \
 $(OBJDIR)/diffcmd.o \
 $(OBJDIR)/doc.o \
 $(OBJDIR)/encode.o \
 $(OBJDIR)/event.o \
 $(OBJDIR)/export.o \
 $(OBJDIR)/file.o \
 $(OBJDIR)/finfo.o \
 $(OBJDIR)/graph.o \
 $(OBJDIR)/http.o \
 $(OBJDIR)/http_socket.o \
 $(OBJDIR)/http_ssl.o \
 $(OBJDIR)/http_transport.o \
 $(OBJDIR)/import.o \
 $(OBJDIR)/info.o \
 $(OBJDIR)/login.o \
 $(OBJDIR)/main.o \
 $(OBJDIR)/manifest.o \
 $(OBJDIR)/md5.o \
 $(OBJDIR)/merge.o \
 $(OBJDIR)/merge3.o \
 $(OBJDIR)/name.o \
 $(OBJDIR)/pivot.o \
 $(OBJDIR)/popen.o \
 $(OBJDIR)/pqueue.o \
 $(OBJDIR)/printf.o \
 $(OBJDIR)/rebuild.o \
 $(OBJDIR)/report.o \
 $(OBJDIR)/rss.o \
 $(OBJDIR)/schema.o \
 $(OBJDIR)/search.o \
 $(OBJDIR)/setup.o \
 $(OBJDIR)/sha1.o \
 $(OBJDIR)/shun.o \
 $(OBJDIR)/skins.o \
 $(OBJDIR)/sqlcmd.o \
 $(OBJDIR)/stash.o \
 $(OBJDIR)/stat.o \
 $(OBJDIR)/style.o \
 $(OBJDIR)/sync.o \
 $(OBJDIR)/tag.o \
 $(OBJDIR)/th_main.o \
 $(OBJDIR)/timeline.o \
 $(OBJDIR)/tkt.o \
 $(OBJDIR)/tktsetup.o \
 $(OBJDIR)/undo.o \
 $(OBJDIR)/update.o \
 $(OBJDIR)/url.o \
 $(OBJDIR)/user.o \
 $(OBJDIR)/verify.o \
 $(OBJDIR)/vfile.o \
 $(OBJDIR)/wiki.o \
 $(OBJDIR)/wikiformat.o \
 $(OBJDIR)/winhttp.o \
 $(OBJDIR)/xfer.o \
 $(OBJDIR)/zip.o

APPNAME = fossil$(E)



all:	$(OBJDIR) $(APPNAME)

install:	$(APPNAME)
	mv $(APPNAME) $(INSTALLDIR)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/translate:	$(SRCDIR)/translate.c
	$(BCC) -o $(OBJDIR)/translate $(SRCDIR)/translate.c

$(OBJDIR)/makeheaders:	$(SRCDIR)/makeheaders.c
	$(BCC) -o $(OBJDIR)/makeheaders $(SRCDIR)/makeheaders.c

$(OBJDIR)/mkindex:	$(SRCDIR)/mkindex.c
	$(BCC) -o $(OBJDIR)/mkindex $(SRCDIR)/mkindex.c

# WARNING. DANGER. Running the testsuite modifies the repository the
# build is done from, i.e. the checkout belongs to. Do not sync/push
# the repository after running the tests.
test:	$(APPNAME)
	$(TCLSH) test/tester.tcl $(APPNAME)

$(OBJDIR)/VERSION.h:	$(SRCDIR)/../manifest.uuid $(SRCDIR)/../manifest
	awk '{ printf "#define MANIFEST_UUID \"%s\"\n", $$1}'  $(SRCDIR)/../manifest.uuid >$(OBJDIR)/VERSION.h
	awk '{ printf "#define MANIFEST_VERSION \"[%.10s]\"\n", $$1}'  $(SRCDIR)/../manifest.uuid >>$(OBJDIR)/VERSION.h
	awk '$$1=="D"{printf "#define MANIFEST_DATE \"%s %s\"\n", substr($$2,1,10),substr($$2,12)}'  $(SRCDIR)/../manifest >>$(OBJDIR)/VERSION.h

EXTRAOBJ =  $(OBJDIR)/sqlite3.o  $(OBJDIR)/shell.o  $(OBJDIR)/th.o  $(OBJDIR)/th_lang.o

$(APPNAME):	$(OBJDIR)/headers $(OBJ) $(EXTRAOBJ)
	$(TCC) -o $(APPNAME) $(OBJ) $(EXTRAOBJ) $(LIB)

# This rule prevents make from using its default rules to try build
# an executable named "manifest" out of the file named "manifest.c"
#
$(SRCDIR)/../manifest:	
	# noop

clean:	
	rm -rf $(OBJDIR)/* $(APPNAME)


$(OBJDIR)/page_index.h: $(TRANS_SRC) $(OBJDIR)/mkindex
	$(OBJDIR)/mkindex $(TRANS_SRC) >$@
$(OBJDIR)/headers:	$(OBJDIR)/page_index.h $(OBJDIR)/makeheaders $(OBJDIR)/VERSION.h
	$(OBJDIR)/makeheaders  $(OBJDIR)/add_.c:$(OBJDIR)/add.h $(OBJDIR)/allrepo_.c:$(OBJDIR)/allrepo.h $(OBJDIR)/attach_.c:$(OBJDIR)/attach.h $(OBJDIR)/bag_.c:$(OBJDIR)/bag.h $(OBJDIR)/bisect_.c:$(OBJDIR)/bisect.h $(OBJDIR)/blob_.c:$(OBJDIR)/blob.h $(OBJDIR)/branch_.c:$(OBJDIR)/branch.h $(OBJDIR)/browse_.c:$(OBJDIR)/browse.h $(OBJDIR)/captcha_.c:$(OBJDIR)/captcha.h $(OBJDIR)/cgi_.c:$(OBJDIR)/cgi.h $(OBJDIR)/checkin_.c:$(OBJDIR)/checkin.h $(OBJDIR)/checkout_.c:$(OBJDIR)/checkout.h $(OBJDIR)/clearsign_.c:$(OBJDIR)/clearsign.h $(OBJDIR)/clone_.c:$(OBJDIR)/clone.h $(OBJDIR)/comformat_.c:$(OBJDIR)/comformat.h $(OBJDIR)/configure_.c:$(OBJDIR)/configure.h $(OBJDIR)/content_.c:$(OBJDIR)/content.h $(OBJDIR)/db_.c:$(OBJDIR)/db.h $(OBJDIR)/delta_.c:$(OBJDIR)/delta.h $(OBJDIR)/deltacmd_.c:$(OBJDIR)/deltacmd.h $(OBJDIR)/descendants_.c:$(OBJDIR)/descendants.h $(OBJDIR)/diff_.c:$(OBJDIR)/diff.h $(OBJDIR)/diffcmd_.c:$(OBJDIR)/diffcmd.h $(OBJDIR)/doc_.c:$(OBJDIR)/doc.h $(OBJDIR)/encode_.c:$(OBJDIR)/encode.h $(OBJDIR)/event_.c:$(OBJDIR)/event.h $(OBJDIR)/export_.c:$(OBJDIR)/export.h $(OBJDIR)/file_.c:$(OBJDIR)/file.h $(OBJDIR)/finfo_.c:$(OBJDIR)/finfo.h $(OBJDIR)/graph_.c:$(OBJDIR)/graph.h $(OBJDIR)/http_.c:$(OBJDIR)/http.h $(OBJDIR)/http_socket_.c:$(OBJDIR)/http_socket.h $(OBJDIR)/http_ssl_.c:$(OBJDIR)/http_ssl.h $(OBJDIR)/http_transport_.c:$(OBJDIR)/http_transport.h $(OBJDIR)/import_.c:$(OBJDIR)/import.h $(OBJDIR)/info_.c:$(OBJDIR)/info.h $(OBJDIR)/login_.c:$(OBJDIR)/login.h $(OBJDIR)/main_.c:$(OBJDIR)/main.h $(OBJDIR)/manifest_.c:$(OBJDIR)/manifest.h $(OBJDIR)/md5_.c:$(OBJDIR)/md5.h $(OBJDIR)/merge_.c:$(OBJDIR)/merge.h $(OBJDIR)/merge3_.c:$(OBJDIR)/merge3.h $(OBJDIR)/name_.c:$(OBJDIR)/name.h $(OBJDIR)/pivot_.c:$(OBJDIR)/pivot.h $(OBJDIR)/popen_.c:$(OBJDIR)/popen.h $(OBJDIR)/pqueue_.c:$(OBJDIR)/pqueue.h $(OBJDIR)/printf_.c:$(OBJDIR)/printf.h $(OBJDIR)/rebuild_.c:$(OBJDIR)/rebuild.h $(OBJDIR)/report_.c:$(OBJDIR)/report.h $(OBJDIR)/rss_.c:$(OBJDIR)/rss.h $(OBJDIR)/schema_.c:$(OBJDIR)/schema.h $(OBJDIR)/search_.c:$(OBJDIR)/search.h $(OBJDIR)/setup_.c:$(OBJDIR)/setup.h $(OBJDIR)/sha1_.c:$(OBJDIR)/sha1.h $(OBJDIR)/shun_.c:$(OBJDIR)/shun.h $(OBJDIR)/skins_.c:$(OBJDIR)/skins.h $(OBJDIR)/sqlcmd_.c:$(OBJDIR)/sqlcmd.h $(OBJDIR)/stash_.c:$(OBJDIR)/stash.h $(OBJDIR)/stat_.c:$(OBJDIR)/stat.h $(OBJDIR)/style_.c:$(OBJDIR)/style.h $(OBJDIR)/sync_.c:$(OBJDIR)/sync.h $(OBJDIR)/tag_.c:$(OBJDIR)/tag.h $(OBJDIR)/th_main_.c:$(OBJDIR)/th_main.h $(OBJDIR)/timeline_.c:$(OBJDIR)/timeline.h $(OBJDIR)/tkt_.c:$(OBJDIR)/tkt.h $(OBJDIR)/tktsetup_.c:$(OBJDIR)/tktsetup.h $(OBJDIR)/undo_.c:$(OBJDIR)/undo.h $(OBJDIR)/update_.c:$(OBJDIR)/update.h $(OBJDIR)/url_.c:$(OBJDIR)/url.h $(OBJDIR)/user_.c:$(OBJDIR)/user.h $(OBJDIR)/verify_.c:$(OBJDIR)/verify.h $(OBJDIR)/vfile_.c:$(OBJDIR)/vfile.h $(OBJDIR)/wiki_.c:$(OBJDIR)/wiki.h $(OBJDIR)/wikiformat_.c:$(OBJDIR)/wikiformat.h $(OBJDIR)/winhttp_.c:$(OBJDIR)/winhttp.h $(OBJDIR)/xfer_.c:$(OBJDIR)/xfer.h $(OBJDIR)/zip_.c:$(OBJDIR)/zip.h $(SRCDIR)/sqlite3.h $(SRCDIR)/th.h $(OBJDIR)/VERSION.h
	touch $(OBJDIR)/headers
$(OBJDIR)/headers: Makefile
Makefile:
$(OBJDIR)/add_.c:	$(SRCDIR)/add.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/add.c >$(OBJDIR)/add_.c

$(OBJDIR)/add.o:	$(OBJDIR)/add_.c $(OBJDIR)/add.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/add.o -c $(OBJDIR)/add_.c

add.h:	$(OBJDIR)/headers
$(OBJDIR)/allrepo_.c:	$(SRCDIR)/allrepo.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/allrepo.c >$(OBJDIR)/allrepo_.c

$(OBJDIR)/allrepo.o:	$(OBJDIR)/allrepo_.c $(OBJDIR)/allrepo.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/allrepo.o -c $(OBJDIR)/allrepo_.c

allrepo.h:	$(OBJDIR)/headers
$(OBJDIR)/attach_.c:	$(SRCDIR)/attach.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/attach.c >$(OBJDIR)/attach_.c

$(OBJDIR)/attach.o:	$(OBJDIR)/attach_.c $(OBJDIR)/attach.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/attach.o -c $(OBJDIR)/attach_.c

attach.h:	$(OBJDIR)/headers
$(OBJDIR)/bag_.c:	$(SRCDIR)/bag.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/bag.c >$(OBJDIR)/bag_.c

$(OBJDIR)/bag.o:	$(OBJDIR)/bag_.c $(OBJDIR)/bag.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/bag.o -c $(OBJDIR)/bag_.c

bag.h:	$(OBJDIR)/headers
$(OBJDIR)/bisect_.c:	$(SRCDIR)/bisect.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/bisect.c >$(OBJDIR)/bisect_.c

$(OBJDIR)/bisect.o:	$(OBJDIR)/bisect_.c $(OBJDIR)/bisect.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/bisect.o -c $(OBJDIR)/bisect_.c

bisect.h:	$(OBJDIR)/headers
$(OBJDIR)/blob_.c:	$(SRCDIR)/blob.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/blob.c >$(OBJDIR)/blob_.c

$(OBJDIR)/blob.o:	$(OBJDIR)/blob_.c $(OBJDIR)/blob.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/blob.o -c $(OBJDIR)/blob_.c

blob.h:	$(OBJDIR)/headers
$(OBJDIR)/branch_.c:	$(SRCDIR)/branch.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/branch.c >$(OBJDIR)/branch_.c

$(OBJDIR)/branch.o:	$(OBJDIR)/branch_.c $(OBJDIR)/branch.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/branch.o -c $(OBJDIR)/branch_.c

branch.h:	$(OBJDIR)/headers
$(OBJDIR)/browse_.c:	$(SRCDIR)/browse.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/browse.c >$(OBJDIR)/browse_.c

$(OBJDIR)/browse.o:	$(OBJDIR)/browse_.c $(OBJDIR)/browse.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/browse.o -c $(OBJDIR)/browse_.c

browse.h:	$(OBJDIR)/headers
$(OBJDIR)/captcha_.c:	$(SRCDIR)/captcha.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/captcha.c >$(OBJDIR)/captcha_.c

$(OBJDIR)/captcha.o:	$(OBJDIR)/captcha_.c $(OBJDIR)/captcha.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/captcha.o -c $(OBJDIR)/captcha_.c

captcha.h:	$(OBJDIR)/headers
$(OBJDIR)/cgi_.c:	$(SRCDIR)/cgi.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/cgi.c >$(OBJDIR)/cgi_.c

$(OBJDIR)/cgi.o:	$(OBJDIR)/cgi_.c $(OBJDIR)/cgi.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/cgi.o -c $(OBJDIR)/cgi_.c

cgi.h:	$(OBJDIR)/headers
$(OBJDIR)/checkin_.c:	$(SRCDIR)/checkin.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/checkin.c >$(OBJDIR)/checkin_.c

$(OBJDIR)/checkin.o:	$(OBJDIR)/checkin_.c $(OBJDIR)/checkin.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/checkin.o -c $(OBJDIR)/checkin_.c

checkin.h:	$(OBJDIR)/headers
$(OBJDIR)/checkout_.c:	$(SRCDIR)/checkout.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/checkout.c >$(OBJDIR)/checkout_.c

$(OBJDIR)/checkout.o:	$(OBJDIR)/checkout_.c $(OBJDIR)/checkout.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/checkout.o -c $(OBJDIR)/checkout_.c

checkout.h:	$(OBJDIR)/headers
$(OBJDIR)/clearsign_.c:	$(SRCDIR)/clearsign.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/clearsign.c >$(OBJDIR)/clearsign_.c

$(OBJDIR)/clearsign.o:	$(OBJDIR)/clearsign_.c $(OBJDIR)/clearsign.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/clearsign.o -c $(OBJDIR)/clearsign_.c

clearsign.h:	$(OBJDIR)/headers
$(OBJDIR)/clone_.c:	$(SRCDIR)/clone.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/clone.c >$(OBJDIR)/clone_.c

$(OBJDIR)/clone.o:	$(OBJDIR)/clone_.c $(OBJDIR)/clone.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/clone.o -c $(OBJDIR)/clone_.c

clone.h:	$(OBJDIR)/headers
$(OBJDIR)/comformat_.c:	$(SRCDIR)/comformat.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/comformat.c >$(OBJDIR)/comformat_.c

$(OBJDIR)/comformat.o:	$(OBJDIR)/comformat_.c $(OBJDIR)/comformat.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/comformat.o -c $(OBJDIR)/comformat_.c

comformat.h:	$(OBJDIR)/headers
$(OBJDIR)/configure_.c:	$(SRCDIR)/configure.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/configure.c >$(OBJDIR)/configure_.c

$(OBJDIR)/configure.o:	$(OBJDIR)/configure_.c $(OBJDIR)/configure.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/configure.o -c $(OBJDIR)/configure_.c

configure.h:	$(OBJDIR)/headers
$(OBJDIR)/content_.c:	$(SRCDIR)/content.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/content.c >$(OBJDIR)/content_.c

$(OBJDIR)/content.o:	$(OBJDIR)/content_.c $(OBJDIR)/content.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/content.o -c $(OBJDIR)/content_.c

content.h:	$(OBJDIR)/headers
$(OBJDIR)/db_.c:	$(SRCDIR)/db.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/db.c >$(OBJDIR)/db_.c

$(OBJDIR)/db.o:	$(OBJDIR)/db_.c $(OBJDIR)/db.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/db.o -c $(OBJDIR)/db_.c

db.h:	$(OBJDIR)/headers
$(OBJDIR)/delta_.c:	$(SRCDIR)/delta.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/delta.c >$(OBJDIR)/delta_.c

$(OBJDIR)/delta.o:	$(OBJDIR)/delta_.c $(OBJDIR)/delta.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/delta.o -c $(OBJDIR)/delta_.c

delta.h:	$(OBJDIR)/headers
$(OBJDIR)/deltacmd_.c:	$(SRCDIR)/deltacmd.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/deltacmd.c >$(OBJDIR)/deltacmd_.c

$(OBJDIR)/deltacmd.o:	$(OBJDIR)/deltacmd_.c $(OBJDIR)/deltacmd.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/deltacmd.o -c $(OBJDIR)/deltacmd_.c

deltacmd.h:	$(OBJDIR)/headers
$(OBJDIR)/descendants_.c:	$(SRCDIR)/descendants.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/descendants.c >$(OBJDIR)/descendants_.c

$(OBJDIR)/descendants.o:	$(OBJDIR)/descendants_.c $(OBJDIR)/descendants.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/descendants.o -c $(OBJDIR)/descendants_.c

descendants.h:	$(OBJDIR)/headers
$(OBJDIR)/diff_.c:	$(SRCDIR)/diff.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/diff.c >$(OBJDIR)/diff_.c

$(OBJDIR)/diff.o:	$(OBJDIR)/diff_.c $(OBJDIR)/diff.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/diff.o -c $(OBJDIR)/diff_.c

diff.h:	$(OBJDIR)/headers
$(OBJDIR)/diffcmd_.c:	$(SRCDIR)/diffcmd.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/diffcmd.c >$(OBJDIR)/diffcmd_.c

$(OBJDIR)/diffcmd.o:	$(OBJDIR)/diffcmd_.c $(OBJDIR)/diffcmd.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/diffcmd.o -c $(OBJDIR)/diffcmd_.c

diffcmd.h:	$(OBJDIR)/headers
$(OBJDIR)/doc_.c:	$(SRCDIR)/doc.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/doc.c >$(OBJDIR)/doc_.c

$(OBJDIR)/doc.o:	$(OBJDIR)/doc_.c $(OBJDIR)/doc.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/doc.o -c $(OBJDIR)/doc_.c

doc.h:	$(OBJDIR)/headers
$(OBJDIR)/encode_.c:	$(SRCDIR)/encode.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/encode.c >$(OBJDIR)/encode_.c

$(OBJDIR)/encode.o:	$(OBJDIR)/encode_.c $(OBJDIR)/encode.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/encode.o -c $(OBJDIR)/encode_.c

encode.h:	$(OBJDIR)/headers
$(OBJDIR)/event_.c:	$(SRCDIR)/event.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/event.c >$(OBJDIR)/event_.c

$(OBJDIR)/event.o:	$(OBJDIR)/event_.c $(OBJDIR)/event.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/event.o -c $(OBJDIR)/event_.c

event.h:	$(OBJDIR)/headers
$(OBJDIR)/export_.c:	$(SRCDIR)/export.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/export.c >$(OBJDIR)/export_.c

$(OBJDIR)/export.o:	$(OBJDIR)/export_.c $(OBJDIR)/export.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/export.o -c $(OBJDIR)/export_.c

export.h:	$(OBJDIR)/headers
$(OBJDIR)/file_.c:	$(SRCDIR)/file.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/file.c >$(OBJDIR)/file_.c

$(OBJDIR)/file.o:	$(OBJDIR)/file_.c $(OBJDIR)/file.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/file.o -c $(OBJDIR)/file_.c

file.h:	$(OBJDIR)/headers
$(OBJDIR)/finfo_.c:	$(SRCDIR)/finfo.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/finfo.c >$(OBJDIR)/finfo_.c

$(OBJDIR)/finfo.o:	$(OBJDIR)/finfo_.c $(OBJDIR)/finfo.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/finfo.o -c $(OBJDIR)/finfo_.c

finfo.h:	$(OBJDIR)/headers
$(OBJDIR)/graph_.c:	$(SRCDIR)/graph.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/graph.c >$(OBJDIR)/graph_.c

$(OBJDIR)/graph.o:	$(OBJDIR)/graph_.c $(OBJDIR)/graph.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/graph.o -c $(OBJDIR)/graph_.c

graph.h:	$(OBJDIR)/headers
$(OBJDIR)/http_.c:	$(SRCDIR)/http.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/http.c >$(OBJDIR)/http_.c

$(OBJDIR)/http.o:	$(OBJDIR)/http_.c $(OBJDIR)/http.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/http.o -c $(OBJDIR)/http_.c

http.h:	$(OBJDIR)/headers
$(OBJDIR)/http_socket_.c:	$(SRCDIR)/http_socket.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/http_socket.c >$(OBJDIR)/http_socket_.c

$(OBJDIR)/http_socket.o:	$(OBJDIR)/http_socket_.c $(OBJDIR)/http_socket.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/http_socket.o -c $(OBJDIR)/http_socket_.c

http_socket.h:	$(OBJDIR)/headers
$(OBJDIR)/http_ssl_.c:	$(SRCDIR)/http_ssl.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/http_ssl.c >$(OBJDIR)/http_ssl_.c

$(OBJDIR)/http_ssl.o:	$(OBJDIR)/http_ssl_.c $(OBJDIR)/http_ssl.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/http_ssl.o -c $(OBJDIR)/http_ssl_.c

http_ssl.h:	$(OBJDIR)/headers
$(OBJDIR)/http_transport_.c:	$(SRCDIR)/http_transport.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/http_transport.c >$(OBJDIR)/http_transport_.c

$(OBJDIR)/http_transport.o:	$(OBJDIR)/http_transport_.c $(OBJDIR)/http_transport.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/http_transport.o -c $(OBJDIR)/http_transport_.c

http_transport.h:	$(OBJDIR)/headers
$(OBJDIR)/import_.c:	$(SRCDIR)/import.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/import.c >$(OBJDIR)/import_.c

$(OBJDIR)/import.o:	$(OBJDIR)/import_.c $(OBJDIR)/import.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/import.o -c $(OBJDIR)/import_.c

import.h:	$(OBJDIR)/headers
$(OBJDIR)/info_.c:	$(SRCDIR)/info.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/info.c >$(OBJDIR)/info_.c

$(OBJDIR)/info.o:	$(OBJDIR)/info_.c $(OBJDIR)/info.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/info.o -c $(OBJDIR)/info_.c

info.h:	$(OBJDIR)/headers
$(OBJDIR)/login_.c:	$(SRCDIR)/login.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/login.c >$(OBJDIR)/login_.c

$(OBJDIR)/login.o:	$(OBJDIR)/login_.c $(OBJDIR)/login.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/login.o -c $(OBJDIR)/login_.c

login.h:	$(OBJDIR)/headers
$(OBJDIR)/main_.c:	$(SRCDIR)/main.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/main.c >$(OBJDIR)/main_.c

$(OBJDIR)/main.o:	$(OBJDIR)/main_.c $(OBJDIR)/main.h $(OBJDIR)/page_index.h $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/main.o -c $(OBJDIR)/main_.c

main.h:	$(OBJDIR)/headers
$(OBJDIR)/manifest_.c:	$(SRCDIR)/manifest.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/manifest.c >$(OBJDIR)/manifest_.c

$(OBJDIR)/manifest.o:	$(OBJDIR)/manifest_.c $(OBJDIR)/manifest.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/manifest.o -c $(OBJDIR)/manifest_.c

manifest.h:	$(OBJDIR)/headers
$(OBJDIR)/md5_.c:	$(SRCDIR)/md5.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/md5.c >$(OBJDIR)/md5_.c

$(OBJDIR)/md5.o:	$(OBJDIR)/md5_.c $(OBJDIR)/md5.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/md5.o -c $(OBJDIR)/md5_.c

md5.h:	$(OBJDIR)/headers
$(OBJDIR)/merge_.c:	$(SRCDIR)/merge.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/merge.c >$(OBJDIR)/merge_.c

$(OBJDIR)/merge.o:	$(OBJDIR)/merge_.c $(OBJDIR)/merge.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/merge.o -c $(OBJDIR)/merge_.c

merge.h:	$(OBJDIR)/headers
$(OBJDIR)/merge3_.c:	$(SRCDIR)/merge3.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/merge3.c >$(OBJDIR)/merge3_.c

$(OBJDIR)/merge3.o:	$(OBJDIR)/merge3_.c $(OBJDIR)/merge3.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/merge3.o -c $(OBJDIR)/merge3_.c

merge3.h:	$(OBJDIR)/headers
$(OBJDIR)/name_.c:	$(SRCDIR)/name.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/name.c >$(OBJDIR)/name_.c

$(OBJDIR)/name.o:	$(OBJDIR)/name_.c $(OBJDIR)/name.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/name.o -c $(OBJDIR)/name_.c

name.h:	$(OBJDIR)/headers
$(OBJDIR)/pivot_.c:	$(SRCDIR)/pivot.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/pivot.c >$(OBJDIR)/pivot_.c

$(OBJDIR)/pivot.o:	$(OBJDIR)/pivot_.c $(OBJDIR)/pivot.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/pivot.o -c $(OBJDIR)/pivot_.c

pivot.h:	$(OBJDIR)/headers
$(OBJDIR)/popen_.c:	$(SRCDIR)/popen.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/popen.c >$(OBJDIR)/popen_.c

$(OBJDIR)/popen.o:	$(OBJDIR)/popen_.c $(OBJDIR)/popen.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/popen.o -c $(OBJDIR)/popen_.c

popen.h:	$(OBJDIR)/headers
$(OBJDIR)/pqueue_.c:	$(SRCDIR)/pqueue.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/pqueue.c >$(OBJDIR)/pqueue_.c

$(OBJDIR)/pqueue.o:	$(OBJDIR)/pqueue_.c $(OBJDIR)/pqueue.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/pqueue.o -c $(OBJDIR)/pqueue_.c

pqueue.h:	$(OBJDIR)/headers
$(OBJDIR)/printf_.c:	$(SRCDIR)/printf.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/printf.c >$(OBJDIR)/printf_.c

$(OBJDIR)/printf.o:	$(OBJDIR)/printf_.c $(OBJDIR)/printf.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/printf.o -c $(OBJDIR)/printf_.c

printf.h:	$(OBJDIR)/headers
$(OBJDIR)/rebuild_.c:	$(SRCDIR)/rebuild.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/rebuild.c >$(OBJDIR)/rebuild_.c

$(OBJDIR)/rebuild.o:	$(OBJDIR)/rebuild_.c $(OBJDIR)/rebuild.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/rebuild.o -c $(OBJDIR)/rebuild_.c

rebuild.h:	$(OBJDIR)/headers
$(OBJDIR)/report_.c:	$(SRCDIR)/report.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/report.c >$(OBJDIR)/report_.c

$(OBJDIR)/report.o:	$(OBJDIR)/report_.c $(OBJDIR)/report.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/report.o -c $(OBJDIR)/report_.c

report.h:	$(OBJDIR)/headers
$(OBJDIR)/rss_.c:	$(SRCDIR)/rss.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/rss.c >$(OBJDIR)/rss_.c

$(OBJDIR)/rss.o:	$(OBJDIR)/rss_.c $(OBJDIR)/rss.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/rss.o -c $(OBJDIR)/rss_.c

rss.h:	$(OBJDIR)/headers
$(OBJDIR)/schema_.c:	$(SRCDIR)/schema.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/schema.c >$(OBJDIR)/schema_.c

$(OBJDIR)/schema.o:	$(OBJDIR)/schema_.c $(OBJDIR)/schema.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/schema.o -c $(OBJDIR)/schema_.c

schema.h:	$(OBJDIR)/headers
$(OBJDIR)/search_.c:	$(SRCDIR)/search.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/search.c >$(OBJDIR)/search_.c

$(OBJDIR)/search.o:	$(OBJDIR)/search_.c $(OBJDIR)/search.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/search.o -c $(OBJDIR)/search_.c

search.h:	$(OBJDIR)/headers
$(OBJDIR)/setup_.c:	$(SRCDIR)/setup.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/setup.c >$(OBJDIR)/setup_.c

$(OBJDIR)/setup.o:	$(OBJDIR)/setup_.c $(OBJDIR)/setup.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/setup.o -c $(OBJDIR)/setup_.c

setup.h:	$(OBJDIR)/headers
$(OBJDIR)/sha1_.c:	$(SRCDIR)/sha1.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/sha1.c >$(OBJDIR)/sha1_.c

$(OBJDIR)/sha1.o:	$(OBJDIR)/sha1_.c $(OBJDIR)/sha1.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/sha1.o -c $(OBJDIR)/sha1_.c

sha1.h:	$(OBJDIR)/headers
$(OBJDIR)/shun_.c:	$(SRCDIR)/shun.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/shun.c >$(OBJDIR)/shun_.c

$(OBJDIR)/shun.o:	$(OBJDIR)/shun_.c $(OBJDIR)/shun.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/shun.o -c $(OBJDIR)/shun_.c

shun.h:	$(OBJDIR)/headers
$(OBJDIR)/skins_.c:	$(SRCDIR)/skins.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/skins.c >$(OBJDIR)/skins_.c

$(OBJDIR)/skins.o:	$(OBJDIR)/skins_.c $(OBJDIR)/skins.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/skins.o -c $(OBJDIR)/skins_.c

skins.h:	$(OBJDIR)/headers
$(OBJDIR)/sqlcmd_.c:	$(SRCDIR)/sqlcmd.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/sqlcmd.c >$(OBJDIR)/sqlcmd_.c

$(OBJDIR)/sqlcmd.o:	$(OBJDIR)/sqlcmd_.c $(OBJDIR)/sqlcmd.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/sqlcmd.o -c $(OBJDIR)/sqlcmd_.c

sqlcmd.h:	$(OBJDIR)/headers
$(OBJDIR)/stash_.c:	$(SRCDIR)/stash.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/stash.c >$(OBJDIR)/stash_.c

$(OBJDIR)/stash.o:	$(OBJDIR)/stash_.c $(OBJDIR)/stash.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/stash.o -c $(OBJDIR)/stash_.c

stash.h:	$(OBJDIR)/headers
$(OBJDIR)/stat_.c:	$(SRCDIR)/stat.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/stat.c >$(OBJDIR)/stat_.c

$(OBJDIR)/stat.o:	$(OBJDIR)/stat_.c $(OBJDIR)/stat.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/stat.o -c $(OBJDIR)/stat_.c

stat.h:	$(OBJDIR)/headers
$(OBJDIR)/style_.c:	$(SRCDIR)/style.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/style.c >$(OBJDIR)/style_.c

$(OBJDIR)/style.o:	$(OBJDIR)/style_.c $(OBJDIR)/style.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/style.o -c $(OBJDIR)/style_.c

style.h:	$(OBJDIR)/headers
$(OBJDIR)/sync_.c:	$(SRCDIR)/sync.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/sync.c >$(OBJDIR)/sync_.c

$(OBJDIR)/sync.o:	$(OBJDIR)/sync_.c $(OBJDIR)/sync.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/sync.o -c $(OBJDIR)/sync_.c

sync.h:	$(OBJDIR)/headers
$(OBJDIR)/tag_.c:	$(SRCDIR)/tag.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/tag.c >$(OBJDIR)/tag_.c

$(OBJDIR)/tag.o:	$(OBJDIR)/tag_.c $(OBJDIR)/tag.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/tag.o -c $(OBJDIR)/tag_.c

tag.h:	$(OBJDIR)/headers
$(OBJDIR)/th_main_.c:	$(SRCDIR)/th_main.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/th_main.c >$(OBJDIR)/th_main_.c

$(OBJDIR)/th_main.o:	$(OBJDIR)/th_main_.c $(OBJDIR)/th_main.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/th_main.o -c $(OBJDIR)/th_main_.c

th_main.h:	$(OBJDIR)/headers
$(OBJDIR)/timeline_.c:	$(SRCDIR)/timeline.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/timeline.c >$(OBJDIR)/timeline_.c

$(OBJDIR)/timeline.o:	$(OBJDIR)/timeline_.c $(OBJDIR)/timeline.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/timeline.o -c $(OBJDIR)/timeline_.c

timeline.h:	$(OBJDIR)/headers
$(OBJDIR)/tkt_.c:	$(SRCDIR)/tkt.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/tkt.c >$(OBJDIR)/tkt_.c

$(OBJDIR)/tkt.o:	$(OBJDIR)/tkt_.c $(OBJDIR)/tkt.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/tkt.o -c $(OBJDIR)/tkt_.c

tkt.h:	$(OBJDIR)/headers
$(OBJDIR)/tktsetup_.c:	$(SRCDIR)/tktsetup.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/tktsetup.c >$(OBJDIR)/tktsetup_.c

$(OBJDIR)/tktsetup.o:	$(OBJDIR)/tktsetup_.c $(OBJDIR)/tktsetup.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/tktsetup.o -c $(OBJDIR)/tktsetup_.c

tktsetup.h:	$(OBJDIR)/headers
$(OBJDIR)/undo_.c:	$(SRCDIR)/undo.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/undo.c >$(OBJDIR)/undo_.c

$(OBJDIR)/undo.o:	$(OBJDIR)/undo_.c $(OBJDIR)/undo.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/undo.o -c $(OBJDIR)/undo_.c

undo.h:	$(OBJDIR)/headers
$(OBJDIR)/update_.c:	$(SRCDIR)/update.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/update.c >$(OBJDIR)/update_.c

$(OBJDIR)/update.o:	$(OBJDIR)/update_.c $(OBJDIR)/update.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/update.o -c $(OBJDIR)/update_.c

update.h:	$(OBJDIR)/headers
$(OBJDIR)/url_.c:	$(SRCDIR)/url.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/url.c >$(OBJDIR)/url_.c

$(OBJDIR)/url.o:	$(OBJDIR)/url_.c $(OBJDIR)/url.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/url.o -c $(OBJDIR)/url_.c

url.h:	$(OBJDIR)/headers
$(OBJDIR)/user_.c:	$(SRCDIR)/user.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/user.c >$(OBJDIR)/user_.c

$(OBJDIR)/user.o:	$(OBJDIR)/user_.c $(OBJDIR)/user.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/user.o -c $(OBJDIR)/user_.c

user.h:	$(OBJDIR)/headers
$(OBJDIR)/verify_.c:	$(SRCDIR)/verify.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/verify.c >$(OBJDIR)/verify_.c

$(OBJDIR)/verify.o:	$(OBJDIR)/verify_.c $(OBJDIR)/verify.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/verify.o -c $(OBJDIR)/verify_.c

verify.h:	$(OBJDIR)/headers
$(OBJDIR)/vfile_.c:	$(SRCDIR)/vfile.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/vfile.c >$(OBJDIR)/vfile_.c

$(OBJDIR)/vfile.o:	$(OBJDIR)/vfile_.c $(OBJDIR)/vfile.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/vfile.o -c $(OBJDIR)/vfile_.c

vfile.h:	$(OBJDIR)/headers
$(OBJDIR)/wiki_.c:	$(SRCDIR)/wiki.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/wiki.c >$(OBJDIR)/wiki_.c

$(OBJDIR)/wiki.o:	$(OBJDIR)/wiki_.c $(OBJDIR)/wiki.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/wiki.o -c $(OBJDIR)/wiki_.c

wiki.h:	$(OBJDIR)/headers
$(OBJDIR)/wikiformat_.c:	$(SRCDIR)/wikiformat.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/wikiformat.c >$(OBJDIR)/wikiformat_.c

$(OBJDIR)/wikiformat.o:	$(OBJDIR)/wikiformat_.c $(OBJDIR)/wikiformat.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/wikiformat.o -c $(OBJDIR)/wikiformat_.c

wikiformat.h:	$(OBJDIR)/headers
$(OBJDIR)/winhttp_.c:	$(SRCDIR)/winhttp.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/winhttp.c >$(OBJDIR)/winhttp_.c

$(OBJDIR)/winhttp.o:	$(OBJDIR)/winhttp_.c $(OBJDIR)/winhttp.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/winhttp.o -c $(OBJDIR)/winhttp_.c

winhttp.h:	$(OBJDIR)/headers
$(OBJDIR)/xfer_.c:	$(SRCDIR)/xfer.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/xfer.c >$(OBJDIR)/xfer_.c

$(OBJDIR)/xfer.o:	$(OBJDIR)/xfer_.c $(OBJDIR)/xfer.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/xfer.o -c $(OBJDIR)/xfer_.c

xfer.h:	$(OBJDIR)/headers
$(OBJDIR)/zip_.c:	$(SRCDIR)/zip.c $(OBJDIR)/translate
	$(OBJDIR)/translate $(SRCDIR)/zip.c >$(OBJDIR)/zip_.c

$(OBJDIR)/zip.o:	$(OBJDIR)/zip_.c $(OBJDIR)/zip.h  $(SRCDIR)/config.h
	$(XTCC) -o $(OBJDIR)/zip.o -c $(OBJDIR)/zip_.c

zip.h:	$(OBJDIR)/headers
$(OBJDIR)/sqlite3.o:	$(SRCDIR)/sqlite3.c
	$(XTCC) -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 -DSQLITE_DEFAULT_FILE_FORMAT=4 -Dlocaltime=fossil_localtime -DSQLITE_ENABLE_LOCKING_STYLE=0 -c $(SRCDIR)/sqlite3.c -o $(OBJDIR)/sqlite3.o

$(OBJDIR)/shell.o:	$(SRCDIR)/shell.c
	$(XTCC) -Dmain=sqlite3_shell -DSQLITE_OMIT_LOAD_EXTENSION=1 -c $(SRCDIR)/shell.c -o $(OBJDIR)/shell.o

$(OBJDIR)/th.o:	$(SRCDIR)/th.c
	$(XTCC) -I$(SRCDIR) -c $(SRCDIR)/th.c -o $(OBJDIR)/th.o

$(OBJDIR)/th_lang.o:	$(SRCDIR)/th_lang.c
	$(XTCC) -I$(SRCDIR) -c $(SRCDIR)/th_lang.c -o $(OBJDIR)/th_lang.o

