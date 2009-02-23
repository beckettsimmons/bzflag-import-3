/* bzflag
 * Copyright (c) 1993 - 2006 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// ServerControl.cpp : Server shutdown and ban file control
//

#include <string>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "bzfsAPI.h"
#include "plugin_utils.h"
#include "plugin_config.h"

BZ_GET_PLUGIN_VERSION

using namespace std;

enum action
{ join, part };

class ServerControl:public bz_EventHandler
{
public:
  ServerControl() {};
  virtual ~ServerControl() {};
  virtual void process(bz_EventData * eventData);
  int loadConfig(const char *cmdLine);
private:
  void countPlayers(action act, bz_PlayerJoinPartEventData_V1 * data);
  void checkShutdown(void);
  void checkBanChanges(void);
  void checkMasterBanChanges(void);
  void fileAccessTime(const std::string filename, time_t * mtime);
  string banFilename;
  string masterBanFilename;
  string resetServerOnceFilename;
  string resetServerAlwaysFilename;
  string banReloadMessage;
  string masterBanReloadMessage;
  time_t banFileAccessTime;
  time_t masterBanFileAccessTime;
  int numPlayers;
  int numObservers;
  bool serverActive;
  bool ignoreObservers;
};

ServerControl serverControlHandler;

BZF_PLUGIN_CALL int bz_Load(const char *cmdLine)
{
  if (serverControlHandler.loadConfig(cmdLine) < 0)
    return -1;

  bz_registerEvent(bz_ePlayerJoinEvent, &serverControlHandler);
  bz_registerEvent(bz_ePlayerPartEvent, &serverControlHandler);
  bz_registerEvent(bz_eTickEvent, &serverControlHandler);
  bz_setMaxWaitTime(3.0, "SERVER_CONTROL");
  return 0;
}

BZF_PLUGIN_CALL int bz_Unload(void)
{
  bz_removeEvent(bz_ePlayerJoinEvent, &serverControlHandler);
  bz_removeEvent(bz_ePlayerPartEvent, &serverControlHandler);
  bz_removeEvent(bz_eTickEvent, &serverControlHandler);
  bz_clearMaxWaitTime("SERVER_CONTROL");
  return 0;
}

int ServerControl::loadConfig(const char *cmdLine)
{
  PluginConfig config = PluginConfig(cmdLine);
  string section = "ServerControl";

  if (config.errors)
    return -1;

  serverActive = false;
  countPlayers(join, NULL);

  /*
   * Set up options from the configuration file
   */
  banFilename = config.item(section, "BanFile");
  masterBanFilename = config.item(section, "MasterBanFile");
  resetServerOnceFilename = config.item(section, "ResetServerOnceFile");
  resetServerAlwaysFilename = config.item(section, "ResetServerAlwaysFile");
  banReloadMessage = config.item(section, "BanReloadMessage");
  masterBanReloadMessage = config.item(section, "MasterBanReloadMessage");
  ignoreObservers = (config.item(section, "IgnoreObservers") != "");

  /*
   * Report settings
   */
  // Ban file
  if (banFilename == "")
    bz_debugMessagef(1, "ServerControl - No banfile checks - no BanFile specified");
  else
    bz_debugMessagef(1, "ServerControl - Monitoring ban file: %s", banFilename.c_str());
  // Ban reload message
  if (banReloadMessage == "")
    bz_debugMessagef(1, "ServerControl - No BanReloadMessage notification");
  else
    bz_debugMessagef(1, "ServerControl - BanReloadMessage: %s", banReloadMessage.c_str());
  // Masterban file
  if (masterBanFilename == "")
    bz_debugMessagef(1, "ServerControl - No masterban file checks - no MasterbanFile specified");
  else
    bz_debugMessagef(1, "ServerControl - Monitoring master ban file: %s", masterBanFilename.c_str());
  // Master Ban reload message
  if (masterBanReloadMessage == "")
    bz_debugMessagef(1, "ServerControl - No MasterBanReloadMessage notification");
  else
    bz_debugMessagef(1, "ServerControl - MasterBanReloadMessage: %s", masterBanReloadMessage.c_str());
  // Reset Server Once file
  if (resetServerOnceFilename == "")
    bz_debugMessagef(1, "ServerControl - No ResetServerOnceFile specified");
  else
    bz_debugMessagef(1, "ServerControl - Using ResetServerOnceFile: %s", resetServerOnceFilename.c_str());
  // Reset Server Always file
  if (resetServerAlwaysFilename == "")
    bz_debugMessagef(1, "ServerControl - No ResetServerAlwaysFile specified");
  else
    bz_debugMessagef(1, "ServerControl - Using ResetServerAlwaysFile: %s", resetServerAlwaysFilename.c_str());

  // Ignore Observers
  if (ignoreObservers)
    bz_debugMessage(1, "ServerControl - Ignoring Observers for server restarts");
  else
    bz_debugMessage(1, "ServerControl - Server must be empty for server restarts");

  /* Set the initial ban file access times */
  if (masterBanFilename != "")
    fileAccessTime(masterBanFilename, &masterBanFileAccessTime);
  if (banFilename != "")
    fileAccessTime(banFilename, &banFileAccessTime);

  return 0;
}

