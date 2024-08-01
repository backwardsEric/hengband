## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = hengband

hengband_SOURCES = \
	z-util.c z-virt.c z-form.c z-rand.c z-term.c \
	variable.c tables.c util.c cave.c \
	kajitips.h object1.c object2.c monster1.c monster2.c \
	xtra1.c xtra2.c spells1.c spells2.c \
	melee1.c melee2.c save.c files.c floors.c\
	cmd1.c cmd2.c cmd3.c cmd4.c cmd5.c cmd6.c \
	store.c birth.c load.c \
	wizard1.c wizard2.c grid.c streams.c rooms.c \
	generate.c dungeon.c init1.c init2.c \
	effects.c racial.c inet.c report.c\
	artifact.c autopick.c mutation.c flavor.c spells3.c \
	mspells1.c mspells2.c scores.c mind.c mane.c hissatsu.c \
	bldg.c obj_kind.c wild.c avatar.c japanese.c mspells3.c \
	main-cap.c main-gcu.c main-x11.c main-xaw.c main.c chuukei.c \
	angband.h defines.h externs.h generate.h grid.h \
	h-basic.h h-config.h h-define.h h-system.h h-type.h \
	mindtips.h readdib.h rooms.h spellstips.h streams.h init.h \
	types.h z-config.h z-form.h z-rand.h z-term.h z-util.h z-virt.h

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-mac.c main-win.c \
	main-dos.c main-ibm.c \
	makefile.bcc makefile.std makefile.dos makefile.ibm \
	readdib.c wall.bmp

cocoa_xcode_files = \
	cocoa/AppDelegate.m \
	cocoa/Base.lproj/MainMenu.xib
cocoa_icon_files = \
	cocoa/hengband_Icons.icns \
	cocoa/Save.icns \
	cocoa/Edit.icns \
	cocoa/Data.icns
cocoa_plist_template = cocoa/Angband-Cocoa.xml
cocoa_plist_strings_template = cocoa/Angband-Cocoa.strings
cocoa_plist_files = \
	cocoa/CommandMenu.plist
cocoa_en_nib_files = \
	cocoa/Base.lproj/MainMenu.nib
cocoa_en_strings_files = \
	cocoa/en.lproj/Localizable.strings \
	cocoa/en.lproj/CommandMenu.strings \
	cocoa/en.lproj/GraphicsMenu.strings
cocoa_ja_strings_files = \
	cocoa/ja.lproj/MainMenu.strings \
	cocoa/ja.lproj/Localizable.strings \
	cocoa/ja.lproj/CommandMenu.strings \
	cocoa/ja.lproj/GraphicsMenu.strings

EXTRA_DIST = \
	$(cocoa_xcode_files) \
	$(cocoa_icon_files) \
	$(cocoa_plist_template) \
	$(cocoa_plist_strings_template) \
	$(cocoa_plist_files) \
	$(cocoa_en_nib_files) \
	$(cocoa_en_strings_files) \
	$(cocoa_ja_strings_files)

if COCOA
hengband_SOURCES += main-cocoa.m grafmode.h grafmode.c cocoa/AppDelegate.h
AM_CFLAGS = -mmacosx-version-min=10.11 -Wunguarded-availability
AM_OBJCFLAGS = -fobjc-arc -mmacosx-version-min=10.11 -Wunguarded-availability
hengband_LDFLAGS = -framework cocoa $(AM_LDFLAGS)
hengband_LINK = MACOSX_DEPLOYMENT_TARGET=10.11 $(OBJCLINK) $(hengband_LDFLAGS) $(LDFLAGS) -o $@
APPNAME = $(PACKAGE_NAME)
APPEXE = hengband
APPDIR = $(APPNAME).app
BUNDLE_IDENTIFIER = jp.osdn.hengband
BUNDLE_VERSION = $(PACKAGE_VERSION)
BUNDLE_DISPLAY_NAME = $(APPNAME)
BUNDLE_NAME = $(BUNDLE_DISPLAY_NAME)
BUNDLE_DISPLAY_NAME_JA = 変愚蛮怒
BUNDLE_NAME_JA = $(BUNDLE_DISPLAY_NAME_JA)
# Be careful with characters (like '&') in the copyright that have special
# meanings to sed.
COPYRIGHT = Copyright © Mr. Hoge and many others
# For now, using the line that appears in news_j.txt.  Is that appropriate?
# The ampersand is a kanji so it doesn't seem to be treated as a special
# character by sed.
COPYRIGHT_JA = Mr.hoge (echizen＠users.sourceforge.jp) ＆ 多くの方々
APPBNDL = $(bindir)/$(APPDIR)
APPCONT = $(APPBNDL)/Contents
APPBIN = $(APPCONT)/MacOS
APPRES = $(APPCONT)/Resources
appbin_PROGRAMS = $(APPEXE)
appbindir = $(APPBIN)
dist_appicon_DATA = $(cocoa_icon_files)
appicondir = $(APPRES)
appplist_DATA = $(cocoa_plist_files)
appplistdir = $(APPRES)
appennib_DATA = $(cocoa_en_nib_files)
appennibdir = $(APPRES)/Base.lproj
appen_DATA = $(cocoa_en_strings_files)
appendir = $(APPRES)/en.lproj
appja_DATA = $(cocoa_ja_strings_files)
appjadir = $(APPRES)/ja.lproj
else
EXTRA_hengband_SOURCES += main-cocoa.m grafmode.h grafmode.c \
	cocoa/AppDelegate.h
