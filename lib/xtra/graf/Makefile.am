## Makefile.am -- Process this file with automake to process Makefile.in

hengband_tilesets = \
	8x8.bmp \
	16x16.bmp \
	16x16-mask.bmp

angband4_conf = list.txt
converted_tilesets = \
	old/8x8.png \
	adam-bolt/16x16.png

EXTRA_DIST = \
  delete.me \
  $(hengband_tilesets) \
  $(angband4_conf) \
  osx_bmp2png.py \
  $(converted_tilesets)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/xtra/graf

angband_DATA = \
  $(hengband_tilesets)
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
	$(converted_tilesets)
endif
