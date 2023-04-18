/**
 * \file SoundAndMusc.h
 * \brief Declare interface to sound and music configuration panel used by
 * the OS X front end.
 */

#import <Cocoa/Cocoa.h>


/**
 * Declare a protocol to encapsulate changes to the settings for sounds and
 * music.
 */
@protocol SoundAndMusicChanges
- (void)changeSoundEnabled:(BOOL)newv;
- (void)changeSoundVolume:(NSInteger)newv;
- (void)changeMusicEnabled:(BOOL)newv;
- (void)changeMusicPausedWhenInactive:(BOOL)newv;
- (void)changeMusicVolume:(NSInteger)newv;
- (void)changeMusicTransitionTime:(NSInteger)newv;
- (void)soundAndMusicPanelWillClose;
@end


/**
 * Declare a NSWindowController subclass to load the panel from the nib file.
 */
@interface SoundAndMusicPanelController : NSWindowController <NSWindowDelegate>

/**
 * Hold whether or not incidental sounds (and beeps) are played.
 */
@property (nonatomic, getter=isSoundEnabled) BOOL soundEnabled;

/**
 * Hold the volume for incidental sounds as a percentage (1 to 100) of the
 * system volume.
 */
@property (nonatomic) NSInteger soundVolume;

/**
 * Hold whether or not background music is played.
 */
@property (nonatomic, getter=isMusicEnabled) BOOL musicEnabled;

/**
 * Hold whether or not currently playing music tracks are paused when the
 * containing application becomes inactive.
 */
@property (nonatomic, getter=isMusicPausedWhenInactive)
		BOOL musicPausedWhenInactive;

/**
 * Hold the volume for background music as a percentage (1 to 100) of the
 * system volume.
 */
@property (nonatomic) NSInteger musicVolume;

/**
 * Hold the transition time in milliseconds for when a background music track
 * is started when another background music track is already playing.  If
 * than or equal to zero, the current track is stopped and the new track
 * starts playing at full volume without any extra delay.
 */
@property (nonatomic) NSInteger musicTransitionTime;

/**
 * Is the delegate that responds when one of the settings changes.
 */
@property (weak, nonatomic) id<SoundAndMusicChanges> changeHandler;

/* These are implementation details. */
@property (strong) IBOutlet NSPanel *window;
@property (strong) IBOutlet NSButton *soundEnabledControl;
@property (strong) IBOutlet NSSlider *soundVolumeControl;
@property (strong) IBOutlet NSButton *musicEnabledControl;
@property (strong) IBOutlet NSButton *musicPausedWhenInactiveControl;
@property (strong) IBOutlet NSSlider *musicVolumeControl;
@property (strong) IBOutlet NSSlider *musicTransitionTimeControl;

@end
