## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = apex bone data edit file help info pref save script user

## lib/xtra はサブモジュールで、無くてもビルドできるようにここで扱う (インストールは HAVE_XTRA のときだけ行う)。
## 配布物には必ず含めるため EXTRA_DIST は条件なしとし、サブモジュールが無いと make dist は失敗する。

xtra_graf_hengband_tilesets = \
	xtra/graf/8x8.bmp

xtra_graf_angband4_conf = xtra/graf/list.txt
xtra_graf_converted_tilesets = \
	xtra/graf/old/8x8.png

xtra_music_misc = \
	xtra/music/delete.me \
	xtra/music/desktop.ini \
	xtra/music/readme.txt

xtra_music_conf = \
	xtra/music/music.cfg

xtra_music_files = \
	xtra/music/The_Serpent_of_Chaos.mp3 \
	xtra/music/ambush.mp3 \
	xtra/music/arena.mp3 \
	xtra/music/battle.mp3 \
	xtra/music/build.mp3 \
	xtra/music/dun_high1.mp3 \
	xtra/music/dun_high2.mp3 \
	xtra/music/dun_high3.mp3 \
	xtra/music/dun_high4.mp3 \
	xtra/music/dun_high5.mp3 \
	xtra/music/dun_low1.mp3 \
	xtra/music/dun_low2.mp3 \
	xtra/music/dun_low3.mp3 \
	xtra/music/dun_low4.mp3 \
	xtra/music/dun_low5.mp3 \
	xtra/music/dun_med1.mp3 \
	xtra/music/dun_med2.mp3 \
	xtra/music/dun_med3.mp3 \
	xtra/music/dun_med4.mp3 \
	xtra/music/dun_med5.mp3 \
	xtra/music/exit.mp3 \
	xtra/music/feel1.mp3 \
	xtra/music/feel2.mp3 \
	xtra/music/field_high1.mp3 \
	xtra/music/field_high2.mp3 \
	xtra/music/field_high3.mp3 \
	xtra/music/field_low1.mp3 \
	xtra/music/field_low2.mp3 \
	xtra/music/field_low3.mp3 \
	xtra/music/field_med1.mp3 \
	xtra/music/field_med2.mp3 \
	xtra/music/field_med3.mp3 \
	xtra/music/gameover.mp3 \
	xtra/music/new_game.mp3 \
	xtra/music/quest1.mp3 \
	xtra/music/quest2.mp3 \
	xtra/music/quest3.mp3 \
	xtra/music/quest4.mp3 \
	xtra/music/quest_clear.mp3 \
	xtra/music/town1.mp3 \
	xtra/music/town2.mp3 \
	xtra/music/town3.mp3 \
	xtra/music/town4.mp3 \
	xtra/music/town5.mp3 \
	xtra/music/wild.mp3 \
	xtra/music/winner.mp3

xtra_sound_misc = \
	xtra/sound/readme.txt

xtra_sound_conf = \
	xtra/sound/sound.cfg

