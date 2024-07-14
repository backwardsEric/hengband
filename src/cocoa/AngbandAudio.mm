/**
 * \file AngbandAudio.mm
 * \brief Define an interface for handling incidental sounds and background
 * music in the OS X front end.
 */

#import <Cocoa/Cocoa.h>
#import "AngbandAudio.h"
#include "io/files-util.h"
#include "main/music-definitions-table.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "util/angband-files.h"
#include <limits.h>
#include <string.h>
#include <ctype.h>


/*
 * Helper functions to extract the appropriate number ID from a name in
 * music.cfg.  Return true if the extraction failed and false if the extraction
 * succeeded with *id holding the result.
 */
static bool get_basic_id(const char *s, int *id)
{
	int i = 0;

	while (1) {
		if (i >= MUSIC_BASIC_MAX) {
			return true;
		}
		if (!strcmp(angband_music_basic_name[i], s)) {
			*id = i;
			return false;
		}
		++i;
	}
}


static bool get_dungeon_id(const char *s, int *id)
{
	if (strcmp(s, "dungeon") == 0) {
		char *pe;
		long lv = strtol(s + 7, &pe, 10);

		if (pe != s && !*pe && lv > INT_MIN && lv < INT_MAX) {
			*id = (int)lv;
			return false;
		}
	}
	return true;
}


static bool get_quest_id(const char *s, int *id)
{
	if (strcmp(s, "quest") == 0) {
		char *pe;
		long lv = strtol(s + 5, &pe, 10);

		if (pe != s && !*pe && lv > INT_MIN && lv < INT_MAX) {
			*id = (int)lv;
			return false;
		}
	}
	return true;
}


static bool get_town_id(const char *s, int *id)
{
	if (strcmp(s, "town") == 0) {
		char *pe;
		long lv = strtol(s + 4, &pe, 10);

		if (pe != s && !*pe && lv > INT_MIN && lv < INT_MAX) {
			*id = (int)lv;
			return false;
		}
	}
	return true;
}


static bool get_monster_id(const char *s, int *id)
{
	if (strcmp(s, "monster") == 0) {
		char *pe;
		long lv = strtol(s + 7, &pe, 10);

		if (pe != s && !*pe && lv > INT_MIN && lv < INT_MAX) {
			*id = (int)lv;
			return false;
		}
	}
	return true;
}


@implementation AngbandActiveAudio

/*
 * Handle property methods where need more than is provided by the default
 * synthesis.
 */
- (BOOL) isPlaying
{
	return self->player && [self->player isPlaying];
}


- (id)initWithPlayer:(AVAudioPlayer *)aPlayer fadeInBy:(NSInteger)fadeIn
		prior:(AngbandActiveAudio *)p paused:(BOOL)isPaused
{
	if (self = [super init]) {
		self->_priorAudio = p;
		if (p) {
			self->_nextAudio = p->_nextAudio;
			if (p->_nextAudio) {
				p->_nextAudio->_priorAudio = self;
			}
			p->_nextAudio = self;
		} else {
			self->_nextAudio = nil;
		}
		self->player = aPlayer;
		self->fadeTimer = nil;
		if (aPlayer) {
			aPlayer.delegate = self;
			if (fadeIn > 0) {
				float volume = [aPlayer volume];

				aPlayer.volume = 0;
				[aPlayer setVolume:volume
					fadeDuration:(fadeIn * .001)];
			}
			if (!isPaused) {
				[aPlayer play];
			}
		}
	}
	return self;
}


- (void)pause
{
	if (self->player) {
		[self->player pause];
	}
}


- (void)resume
{
	if (self->player) {
		[self->player play];
	}
}


- (void)stop
{
	if (self->player) {
		if (self->fadeTimer) {
			[self->fadeTimer invalidate];
			self->fadeTimer = nil;
		}
		[self->player stop];
	}
}


