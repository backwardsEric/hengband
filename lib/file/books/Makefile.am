## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	book-000_en.txt book-000_jp.txt \
	book-001_en.txt book-001_jp.txt \
	book-002_en.txt book-002_jp.txt \
	book-003_en.txt book-003_jp.txt \
	book-004_en.txt book-004_jp.txt \
	book-005_en.txt book-005_jp.txt \
	book-006_en.txt book-006_jp.txt \
	book-007_en.txt book-007_jp.txt

EXTRA_DIST = \
	$(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/file/books

angband_DATA = \
	$(angband_files)
endif

if COCOA
# APPNAME to APPRES duplicate what's in src/Makefile.am.  It would be nice to
# avoid that, but until then, they should match.
APPNAME = $(PACKAGE_NAME)
APPDIR = $(APPNAME).app
APPBNDL = $(bindir)/$(APPDIR)
APPCONT = $(APPBNDL)/Contents
APPRES = $(APPCONT)/Resources

appfiledir = $(APPRES)/lib/file/books
appfile_DATA = \
	$(angband_files)
endif
