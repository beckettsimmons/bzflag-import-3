/* bzflag
 * Copyright 1993-1999, Chris Schoeneman
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named LICENSE that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * main game loop stuff
 */

#ifndef	BZF_PLAYING_H
#define	BZF_PLAYING_H

#include "common.h"
#include "global.h"
#include "AList.h"

class SceneRenderer;
class KeyMap;

struct StartupInfo {
  public:
			StartupInfo();

  public:
    boolean		hasConfiguration;
    boolean		autoConnect;
    char		serverName[80];
    int			serverPort;
    int			ttl;
    char		multicastInterface[65];
    TeamColor		team;
    char		callsign[CallSignLen];
    char		email[EmailLen];
};

typedef void		(*JoinGameCallback)(boolean success, void* data);
typedef void		(*PlayingCallback)(void*);
struct PlayingCallbackItem {
  public:
    PlayingCallback	cb;
    void*		data;
};
BZF_DEFINE_ALIST(PlayingCallbackList, PlayingCallbackItem);

class BzfDisplay;
class MainWindow;
class SceneRenderer;
class ResourceDatabase;
class PlayerId;
class Player;

BzfDisplay*		getDisplay();
MainWindow*		getMainWindow();
SceneRenderer*		getSceneRenderer();
void			setSceneDatabase();
StartupInfo*		getStartupInfo();
KeyMap&			getKeyMap();
void			notifyKeyMapChanged();
boolean			setVideoFormat(int, boolean test = False);
Player*			lookupPlayer(const PlayerId& id);
void			startPlaying(BzfDisplay* display,
				SceneRenderer&,
				ResourceDatabase&,
				StartupInfo*);

boolean			addExplosion(const float* pos,
				float size, float duration);
void			addTankExplosion(const float* pos);
void			addShotExplosion(const float* pos);

void			addPlayingCallback(PlayingCallback, void* data);
void			removePlayingCallback(PlayingCallback, void* data);

void			joinGame(JoinGameCallback, void* userData);

#endif // BZF_PLAYING_H