- (void)fadeOutBy:(NSInteger)t
{
	/* Only fade out if there's a track and it is not already fading out. */
	if (self->player && !self->fadeTimer) {
		[self->player setVolume:0.0f
			fadeDuration:(((t > 0) ? t : 0) * .001)];
		/* Set up a timer to remove the faded out track. */
		self->fadeTimer = [NSTimer
			scheduledTimerWithTimeInterval:(t * .001 + .01)
			target:self
			selector:@selector(handleFadeOutTimer:)
			userInfo:nil
			repeats:NO];
		self->fadeTimer.tolerance = 0.02;
	}
}


- (void)changeVolumeTo:(NSInteger)v
{
	if (self->player) {
		NSInteger safev;

		if (v < 0) {
			safev = 0;
		} else if (v > 100) {
			safev = 100;
		} else {
			safev = v;
		}
		self->player.volume = safev * .01f;
	}
}


- (void)handleFadeOutTimer:(NSTimer *)timer
{
	assert(self->player && self->fadeTimer == timer);
	self->fadeTimer = nil;
	[self->player stop];
}


- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)aPlayer
		successfully:(BOOL)flag
{
	assert(aPlayer == self->player);
	if (self->fadeTimer) {
		[self->fadeTimer invalidate];
		self->fadeTimer = nil;
	}
	self->player = nil;
	/* Unlink from the list of active tracks. */
	assert([self priorAudio] && [self nextAudio]);
	self->_priorAudio->_nextAudio = self->_nextAudio;
	self->_nextAudio->_priorAudio = self->_priorAudio;
	self->_priorAudio = nil;
	self->_nextAudio = nil;
}

@end


@implementation AngbandAudioManager

@synthesize soundVolume=_soundVolume;
@synthesize musicEnabled=_musicEnabled;
@synthesize musicVolume=_musicVolume;

/*
 * Handle property methods where need more than provided by the default
 * synthesis.
 */
- (void)setSoundVolume:(NSInteger)v
{
	/* Incidental sounds that are currently playing aren't changed. */
	if (v < 0) {
		self->_soundVolume = 0;
	} else if (v > 100) {
		self->_soundVolume = 100;
	} else {
		self->_soundVolume = v;
	}
}


- (void)setMusicEnabled:(BOOL)b
{
	BOOL old = self->_musicEnabled;

	self->_musicEnabled = b;

	if (!b && old) {
		AngbandActiveAudio *a = [self->tracksPlayingHead nextAudio];

		while (a) {
			AngbandActiveAudio *t = a;

			a = [a nextAudio];
			[t stop];
		};
	}
}


- (void)setMusicVolume:(NSInteger)v
{
	NSInteger old = self->_musicVolume;

	if (v < 0) {
		self->_musicVolume = 0;
	} else if (v > 100) {
		self->_musicVolume = 100;
	} else {
		self->_musicVolume = v;
	}

	if (v != old) {
		[[self->tracksPlayingTail priorAudio]
			changeVolumeTo:self->_musicVolume];
	}
}


/* Define methods. */
- (id)init
{
	if (self = [super init]) {
		self->tracksPlayingHead = [[AngbandActiveAudio alloc]
			initWithPlayer:nil fadeInBy:0 prior:nil paused:NO];
		self->tracksPlayingTail = [[AngbandActiveAudio alloc]
			initWithPlayer:nil fadeInBy:0
			prior:self->tracksPlayingHead paused:NO];
		self->soundArraysByEvent = nil;
		self->musicByTypeAndID = nil;
		self->appActive = YES;
		self->_beepEnabled = YES;
		self->_soundEnabled = YES;
		self->_soundVolume = 30;
		self->_musicEnabled = YES;
		self->_musicPausedWhenInactive = YES;
		self->_musicVolume = 20;
		self->_musicTransitionTime = 3000;
	}
	return self;
}


