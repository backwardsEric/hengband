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
	char sound_dir[1024], path[1024];
	FILE *fff;
	NSMutableDictionary *arraysByEvent;

	/* Build the "sound" path. */
	path_build(sound_dir, sizeof(sound_dir), ANGBAND_DIR_XTRA, "sound");

	/* Find and open the config file. */
	path_build(path, sizeof(path), sound_dir, "sound.cfg");
	fff = angband_fopen(path, "r");

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
		char buffer[2048];

		/* Parse the file. */
		/* Lines are always of the form "name = sample [sample ...]". */
		while (angband_fgets(fff, buffer, sizeof(buffer)) == 0) {
			NSMutableArray *soundSamples;
			char *msg_name;
			char *sample_name;
			char *search;
			int match;
			size_t skip;

			/* Skip leading whitespace. */
			skip = strspn(buffer, white);

			/*
			 * Ignore anything not beginning with an alphabetic
			 * character.
			 */
			if (!buffer[skip] || !isalpha((unsigned char)buffer[skip])) {
				continue;
			}

			/*
			 * Split the line into two; message name and the rest.
			 */
			search = strchr(buffer + skip, '=');
			if (!search) {
				continue;
			}
			msg_name = buffer + skip;
			skip = strcspn(msg_name, white);
			if (skip > (size_t)(search - msg_name)) {
				/*
				 *  No white space between the message name and
				 * '='.
				 */
				*search = '\0';
			} else {
				msg_name[skip] = '\0';
			}
			skip = strspn(search + 1, white);
			sample_name = search + 1 + skip;

			/* Make sure this is a valid event name. */
			for (match = SOUND_MAX - 1; match >= 0; --match) {
				if (!strcmp(msg_name, angband_sound_name[match])) {
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
			while (1) {
				int num;
				NSString *token_string;
				AVAudioPlayer *player;
				BOOL done;

				if (!sample_name[0]) {
					break;
				}
				/* Terminate the current token. */
				skip = strcspn(sample_name, white);
				done = !sample_name[skip];
				sample_name[skip] = '\0';

				/* Don't allow too many samples. */
				num = (int) soundSamples.count;
				if (num >= [AngbandAudioManager maxSamples]) {
					break;
				}

				token_string = [NSString
					stringWithUTF8String:sample_name];
				player = [playersByPath
					objectForKey:token_string];
				if (!player) {
					/*
					 * We have to load the sound.
					 * Build the path to the sample.
					 */
					struct stat stb;

					path_build(path, sizeof(path),
						sound_dir, sample_name);
					if (stat(path, &stb) == 0
							&& (stb.st_mode & S_IFREG)) {
						NSData *audioData = [NSData
							dataWithContentsOfFile:[NSString stringWithUTF8String:path]];

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
				sample_name += skip + 1;
				skip = strspn(sample_name, white);
				sample_name += skip;
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
	char music_dir[1024], path[1024];
	FILE *fff;
	NSMutableDictionary *catalog;

	/* Build the "sound" path. */
	path_build(music_dir, sizeof(music_dir), ANGBAND_DIR_XTRA, "music");

	/* Find and open the config file. */
	path_build(path, sizeof(path), music_dir, "music.cfg");
	fff = angband_fopen(path, "r");

	if (!fff) {
		NSLog(@"The music configuration file could not be opened");
		return nil;
	}

	catalog = [[NSMutableDictionary alloc] init];
	@autoreleasepool {
		const char white[] = " \t";
		NSMutableDictionary *catalogTypeRestricted = nil;
		int isec = -1;
		char buffer[2048];

		/* Parse the file. */
		while (angband_fgets(fff, buffer, sizeof(buffer)) == 0) {
			NSMutableArray *samples;
			char *id_name;
			char *sample_name;
			char *search;
			size_t skip;
			int id;

			/* Skip leading whitespace. */
			skip = strspn(buffer, white);

			/*
			 * Ignore empty lines or ones that only have comments.
			 */
			if (!buffer[skip] || buffer[skip] == '#') {
				continue;
			}

			if (buffer[skip] == '[') {
				/*
				 * Found the start of a new section.  Will do
				 * nothing if the section name is malformed
				 * (i.e. missing the trailing bracket).
				 */
				search = strchr(buffer + skip + 1, ']');
				if (search) {
					isec = 0;

					*search = '\0';
					while (1) {
						if (!sections_of_interest[isec].name) {
							catalogTypeRestricted =
								nil;
							isec = -1;
							break;
						}
						if (!strcmp(sections_of_interest[isec].name, buffer + skip + 1)) {
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
			if (!isalpha((unsigned char)buffer[skip])) {
				continue;
			}

			search = strchr(buffer + skip, '=');
			if (!search) {
				continue;
			}
			id_name = buffer + skip;
			skip = strcspn(id_name, white);
			if (skip > (size_t)(search - id_name)) {
				/*
				 * No white space between the name on the left
				 * and '='.
				 */
				*search = '\0';
			} else {
				id_name[skip] = '\0';
			}
			skip = strspn(search + 1, white);
			sample_name = search + 1 + skip;

			if (!catalogTypeRestricted
					|| (*(sections_of_interest[isec].name2id_func))(id_name, &id)) {
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
			while (1) {
				BOOL done;
				struct stat stb;

				if (!sample_name[0]) {
					break;
				}
				/* Terminate the current token. */
				skip = strcspn(sample_name, white);
				done = !sample_name[skip];
				sample_name[skip] = '\0';

				/*
				 * Check if the path actually corresponds to a
				 * file.  Also restrict the number of samples
				 * stored for any type/ID combination.
				 */
				path_build(path, sizeof(path), music_dir,
					sample_name);
				if (stat(path, &stb) == 0
						&& (stb.st_mode & S_IFREG)
						&& (int)samples.count
						< [AngbandAudioManager maxSamples]) {
					[samples addObject:[NSString
						stringWithUTF8String:path]];
				}

				if (done) {
					break;
				}
				sample_name += skip + 1;
				skip = strspn(sample_name, white);
				sample_name += skip;
			}
		}
	}
	angband_fclose(fff);

	return catalog;
}

@end
