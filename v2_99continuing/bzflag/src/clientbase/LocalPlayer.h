/* bzflag
 * Copyright (c) 1993-2010 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __LOCALPLAYER_H__
#define __LOCALPLAYER_H__

// interface header
#include "BaseLocalPlayer.h"

// system headers
#include <string>
#include <vector>

// common headers
#include "Obstacle.h"
#include "BzTime.h"
#include "vectors.h"

// local headers
#include "Player.h"
#include "ServerLink.h"

class MeshFace;
class FlagSceneNode;

class LocalPlayer : public BaseLocalPlayer {
  public:
    enum FiringStatus {
      Deceased,   // can't shoot cos I'm dead
      Ready,    // ready to shoot
      Loading,    // reloading
      Sealed,   // I'm inside a building
      Zoned   // I'm zoned
    };
    enum Location {
      Dead,   // dead, explosion over
      Exploding,    // dead and exploding
      OnGround,   // playing on ground
      InBuilding,   // playing in building
      OnBuilding,   // playing on building
      InAir   // playing in air
    };
    enum InputMethod {  // what device am I using to move around
      Keyboard = 0,
      Mouse,
      Joystick,
      InputMethodCount
    };

  public:
    static LocalPlayer* getMyTank();
    static void   setMyTank(LocalPlayer*);
    static std::string  getLocationString(Location);

  public:
    LocalPlayer(const PlayerId&,
                const char* name,
                const PlayerType _type = TankPlayer);
    ~LocalPlayer();

    void    setLocation(Location loc) { location = loc; }
    Location      getLocation() const;
    FiringStatus  getFiringStatus() const;
    float         getFlagShakingTime() const;
    int           getFlagShakingWins() const;
    const fvec3*  getAntidoteLocation() const;
    const Player* getTarget() const;
    int           getDeathPhysicsDriver() const;
    const std::vector<const Obstacle*>& getInsideBuildings() const;

    void setTeam(TeamColor);
    void changeTeam(TeamColor newTeam);
    void setDesiredSpeed(float fracOfMaxSpeed);
    void setDesiredAngVel(float fracOfMaxAngVel);
    void setPause(bool = true); // virtual
    void requestAutoPilot(bool = true);
    ShotPath* fireShot();
    void explodeTank();
    bool canJump() const;
    void doJump();
    void setJump();
    void setJumpPressed(bool value);
    void setTarget(const Player*);

    void setDeadStop(void);

    void    setNemesis(const Player*);
    const Player* getNemesis() const;

    void    setRecipient(const Player*);
    const Player* getRecipient() const;

    virtual void restart(const fvec3& pos, float azimuth);
    void setFlagID(int id);
    bool checkHit(const Player* source, const ShotPath*& hit, float& minTime) const;
    void changeScore(float newRank, short newWins, short newLosses, short newTeamKills);

    void addAntidote(SceneDatabase*);

    InputMethod getInputMethod() const;
    void    setInputMethod(InputMethod newInput);
    void    setInputMethod(std::string newInput);
    bool    queryInputChange();
    static std::string  getInputMethodName(InputMethod whatInput);

    void setKey(int button, bool pressed);
    int  getRotation();
    int  getSpeed();
    bool isSpawning();
    void setSpawning(bool spawn);
    bool hasHitWall();

    const Obstacle* getHitBuilding(const fvec3& oldPos, float oldAngle,
                                   const fvec3& newPos, float newAngle,
                                   bool phased, bool& expel);
    bool getHitNormal(const Obstacle* o,
                      const fvec3& pos1, float azimuth1,
                      const fvec3& pos2, float azimuth2,
                      fvec3& normal) const;

    inline bool onSolidSurface() {
      return (location == OnGround) || (location == OnBuilding);
    }

    bool    requestedAutopilot;

  protected:
    bool doEndShot(int index, bool isHit, fvec3& pos);
    void doUpdate(float dt);
    void doUpdateMotion(float dt);
    void doMomentum(float dt, float& speed, float& angVel);
    void doFriction(float dt, const fvec3& oldVelocity, fvec3& newVelocity);
    void doForces(float dt, fvec3& velocity, float& angVel);
    bool gettingSound;

    ServerLink* server;

  private:
    void    doSlideMotion(float dt, float slideTime,
                          float newAngVel, fvec3& newVelocity);
    bool    tryTeleporting(const MeshFace* linkSrc,
                           const fvec3& oldPos,    fvec3& newPos,
                           const fvec3& oldVel,    fvec3& newVel,
                           const float  oldAngle,  float& newAngle,
                           const float  oldAngVel, float& newAngVel,
                           bool phased, bool& expel);
    float   getNewAngVel(float old, float desired, float dt);
    void    collectInsideBuildings();

  private:
    Location  location;
    FiringStatus  firingStatus;
    BzTime  bounceTime;
    BzTime  agilityTime;
    float   flagShakingTime;
    int   flagShakingWins;
    fvec3   flagAntidotePos;
    FlagSceneNode*  antidoteFlag;
    float   desiredSpeed;
    float   desiredAngVel;
    float   lastSpeed;
    fvec4   crossingPlane;
    bool    anyShotActive;
    const Player* target;
    const Player* nemesis;
    const Player* recipient;
    static LocalPlayer* mainPlayer;
    InputMethod inputMethod;
    bool    inputChanged;
    bool    spawning;
    int   wingsFlapCount;
    bool    left;
    bool    right;
    bool    up;
    bool    down;
    bool    entryDrop; // first drop since entering
    bool    wantJump;
    bool    jumpPressed;
    int   deathPhyDrv;  // physics driver that caused death
    std::vector<const Obstacle*> insideBuildings;
    BzTime  stuckStartTime;
    BzTime  lastCollisionTime;
    bool          hitWall; // If doUpdateMotion hit a wall, this is true.
};


inline LocalPlayer::Location LocalPlayer::getLocation() const {
  return location;
}

inline LocalPlayer::FiringStatus LocalPlayer::getFiringStatus() const {
  return firingStatus;
}

inline const Player* LocalPlayer::getTarget() const {
  return target;
}

inline const Player* LocalPlayer::getNemesis() const {
  return nemesis;
}

inline const Player* LocalPlayer::getRecipient() const {
  return recipient;
}

inline int    LocalPlayer::getDeathPhysicsDriver() const {
  return deathPhyDrv;
}

inline const std::vector<const Obstacle*>& LocalPlayer::getInsideBuildings() const {
  return insideBuildings;
}

inline LocalPlayer::InputMethod LocalPlayer::getInputMethod() const {
  if (isObserver()) {
    return Keyboard;
  }
  return inputMethod;
}

inline void LocalPlayer::setInputMethod(InputMethod newInput) {
  inputMethod = newInput;
  inputChanged = true;
}

inline void LocalPlayer::setInputMethod(std::string newInput) {
  for (int i = 0; i < InputMethodCount; i++) {
    if (newInput == getInputMethodName((InputMethod)i)) {
      setInputMethod((InputMethod)i);
    }
  }
}

inline bool LocalPlayer::queryInputChange() {
  const bool returnVal = inputChanged;
  inputChanged = false;
  return returnVal;
}

inline bool LocalPlayer::isSpawning() {
  return spawning;
}

inline void LocalPlayer::setSpawning(bool spawn) {
  spawning = spawn;
}

inline bool LocalPlayer::hasHitWall() {
  return hitWall;
}

inline int LocalPlayer::getRotation() {
  if (left && !right) {
    return 1;
  }
  else if (right && !left) {
    return -1;
  }
  else {
    return 0;
  }
}

inline int LocalPlayer::getSpeed() {
  if (up && !down) {
    return 1;
  }
  else if (down && !up) {
    return -1;
  }
  else {
    return 0;
  }
}

inline LocalPlayer* LocalPlayer::getMyTank() {
  return mainPlayer;
}

#endif /* __LOCALPLAYER_H__ */

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=2 tabstop=8 expandtab expandtab