- (void)playBeep
{
	if (!self->appActive || ![self isBeepEnabled]) {
		return;
	}
	/*
	 * Use NSBeep() for this, though that means it doesn't heed the
	 * volume set by the soundVolume property.
	 */
	NSBeep();
}

- (void)playSound:(int)event
{
	if (!self->appActive || ![self isSoundEnabled]) {
		return;
	}

	/* Initialize when the first sound is played. */
	if (!self->soundArraysByEvent) {
		self->soundArraysByEvent =
			[AngbandAudioManager setupSoundArraysByEvent];
		if (!self->soundArraysByEvent) {
			return;
		}
	}

	@autoreleasepool {
		NSMutableArray *samples = [self->soundArraysByEvent
			objectForKey:[NSNumber numberWithInteger:event]];
		AVAudioPlayer *player;
		int s;

		if (!samples || !samples.count) {
			return;
		}

		s = randint0((int)samples.count);
		player = samples[s];

		if ([player isPlaying]) {
			[player stop];
			player.currentTime = 0;
		}
		player.volume = self.soundVolume * .01f;
		[player play];
	}
}


- (void)playMusicType:(int)t ID:(int)i
{
	if (![self isMusicEnabled]) {
		return;
	}

	/* Initialize when the first music track is played. */
	if (!self->musicByTypeAndID) {
		self->musicByTypeAndID =
			[AngbandAudioManager setupMusicByTypeAndID];
	}

	@autoreleasepool {
		NSMutableDictionary *musicByID = [self->musicByTypeAndID
			objectForKey:[NSNumber numberWithInteger:t]];
		NSMutableArray *paths;
                NSData *audioData;
		AVAudioPlayer *player;

		if (!musicByID) {
			return;
		}
		paths = [musicByID
			objectForKey:[NSNumber numberWithInteger:i]];
		if (!paths || !paths.count) {
			return;
		}
                audioData = [NSData dataWithContentsOfFile:paths[randint0((int)paths.count)]];
		player = [[AVAudioPlayer alloc] initWithData:audioData
			error:nil];

		if (player) {
			AngbandActiveAudio *prior_track =
				[self->tracksPlayingTail priorAudio];
			AngbandActiveAudio *active_track;
			NSInteger fade_time;

			player.volume = 0.01f * [self musicVolume];
			if ([prior_track isPlaying]) {
				fade_time = [self musicTransitionTime];
				if (fade_time < 0) {
					fade_time = 0;
				}
				[prior_track fadeOutBy:fade_time];
			} else {
				fade_time = 0;
			}
			active_track = [[AngbandActiveAudio alloc]
				initWithPlayer:player fadeInBy:fade_time
				prior:prior_track
				paused:(!self->appActive && [self isMusicPausedWhenInactive])];
		}
	}
}


- (BOOL)musicExists:(int)t ID:(int)i
{
	NSMutableDictionary *musicByID;
	BOOL exists;

	/* Initialize on the first call if it hasn't already been done. */
	if (!self->musicByTypeAndID) {
		self->musicByTypeAndID =
			[AngbandAudioManager setupMusicByTypeAndID];
	}

	musicByID = [self->musicByTypeAndID
		objectForKey:[NSNumber numberWithInteger:t]];

	if (musicByID) {
		NSString *path = [musicByID
			objectForKey:[NSNumber numberWithInteger:i]];

		exists = (path != nil);
	} else {
		exists = NO;
	}

	return exists;
}


- (void)stopAllMusic
{
	AngbandActiveAudio *track = [self->tracksPlayingHead nextAudio];

	while (1) {
		[track stop];
		track = [track nextAudio];
		if (!track) {
			break;
		}
	}
}


