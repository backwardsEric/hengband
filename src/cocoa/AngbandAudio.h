/**
 * \file AngbandAudio.h
 * \brief Declare an interface for handling incidental sounds and background
 * music in the OS X front end.
 */
#ifndef INCLUDED_ANGBAND_AUDIO_H
#define INCLUDED_ANGBAND_AUDIO_H

#import <AVFAudio/AVFAudio.h>

@class AngbandAudioManager;


@interface AngbandActiveAudio : NSObject <AVAudioPlayerDelegate> {
@private
	AVAudioPlayer *player;
	NSTimer *fadeTimer;
}

@property (nonatomic, readonly, getter=isPlaying) BOOL playing;

@property (nonatomic, readonly, weak) AngbandActiveAudio *priorAudio;

@property (nonatomic, readonly, strong) AngbandActiveAudio *nextAudio;

- (id)initWithPlayer:(AVAudioPlayer *)aPlayer fadeInBy:(NSInteger)fadeIn
		prior:(AngbandActiveAudio *)p paused:(BOOL)isPaused
		NS_DESIGNATED_INITIALIZER;

- (id)init NS_UNAVAILABLE;

- (void)pause;

- (void)resume;

- (void)stop;

- (void)fadeOutBy:(NSInteger)t;

- (void)changeVolumeTo:(NSInteger)v;

/* Methods for internal use to manage the lifetime of the active audio track. */
- (void)handleFadeOutTimer:(NSTimer *)timer;

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)aPlayer
		successfully:(BOOL)flag;

@end


@interface AngbandAudioManager : NSObject {
@private
	/**
	 * These are sentinels for the beginning and end, respectively, of
	 * the background muisc tracks currently playing in the order they
	 * were started.
	 */
	AngbandActiveAudio *tracksPlayingHead;
	AngbandActiveAudio *tracksPlayingTail;

	/**
	 * Stores instances of AVAudioPlayer keyed by path so the same
	 * incidental sound can be used for multiple events.
	 */
	NSMutableDictionary *soundsByPath;

	/**
	 * Stores arrays of AVAudioPlayer instances keyed by event number.
	 */
	NSMutableDictionary *soundArraysByEvent;

	/**
	 * Stores paths to music tracks keyed first by the music type and
	 * then the music id.
	 */
	NSMutableDictionary *musicByTypeAndID;

	/**
	 * If NO, then do not play beeps or incidental sounds and pause
	 * the background music track if isPausedWhenInactive is YES.
	 */
	BOOL appActive;
}

/**
 * Holds whether a beep will be played.  If NO, then playBeep effectively
 * becomes a do nothing operation.
 */
@property (nonatomic, getter=isBeepEnabled) BOOL beepEnabled;

/**
 * Holds whether incidental sounds will be played.  If NO, then playSound
 * effectively becomes a do nothing operation.
 */
@property (nonatomic, getter=isSoundEnabled) BOOL soundEnabled;

/**
 * Holds the volume (as a percentage, 0 - 100, of the system volume) for
 * incidental sounds.
 */
@property (nonatomic) NSInteger soundVolume;

/**
 * Holds whether background music will be played.  If NO, then playMusicType
 * effectively becomes a do nothing operation.
 */
@property (nonatomic, getter=isMusicEnabled) BOOL musicEnabled;

/**
 * Holds whether music is paused when the application is iconified or otherwise
 * marked as being an inactive application.  Beeps and incidental sounds are
 * never played when the application is an inactive application.
 */
@property (nonatomic, getter=isMusicPausedWhenInactive)
		BOOL musicPausedWhenInactive;

/**
 * Holds the volume (as a percentage, 0 - 100, of the system volume) for
 * background music.
 */
@property (nonatomic) NSInteger musicVolume;

/**
 * Holds the amount of time, in milliseconds, for transitions between
 * different music tracks.  If less than or equal to zero, there's no
 * transition interval:  one track stops playing and the other starts playing
 * at full volume.
 */
@property (nonatomic) NSInteger musicTransitionTime;

/**
 * Set up for lazy initialization in playSound() and playMusicType().  Set
 * appActive to YES, beepEnabled to YES, soundEnabled to YES, soundVolume
 * to 30, musicEnabled to YES, pausedWhenInactive to YES,
 * musicVolume to 20, and musicTransitionTime to 3000.
 */
- (id)init;

/**
 * If self.beepEnabed is YES, make a beep.
 */
- (void)playBeep;

/**
 * If self.soundEnabled is YES and the given event has one or more sounds
 * corresponding to it in the catalog, plays one of those sounds, chosen at
 * random.
 */
- (void)playSound:(int)event;

/**
 * If self.musicEnabled is YES and the given music type and id exists, play
 * it.
 */
- (void)playMusicType:(int)t ID:(int)i;

/**
 * Return YES if combination of the given type and ID is in the music
 * catalog.  Otherwise, return NO.
 */
- (BOOL)musicExists:(int)t ID:(int)i;

/**
 * Stop all currently playing music tracks.
 */
- (void)stopAllMusic;

/**
 * Set up to act appropiately if the containing application is inactive.
 */
- (void)setupForInactiveApp;

/**
 * Set up to act appropriately if the containing application is active.
 */
- (void)setupForActiveApp;

/**
 * Impose an arbitrary limit on the number of possible samples for an
 * incidental sound event or the background music tracks for a type/ID
 * combination.
 */
@property (class, nonatomic, readonly) NSInteger maxSamples;

/**
 * Return the shared audio manager instance, creating it if it does not
 * exist yet.
 */
@property (class, nonatomic, strong, readonly) AngbandAudioManager
		*sharedManager;

/**
 * Release any resources associated with the shared audio manager.
 */
+ (void)clearSharedManager;

/**
 * Help playSound() to set up a catalog of the incidental sounds.
 */
+ (NSMutableDictionary *)setupSoundArraysByEvent;

/**
 * Help playMusicType() to set up a catalog of the background music.
 */
+ (NSMutableDictionary *)setupMusicByTypeAndID;

@end


#endif /* include guard */
