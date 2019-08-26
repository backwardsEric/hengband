## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

visual_studio_files = \
	Hengband_vcs2015/Hengband.sln \
	Hengband_vcs2015/Hengband/Hengband.vcxproj \
	Hengband_vcs2017/Hengband.sln \
	Hengband_vcs2017/Hengband/Hengband.vcxproj \
	Hengband_vcs2017/Hengband/Hengband.vcxproj.filters

EXTRA_DIST = \
	autopick.txt \
	autopick_eng.txt \
	readme.txt \
	readme_angband \
	readme_eng.txt \
	hengband.spec \
	$(visual_studio_files)

SUBDIRS = src lib