- (void)setupForInactiveApp
{
	if (!self->appActive) {
		return;
	}
	self->appActive = NO;
	if ([self isMusicPausedWhenInactive]) {
		AngbandActiveAudio *track =
			[self->tracksPlayingHead nextAudio];

		while (1) {
			AngbandActiveAudio *next_track = [track nextAudio];

			if (next_track != self->tracksPlayingTail) {
				/* Stop all tracks but the last one playing. */
				[track stop];
			} else {
				/*
				 * Pause the last track playing.  Set its
				 * volume to maximum so, when resumed, it'll
				 * play that way even if it was fading in when
				 * paused.
				 */
				[track pause];
				[track changeVolumeTo:[self musicVolume]];
			}
			track = next_track;
			if (!track) {
				break;
			}
		}
	}
}


- (void)setupForActiveApp
{
	if (self->appActive) {
		return;
	}
	self->appActive = YES;
	if ([self isMusicPausedWhenInactive]) {
		/* Resume any tracks that were playing. */
		AngbandActiveAudio *track =
			[self->tracksPlayingTail priorAudio];

		while (1) {
			[track resume];
			track = [track priorAudio];
			if (!track) {
				break;
			}
		}
	}
}


/* Set up the class properties. */
static NSInteger _maxSamples = 16;
+ (NSInteger)maxSamples
{
	return _maxSamples;
}


static AngbandAudioManager *_sharedManager = nil;
+ (AngbandAudioManager *)sharedManager
{
	if (!_sharedManager) {
		_sharedManager = [[AngbandAudioManager alloc] init];
	}
	return _sharedManager;
}


/* Define class methods. */
+ (void)clearSharedManager
{
	_sharedManager = nil;
}


+ (NSMutableDictionary *)setupSoundArraysByEvent
{
	std::filesystem::path psound, p;
	FILE *fff;
	NSMutableDictionary *arraysByEvent;

	/* Build the "sound" path. */
	psound = path_build(ANGBAND_DIR_XTRA, "sound");

	/* Find and open the config file. */
	p = path_build(psound, "sound.cfg");
	fff = angband_fopen(p, FileOpenMode::READ);

	if (!fff) {
		NSLog(@"The sound configuration file could not be opened");
		return nil;
	}

	arraysByEvent = [[NSMutableDictionary alloc] init];
	@autoreleasepool {
		/*
		 * This loop may take a while depending on the count and size
		 * of samples to load.
		 */
		const char white[] = " \t";
		NSMutableDictionary *playersByPath =
			[[NSMutableDictionary alloc] init];

		/* Parse the file. */
		/* Lines are always of the form "name = sample [sample ...]". */
		while (1) {
			const auto line_str = angband_fgets(fff);
			NSMutableArray *soundSamples;
			std::string msg_name;
			std::string sample_names;
			int match;
			size_t skip1, skip2, search;

			if (!line_str) {
				break;
			}

			/* Skip leading whitespace. */
			skip1 = line_str->find_first_not_of(white);

			/*
			 * Ignore anything not beginning with an alphabetic
			 * character.
			 */
			if (skip1 == std::string::npos
					|| !isalpha((unsigned char)(line_str->at(skip1)))) {
				continue;
			}

			/*
			 * Split the line into two; message name and the rest.
			 */
			search = line_str->find('=', skip1);
			if (search == std::string::npos) {
				continue;
			}
			skip2 = line_str->find_first_of(white, skip1);
			msg_name = line_str->substr(skip1,
				((search <= skip2) ? search : skip2) - skip1);
			skip2 = line_str->find_first_not_of(white, search + 1);
			if (skip2 != std::string::npos) {
				sample_names = line_str->substr(skip2);
			}

			/* Make sure this is a valid event name. */
			for (match = SOUND_MAX - 1; match >= 0; --match) {
				if (!strcmp(msg_name.data(),
						angband_sound_name[match])) {
					break;
				}
			}
			if (match < 0) {
				continue;
			}

			soundSamples = [arraysByEvent
				objectForKey:[NSNumber numberWithInteger:match]];
			if (!soundSamples) {
				soundSamples = [[NSMutableArray alloc] init];
				[arraysByEvent
					setObject:soundSamples
					forKey:[NSNumber numberWithInteger:match]];
			}

			/*
			 * Now find all the sample names and add them one by
			 * one.
			 */
			skip1 = 0;
			while (1) {
				int num;
				std::string sample_name;
				NSString *token_string;
				AVAudioPlayer *player;
				BOOL done;

				if (skip1 >= sample_names.length()) {
					break;
				}
				/* Terminate the current token. */
				skip2 = sample_names.find_first_of(white, skip1);
				if (skip2 == std::string::npos) {
					sample_name = sample_names.substr(skip1);
					done = TRUE;
				} else {
					sample_name = sample_names.substr(skip1,
						skip2 - skip1);
					done = FALSE;
				}

				/* Don't allow too many samples. */
				num = (int)soundSamples.count;
				if (num >= [AngbandAudioManager maxSamples]) {
					break;
				}

				token_string = [NSString
					stringWithUTF8String:sample_name.data()];
				player = [playersByPath
					objectForKey:token_string];
				if (!player) {
					/*
					 * We have to load the sound.
					 * Build the path to the sample.
					 */
					struct stat stb;

					p = path_build(psound, sample_name.data());
					if (stat(p.native().data(), &stb) == 0
							&& (stb.st_mode & S_IFREG)) {
						NSData *audioData = [NSData
							dataWithContentsOfFile:[NSString stringWithUTF8String:p.native().data()]];

						player = [[AVAudioPlayer alloc]
							initWithData:audioData
							error:nil];
						if (player) {
							[playersByPath
								setObject:player
								forKey:token_string];
						}
					}
				}

				/* Store it if it was loaded. */
				if (player) {
					[soundSamples addObject:player];
				}

				if (done) {
					break;
				}
				skip1 = sample_names.find_first_not_of(white,
					skip2 + 1);
				if (skip1 == std::string::npos) {
					break;
				}
			}
		}
		playersByPath = nil;
	}
	angband_fclose(fff);

	return arraysByEvent;
}