xtra_sound_files = \
	xtra/sound/bite.wav \
	xtra/sound/breath.wav \
	xtra/sound/buy.wav \
	xtra/sound/claw.wav \
	xtra/sound/critical.wav \
	xtra/sound/crush.wav \
	xtra/sound/damage_over_time.wav \
	xtra/sound/destitem.wav \
	xtra/sound/down.wav \
	xtra/sound/drop1.wav \
	xtra/sound/drop2.wav \
	xtra/sound/eat1.wav \
	xtra/sound/eat2.wav \
	xtra/sound/eat3.wav \
	xtra/sound/explode.wav \
	xtra/sound/evil.wav \
	xtra/sound/fail.wav \
	xtra/sound/fall.wav \
	xtra/sound/flee.wav \
	xtra/sound/glass.wav \
	xtra/sound/hit1.wav \
	xtra/sound/hit2.wav \
	xtra/sound/hit3.wav \
	xtra/sound/hitwall1.wav \
	xtra/sound/hitwall2.wav \
	xtra/sound/hitwall3.wav \
	xtra/sound/hungry1.wav \
	xtra/sound/hungry2.wav \
	xtra/sound/kill1.wav \
	xtra/sound/kill2.wav \
	xtra/sound/level.wav \
	xtra/sound/miss1.wav \
	xtra/sound/miss2.wav \
	xtra/sound/miss3.wav \
	xtra/sound/moan.wav \
	xtra/sound/n_kill.wav \
	xtra/sound/opendoor1.wav \
	xtra/sound/opendoor2.wav \
	xtra/sound/opendoor3.wav \
	xtra/sound/pain.wav \
	xtra/sound/quaff.wav \
	xtra/sound/reflect.wav \
	xtra/sound/scroll.wav \
	xtra/sound/sell.wav \
	xtra/sound/shoot.wav \
	xtra/sound/shoot_hit1.wav \
	xtra/sound/shoot_hit2.wav \
	xtra/sound/show.wav \
	xtra/sound/shutdoor1.wav \
	xtra/sound/shutdoor2.wav \
	xtra/sound/shutdoor3.wav \
	xtra/sound/slash.wav \
	xtra/sound/slime.wav \
	xtra/sound/stairway1.wav \
	xtra/sound/stairway2.wav \
	xtra/sound/sting.wav \
	xtra/sound/store1.wav \
	xtra/sound/store2.wav \
	xtra/sound/study.wav \
	xtra/sound/summon.wav \
	xtra/sound/take_off.wav \
	xtra/sound/teleport.wav \
	xtra/sound/terrain_damage.wav \
	xtra/sound/touch.wav \
	xtra/sound/tplevel.wav \
	xtra/sound/tpother.wav \
	xtra/sound/wail.wav \
	xtra/sound/warn.wav \
	xtra/sound/wield.wav \
	xtra/sound/zap.wav

xtra_sound_unused_files = \
	xtra/sound/apple1-r.wav \
	xtra/sound/decision25r.wav \
	xtra/sound/enemy-advent1-r.wav \
	xtra/sound/se_maoudamashii_battle07.wav \
	xtra/sound/se_maoudamashii_retro22.wav \
	xtra/sound/se_maoudamashii_se_drink01.wav \
	xtra/sound/se_maoudamashii_voice_monster01.wav \
	xtra/sound/sea-lion2-r.wav \
	xtra/sound/sword-clash1-r.wav \
	xtra/sound/sword-gesture3-r.wav \
	xtra/sound/sword-slash3-r.wav

EXTRA_DIST = \
	$(xtra_graf_hengband_tilesets) \
	$(xtra_graf_angband4_conf) \
	xtra/graf/osx_bmp2png.py \
	$(xtra_graf_converted_tilesets) \
	$(xtra_music_misc) \
	$(xtra_music_conf) \
	$(xtra_music_files) \
	$(xtra_sound_misc) \
	$(xtra_sound_conf) \
	$(xtra_sound_files) \
	$(xtra_sound_unused_files)

if SET_GID
if HAVE_XTRA
xtragrafdir = @DEFAULT_LIB_PATH@/xtra/graf

xtragraf_DATA = \
	$(xtra_graf_hengband_tilesets)
endif
endif

if COCOA
# APPNAME to APPRES duplicate what's in src/Makefile.am.  It would be nice to
# avoid that, but until then, they should match.
APPNAME = $(PACKAGE_NAME)
APPDIR = $(APPNAME).app
APPBNDL = $(bindir)/$(APPDIR)
APPCONT = $(APPBNDL)/Contents
APPRES = $(APPCONT)/Resources

appdatadir = $(APPRES)/lib
nobase_appdata_DATA = \
	$(xtra_graf_angband4_conf) \
	$(xtra_graf_converted_tilesets) \
	$(xtra_music_conf) \
	$(xtra_music_files) \
	$(xtra_sound_conf) \
	$(xtra_sound_files)

endif
