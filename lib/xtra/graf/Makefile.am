## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	8x8.bmp

angband4_conf = list.txt
angband4_tile_files = \
	old/8x8.png \
	adam-bolt/16x16.png

EXTRA_DIST = \
  delete.me $(angband_files) $(angband4_conf) $(angband4_tile_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/xtra/graf

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

appgrafdir = $(APPRES)/lib/xtra/graf
appgraf_DATA = \
	$(angband4_conf)
nobase_appgraf_DATA = \
	$(angband4_tile_files)
endif