hengband_LINK = $(LINK)
endif

install-exec-hook:
if SET_GID
	chgrp "@GAMEGROUP@" "$(DESTDIR)$(bindir)/hengband"
	chmod g+s "$(DESTDIR)$(bindir)/hengband"
endif

if COCOA
# Suppress the warnings from the EUC-JP encoded strings used when building
# the Japanese version.
AM_CFLAGS += -Wno-invalid-source-encoding
AM_OBJCFLAGS += -Wno-invalid-source-encoding

# Create lib/info within the application bundle since not currently
# installing anything to it.  Other relevant lib directories should
# be created from the Makefiles in the lib directory.
install-data-local:
	@mkdir -p $(APPCONT)
	@mkdir -p $(appendir)
	@mkdir -p $(appjadir)
	@mkdir -p $(APPRES)/lib/info
	sed -e 's/\$$VERSION\$$/$(PACKAGE_VERSION)/' \
		-e 's/\$$COPYRIGHT\$$/$(COPYRIGHT)/' \
		-e 's/\$$NAME\$$/$(APPNAME)/' \
		-e 's/\$$EXECUTABLE\$$/$(APPEXE)/' \
		-e 's/\$$BUNDLE_VERSION\$$/$(BUNDLE_VERSION)/' \
		-e 's/\$$BUNDLE_IDENTIFIER\$$/$(BUNDLE_IDENTIFIER)/' \
		-e 's/\$$BUNDLE_DISPLAY_NAME\$$/$(BUNDLE_DISPLAY_NAME)/' \
		-e 's/\$$BUNDLE_NAME\$$/$(BUNDLE_NAME)/' \
		$(srcdir)/$(cocoa_plist_template) > $(APPCONT)/Info.plist
	sed -e 's/\$$VERSION\$$/$(PACKAGE_VERSION)/' \
		-e 's/\$$COPYRIGHT\$$/$(COPYRIGHT)/' \
		-e 's/\$$BUNDLE_DISPLAY_NAME\$$/$(BUNDLE_DISPLAY_NAME)/' \
		-e 's/\$$BUNDLE_NAME\$$/$(BUNDLE_NAME)/' \
		$(srcdir)/$(cocoa_plist_strings_template) > $(appendir)/InfoPlist.strings
	sed -e 's/\$$VERSION\$$/$(PACKAGE_VERSION)/' \
		-e 's/\$$COPYRIGHT\$$/$(COPYRIGHT_JA)/' \
		-e 's/\$$BUNDLE_DISPLAY_NAME\$$/$(BUNDLE_DISPLAY_NAME_JA)/' \
		-e 's/\$$BUNDLE_NAME\$$/$(BUNDLE_NAME_JA)/' \
		$(srcdir)/$(cocoa_plist_strings_template) > $(appjadir)/InfoPlist.strings

uninstall-local:
	-rm $(APPCONT)/Info.plist
	-rm $(appendir)/InfoPlist.strings
	-rm $(appjadir)/InfoPlist.strings

endif

distclean-local:
	-rm -r -f $(DEPDIR)
