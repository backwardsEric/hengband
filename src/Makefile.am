## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = hengband

hengband_SOURCES = \
	artifact.c artifact.h \
	\
	angband.h autopick.c \
	\
	avatar.h avatar.c \
	\
	birth.c bldg.c chest.c chest.h chuukei.c \
	cmd2.c cmd4.c cmd-activate.c cmd-activate.h cmd-eat.c cmd-eat.h \
	cmd-item.c cmd-item.h cmd-magiceat.c cmd-magiceat.h \
	cmd-mane.c cmd-mane.h cmd-pet.c cmd-pet.h \
	cmd-quaff.c cmd-quaff.h cmd-read.c cmd-read.h cmd-spell.c cmd-spell.h \
	cmd-usestaff.c cmd-usestaff.h cmd-zaprod.c cmd-zaprod.h cmd-zapwand.c \
	cmd-zapwand.h cmd-smith.c cmd-smith.h cmd-hissatsu.c cmd-hissatsu.h\
	\
	defines.h dungeon.c effects.c externs.h files.c flavor.c \
	\
	floor.h floor-events.c floor-events.h floor-generate.c floor-generate.h \
	floor-save.h floor-save.c \
	floor-streams.c floor-streams.h \
	\
	feature.c feature.h \
	geometry.c geometry.h \
	\
	gameoption.c gameoption.h gamevalue.h \
	generate.h grid.c grid.h h-basic.h h-config.h h-define.h \
	history.h history.c \
	h-system.h h-type.h inet.c init1.c init2.c init.h japanese.c load.c \
	main.c main-x11.c main-gcu.c \
	melee.h melee1.c mind.c \
	\
	monster.h \
	monsterrace-hook.c monsterrace-hook.h monster-process.c \
	monster-status.c monster-status.h monster1.c \
	monster2.c mspells1.c mspells2.c mspells3.c mspells4.c monster-spell.h \
	\
	mutation.h mutation.c \
	\
	object-boost.c object-boost.h \
	object-curse.c object-curse.h object-broken.c object-broken.h \
	object-hook.c object-hook.h object1.c object2.c \
	objectkind-hook.c objectkind-hook.h \
	\
	patron.h patron.c \
	\
	projection.h projection.c\
	\
	player-move.c player-move.h player-damage.c player-damage.h player-status.c player-status.h \
	\
	quest.h quest.c\
	\
	racial.c \
	realm-arcane.c \
	realm-arcane.h realm-chaos.c realm-chaos.h realm-craft.c realm-craft.h \
	realm-crusade.c realm-crusade.h realm-daemon.c realm-daemon.h realm-death.c \
	realm-death.h realm-hex.c realm-hex.h realm-hissatsu.c realm-hissatsu.h \
	realm-life.c realm-life.h realm-nature.c realm-nature.h realm-song.c \
	realm-song.h realm-sorcery.c realm-sorcery.h realm-trump.c realm-trump.h \
	report.c rooms.c rooms-city.c rooms-city.h rooms-fractal.c rooms-fractal.h \
	rooms.h rooms-normal.c rooms-normal.h rooms-pitnest.c rooms-pitnest.h \
	rooms-special.c rooms-special.h rooms-trap.c rooms-trap.h rooms-vault.c \
	rooms-vault.h save.c scores.c selfinfo.c selfinfo.h shoot.c shoot.h snipe.c \
	\
	rumor.h rumor.c \
	\
	spells.h \
	spells1.c spells2.c spells3.c \
	spells-diceroll.c spells-diceroll.h \
	spells-floor.c spells-floor.h \
	spells-object.c spells-object.h \
	spells-status.c spells-status.h \
	spells-summon.c spells-summon.h \
	spells-world.c spells-world.h \
	\
	sort.c sort.h \
	store.h store.c tables.c term.c term.h trap.c trap.h types.h util.c \
	variable.c wild.h wild.c wizard1.c wizard2.c \
	warning.c warning.h world.c world.h \
	\
	view-mainwindow.c\
	\
	xtra2.c z-config.h \
	z-form.c z-form.h z-rand.c z-rand.h z-term.c z-term.h z-util.c z-util.h \
	z-virt.c z-virt.h 

EXTRA_hengband_SOURCES = \
	angband.ico angband.rc ang_eng.rc maid-x11.c main-mac.c main-win.c \
	main-cap.c main-dos.c main-ibm.c main-xaw.c \
	makefile.bcc makefile.std makefile.dos makefile.ibm \
	readdib.c readdib.h wall.bmp

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
	gcc-wrap \
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

COMPILE = $(srcdir)/gcc-wrap $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
OBJCCOMPILE = $(srcdir)/gcc-wrap $(OBJC) $(DEFS) $(DEFAULT_INCLUDES) \
	$(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_OBJCFLAGS) $(OBJCFLAGS)

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