+ (NSMutableDictionary *)setupMusicByTypeAndID
{
	struct {
		const char * name;
		int type_code;
		bool *p_has;
		bool (*name2id_func)(const char*, int*);
	} sections_of_interest[] = {
		{ "Basic", TERM_XTRA_MUSIC_BASIC, NULL, get_basic_id },
		{ "Dungeon", TERM_XTRA_MUSIC_DUNGEON, NULL, get_dungeon_id },
		{ "Quest", TERM_XTRA_MUSIC_QUEST, NULL, get_quest_id },
		{ "Town", TERM_XTRA_MUSIC_TOWN, NULL, get_town_id },
		{ "Monster", TERM_XTRA_MUSIC_MONSTER, &has_monster_music,
			get_monster_id },
		{ NULL, 0, NULL, NULL } /* terminating sentinel */
	};
	std::filesystem::path pmusic, p;
	FILE *fff;
	NSMutableDictionary *catalog;

	/* Build the "music" path. */
	pmusic = path_build(ANGBAND_DIR_XTRA, "music");

	/* Find and open the config file. */
	p = path_build(pmusic, "music.cfg");
	fff = angband_fopen(p, FileOpenMode::READ);

	if (!fff) {
		NSLog(@"The music configuration file could not be opened");
		return nil;
	}

	catalog = [[NSMutableDictionary alloc] init];
	@autoreleasepool {
		const char white[] = " \t";
		NSMutableDictionary *catalogTypeRestricted = nil;
		int isec = -1;

		/* Parse the file. */
		while (1) {
			const auto line_str = angband_fgets(fff);
			NSMutableArray *samples;
			std::string id_name;
			std::string sample_names;
			size_t skip1, skip2, search;
			int id;

			if (!line_str) {
				break;
			}

			/* Skip leading whitespace. */
			skip1 = line_str->find_first_not_of(white);

			/*
			 * Ignore empty lines or ones that only have comments.
			 */
			if (skip1 == std::string::npos
					|| line_str->at(skip1) == '#') {
				continue;
			}

			if (line_str->at(skip1) == '[') {
				/*
				 * Found the start of a new section.  Will do
				 * nothing if the section name is malformed
				 * (i.e. missing the trailing bracket).
				 */
				search = line_str->find(']', skip1 + 1);
				if (search != std::string::npos) {
					std::string sec_name =
						line_str->substr(skip1 + 1,
						search - skip1 - 1);
					isec = 0;
					while (1) {
						if (!sections_of_interest[isec].name) {
							catalogTypeRestricted =
								nil;
							isec = -1;
							break;
						}
						if (!strcmp(sections_of_interest[isec].name, sec_name.data())) {
							NSNumber *key = [NSNumber
								numberWithInteger:sections_of_interest[isec].type_code];
							catalogTypeRestricted =
								[catalog objectForKey:key];
							if (!catalogTypeRestricted) {
								catalogTypeRestricted =
									[[NSMutableDictionary alloc] init];
								[catalog
									setObject:catalogTypeRestricted
									forKey:key];
							}
							break;
						}
						++isec;
					}
				}
				/* Skip the rest of the line. */
				continue;
			}

			/*
			 * Targets should begin with an alphabetical character.
			 * Skip anything else.
			 */
			if (!isalpha((unsigned char)(line_str->at(skip1)))) {
				continue;
			}

			search = line_str->find('=', skip1);
			if (search == std::string::npos) {
				continue;
			}
			skip2 = line_str->find_first_of(white, skip1);
			id_name = line_str->substr(skip1,
				((search <= skip2) ? search : skip2) - skip1);
			skip2 = line_str->find_first_not_of(white, search + 1);
			if (skip2 != std::string::npos) {
				sample_names = line_str->substr(skip2);
			}

			if (!catalogTypeRestricted
					|| (*(sections_of_interest[isec].name2id_func))(id_name.data(), &id)) {
				/*
				 * It is not in a section of interest or did
				 * not recognize what was on the left side of
				 * '='.  Ignore the line.
				 */
				continue;
			}
			if (sections_of_interest[isec].p_has) {
				*(sections_of_interest[isec].p_has) = true;
			}
			samples = [catalogTypeRestricted
				objectForKey:[NSNumber numberWithInteger:id]];
			if (!samples) {
				samples = [[NSMutableArray alloc] init];
				[catalogTypeRestricted
					setObject:samples
					forKey:[NSNumber numberWithInteger:id]];
			}

			/*
			 * Now find all the sample names and add them one by
			 * one.
			 */
			skip1 = 0;
			while (1) {
				std::string sample_name;
				BOOL done;
				struct stat stb;

				if (skip1 >= sample_names.length()) {
					break;
				}
				/* Terminate the current token. */
				skip2 = sample_names.find_first_of(white, skip1);
				if (skip2 == std::string::npos) {
					sample_name = sample_names.substr(skip1);
					done = TRUE;
				} else {
					sample_name = sample_names.substr(skip1,
						skip2 - skip1);
					done = FALSE;
				}

				/*
				 * Check if the path actually corresponds to a
				 * file.  Also restrict the number of samples
				 * stored for any type/ID combination.
				 */
				p = path_build(pmusic, sample_name.data());
				if (stat(p.native().data(), &stb) == 0
						&& (stb.st_mode & S_IFREG)
						&& (int)samples.count
						< [AngbandAudioManager maxSamples]) {
					[samples addObject:[NSString
						stringWithUTF8String:p.native().data()]];
				}

				if (done) {
					break;
				}
				skip1 = sample_names.find_first_not_of(white,
					skip2 + 1);
			}
		}
	}
	angband_fclose(fff);

	return catalog;
}

@end