void ServerControl::fileAccessTime(const std::string filename, time_t * mtime)
{
  struct stat buf;

  if (stat(filename.c_str(), &buf) == 0) {
    *mtime = buf.st_mtime;
  } else {
    *mtime = 0;
    bz_debugMessagef(0, "ServerControl - Can't stat the banfile %s", filename.c_str());
  }
}

void ServerControl::checkShutdown(void)
{
  // Check for server shutdown
  //
  // We shutdown the server in the following cases:
  //   The server has no players and
  //     the reset server once file exists OR
  //     the reset server always file exists and someone was on the server
  //
  if ((numPlayers <= 0) || (ignoreObservers && (numPlayers - numObservers) <= 0)) { // No players
    if (resetServerOnceFilename != "") {
      std::ifstream resetOnce(resetServerOnceFilename.c_str());
      if (resetOnce) {		// Reset server once exists
	resetOnce.close();
	remove( resetServerOnceFilename.c_str() );
	bz_debugMessagef(2, "ServerControl - Reset Server Once - SHUTDOWN");
	bz_shutdown();
      } else if (resetServerAlwaysFilename != "" && serverActive) {
	// Server was active - some non-observer player connected
	std::ifstream resetAlways(resetServerAlwaysFilename.c_str());
	if (resetAlways) {	// Reset server always exists
	  resetAlways.close();
	  bz_debugMessagef(2, "ServerControl - Reset Server Always - SHUTDOWN");
	  bz_shutdown();
	}
      }
    }
  }
}

void ServerControl::process(bz_EventData * eventData)
{
  ostringstream msg;
  bz_PlayerJoinPartEventData_V1 *data = (bz_PlayerJoinPartEventData_V1 *) eventData;

  if (eventData) {
    switch (eventData->eventType) {
    case bz_eTickEvent:
      checkShutdown();
      if (banFilename != "")
	checkBanChanges();
      if (masterBanFilename != "")
	checkMasterBanChanges();
      break;
    case bz_ePlayerJoinEvent:
      if (data->record->team >= eRogueTeam && data->record->team <= eHunterTeam && data->record->callsign != "") {
	serverActive = true;
      }
      countPlayers(join, data);
      break;
    case bz_ePlayerPartEvent:
      countPlayers(part, data);
      checkShutdown();
      break;
    default:
      break;
    }
  }
}

void ServerControl::countPlayers(action act, bz_PlayerJoinPartEventData_V1 * data)
{
  bz_APIIntList *playerList = bz_newIntList();
  bz_BasePlayerRecord *player;
  ostringstream msg;
  string str;
  int numLines = 0;
  int numObs = 0;

  bz_getPlayerIndexList(playerList);

  for (unsigned int i = 0; i < playerList->size(); i++) {
    player = bz_getPlayerByIndex(playerList->get(i));
    if (player) {
      if (act == join || (data && (player->playerID != data->playerID) &&
			  (player->callsign != ""))) {
	if (player->callsign != "") {
	  if (player->team == eObservers)
	    numObs++;
	  numLines++;
	}
      }
      bz_freePlayerRecord(player);
    }
  }
  numPlayers = numLines;
  numObservers = numObs;
  bz_debugMessagef(2, "serverControl - %d total players, %d observers", numPlayers, numObservers);
  bz_deleteIntList(playerList);
}

void ServerControl::checkBanChanges(void)
{
  time_t mtime;
  fileAccessTime(banFilename, &mtime);

  if (mtime != banFileAccessTime) {
    banFileAccessTime = mtime;
    bz_debugMessagef(1, "serverControl - ban file changed - reloading...");
    bz_reloadLocalBans();
    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, banReloadMessage.c_str());
  }
}

void ServerControl::checkMasterBanChanges(void)
{
  time_t mtime;
  fileAccessTime(masterBanFilename, &mtime);
  if (mtime != masterBanFileAccessTime) {
    masterBanFileAccessTime = mtime;
    bz_debugMessagef(1, "serverControl: master ban file changed - reloading...");
    bz_reloadMasterBans();
    bz_sendTextMessage(BZ_SERVER, BZ_ALLUSERS, masterBanReloadMessage.c_str());
  }
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
