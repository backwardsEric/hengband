/**
 * \file SoundAndMusic.m
 * \brief Define interface to sound and music configuration panel used by the
 * OS X front end.
 *
 * Copyright (c) 2023 Eric Branlund
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#import "SoundAndMusic.h"


@implementation SoundAndMusicPanelController

/*
 * Don't use the implicit @synthesize since this property is declared in a
 * superclass, NSWindowController.
 */
@dynamic window;


/*
 * Handle property methods where need more than is provided by the default
 * synthesis.
 */
- (BOOL)isSoundEnabled
{
	return ([self soundEnabledControl]
		&& [self soundEnabledControl].state != NSControlStateValueOff)
		? YES : NO;
}


- (void)setSoundEnabled:(BOOL)v
{
	if ([self soundEnabledControl]) {
		[self soundEnabledControl].state = (v) ?
			NSControlStateValueOn : NSControlStateValueOff;
	}
}


- (NSInteger)soundVolume
{
	return ([self soundVolumeControl]) ?
		[self soundVolumeControl].integerValue : 100;
}


- (void)setSoundVolume:(NSInteger)v
{
	if ([self soundVolumeControl]) {
		if (v < 1) {
			v = 1;
		} else if (v > 100) {
			v = 100;
		}
		[self soundVolumeControl].integerValue = v;
	}
}


- (BOOL)isMusicEnabled
{
	return ([self musicEnabledControl]
		&& [self musicEnabledControl].state != NSControlStateValueOff)
		? YES : NO;
}


- (void)setMusicEnabled:(BOOL)v
{
	if ([self musicEnabledControl]) {
		[self musicEnabledControl].state = (v) ?
			NSControlStateValueOn : NSControlStateValueOff;
	}
}


- (BOOL)isMusicPausedWhenInactive
{
	return ([self musicPausedWhenInactiveControl]
		&& [self musicPausedWhenInactiveControl].state
		!= NSControlStateValueOn) ? NO : YES;
}


- (void)setMusicPausedWhenInactive:(BOOL)v
{
	if ([self musicPausedWhenInactiveControl]) {
		[self musicPausedWhenInactiveControl].state =
			(v) ? NSControlStateValueOn : NSControlStateValueOff;
	}
}


- (NSInteger)musicVolume
{
	return ([self musicVolumeControl]) ?
		[self musicVolumeControl].integerValue : 100;
}


- (void)setMusicVolume:(NSInteger)v
{
	if ([self musicVolumeControl]) {
		if (v < 1) {
			v = 1;
		} else if (v > 100) {
			v = 100;
		}
		[self musicVolumeControl].integerValue = v;
	}
}


- (NSInteger)musicTransitionTime
{
	return ([self musicTransitionTimeControl]) ?
		[self musicTransitionTimeControl].integerValue : 0;
}


- (void)setMusicTransitionTime:(NSInteger)v
{
	if ([self musicTransitionTimeControl]) {
		if (v < 0) {
			v = 0;
		} else if (v > [self musicTransitionTimeControl].maxValue) {
			v = (NSInteger)([self musicTransitionTimeControl].maxValue);
		}
		[self musicTransitionTimeControl].integerValue = v;
	}
}


/**
 * Supply the NSWindowDelegate's windowWillClose method to relay the close
 * message through the SoundAndMusicChanges protocol.
 */
- (void)windowWillClose:(NSNotification *)notification
{
	if ([self changeHandler]) {
		[[self changeHandler] soundAndMusicPanelWillClose];
	}
}


/**
 * Override the NSWindowController property getter to get the appropriate nib
 * file.
 */
- (NSString *)windowNibName
{
	return @"SoundAndMusic";
}


/**
 * Override the NSWindowController function to set some attributes on the
 * window.
 */
- (void)windowDidLoad
{
	[super windowDidLoad];
	self.window.floatingPanel = YES;
	self.window.becomesKeyOnlyIfNeeded = YES;
	[self.window center];
	self.window.delegate = self;
}


- (IBAction)respondToSoundEnabledToggle:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeSoundEnabled:self.isSoundEnabled];
	}
}


- (IBAction)respondToSoundVolumeSlider:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeSoundVolume:self.soundVolume];
	}
}


- (IBAction)respondToMusicEnabledToggle:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeMusicEnabled:self.isMusicEnabled];
	}
}


- (IBAction)respondToMusicPausedWhenInactiveToggle:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeMusicPausedWhenInactive:self.isMusicPausedWhenInactive];
	}
}


- (IBAction)respondToMusicVolumeSlider:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeMusicVolume:self.musicVolume];
	}
}


- (IBAction)respondToMusicTransitionTimeSlider:(id)sender
{
	if ([self changeHandler]) {
		[[self changeHandler] changeMusicTransitionTime:self.musicTransitionTime];
	}
}

@end
