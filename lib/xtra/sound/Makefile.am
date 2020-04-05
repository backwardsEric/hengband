## Makefile.am -- Process this file with automake to process Makefile.in

EXTRA_DIST = \
	sound.cfg \
	$(sound_files)

sound_files =

if COCOA
# APPNAME to APPRES duplicate what's in src/Makefile.am.  It would be nice to
# avoid that, but until then, they should match.
APPNAME = $(PACKAGE_NAME)
APPDIR = $(APPNAME).app
APPBNDL = $(bindir)/$(APPDIR)
APPCONT = $(APPBNDL)/Contents
APPRES = $(APPCONT)/Resources

appsounddir = $(APPRES)/lib/xtra/sound
appsound_DATA = \
	sound.cfg $(sound_files)
endif
