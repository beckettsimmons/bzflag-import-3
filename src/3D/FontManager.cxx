/* bzflag
* Copyright (c) 1993 - 2005 Tim Riker
*
* This package is free software;  you can redistribute it and/or
* modify it under the terms of the license found in the file
* named COPYING that should have accompanied this file.
*
* THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <math.h>
#include <string>

#include "common.h"
#include "bzfgl.h"
#include "FontManager.h"
#include "TextureFont.h"
#include "bzfio.h"
#include "AnsiCodes.h"
#include "StateDatabase.h"
#include "BZDBCache.h"
#include "OpenGLGState.h"
#include "OpenGLTexture.h"
#include "TextureManager.h"
#include "TimeKeeper.h"
#include "TextUtils.h"

// initialize the singleton
template <>
FontManager* Singleton<FontManager>::_instance = (FontManager*)0;

/*
typedef std::map<int, TextureFont*> FontSizeMap;
typedef std::vector<FontSizeMap>  FontFaceList;
typedef std::map<std::string, int>  FontFaceMap;
*/

// ANSI code GLFloat equivalents - these should line up with the enums in AnsiCodes.h
static GLfloat BrightColors[8][3] = {
  {1.0f,1.0f,0.0f}, // yellow
  {1.0f,0.0f,0.0f}, // red
  {0.0f,1.0f,0.0f}, // green
  {0.1f,0.2f,1.0f}, // blue
  {1.0f,0.0f,1.0f}, // purple
  {1.0f,1.0f,1.0f}, // white
  {0.5f,0.5f,0.5f}, // grey
  {0.0f,1.0f,1.0f}  // cyan
};

GLfloat FontManager::underlineColor[3];
void FontManager::callback(const std::string &, void *)
{
  // set underline color
  const std::string uColor = BZDB.get("underlineColor");
  if (strcasecmp(uColor.c_str(), "text") == 0) {
    underlineColor[0] = -1.0f;
    underlineColor[1] = -1.0f;
    underlineColor[2] = -1.0f;
  } else if (strcasecmp(uColor.c_str(), "cyan") == 0) {
    underlineColor[0] = BrightColors[CyanColor][0];
    underlineColor[1] = BrightColors[CyanColor][1];
    underlineColor[2] = BrightColors[CyanColor][2];
  } else if (strcasecmp(uColor.c_str(), "grey") == 0) {
    underlineColor[0] = BrightColors[GreyColor][0];
    underlineColor[1] = BrightColors[GreyColor][1];
    underlineColor[2] = BrightColors[GreyColor][2];
  }
}

FontManager::FontManager() : Singleton<FontManager>(), dimFactor(0.7f)
{
  faceNames.clear();
  fontFaces.clear();
  BZDB.addCallback(std::string("underlineColor"), callback, NULL);
  BZDB.touch("underlineColor");
  OpenGLGState::registerContextInitializer(freeContext, initContext,
					   (void*)this);
}

FontManager::~FontManager()
{
  clear();
  OpenGLGState::unregisterContextInitializer(freeContext, initContext,
					     (void*)this);
  return;
}


void FontManager::freeContext(void* data)
{
  ((FontManager*)data)->clear();
  return;
}


void FontManager::initContext(void* data)
{
  ((FontManager*)data)->rebuild();
  return;
}


void FontManager::clear(void)	// clear all the lists
{
  // destroy all the fonts
  faceNames.clear();
  FontFaceList::iterator faceItr = fontFaces.begin();
  while (faceItr != fontFaces.end()) {
    FontSizeMap::iterator itr = faceItr->begin();
    while (itr != faceItr->end()) {
      delete(itr->second);
      itr++;
    }
    faceItr++;
  }
  fontFaces.clear();
  return;
}


void FontManager::rebuild(void)	// rebuild all the lists
{
  clear();
  loadAll(fontDirectory);
}


void FontManager::loadAll(std::string directory)
{
  if (directory.size() == 0)
    return;

  // save this in case we have to rebuild
  fontDirectory = directory;

  OSFile file;

  OSDir dir(directory);

  while (dir.getNextFile(file, true)) {
    std::string ext = file.getExtension();

    if (TextUtils::compare_nocase(ext, "fmt") == 0) {
      TextureFont *pFont = new TextureFont;
      if (pFont) {
        if (pFont->load(file)) {
          std::string  str = TextUtils::toupper(pFont->getFaceName());

          FontFaceMap::iterator faceItr = faceNames.find(str);

          int faceID = 0;
          if (faceItr == faceNames.end()) {
            // it's new
            FontSizeMap faceList;
            fontFaces.push_back(faceList);
            faceID = (int)fontFaces.size() - 1;
            faceNames[str] = faceID;
          } else {
            faceID = faceItr->second;
          }

          fontFaces[faceID][pFont->getSize()] = pFont;
        } else {
          DEBUG4("Font Texture load failed: %s\n", file.getOSName().c_str());
          delete(pFont);
        }
      }
    }
  }
}

int FontManager::getFaceID(std::string faceName)
{
  if (faceName.size() == 0)
    return -1;

  faceName = TextUtils::toupper(faceName);

  FontFaceMap::iterator faceItr = faceNames.find(faceName);

  int faceID = 0;
  if (faceItr == faceNames.end()) {
    // see if there is a default
    DEBUG4("Requested font %s not found, trying Default\n", faceName.c_str());
    faceName = "DEFAULT";
    faceItr = faceNames.find(faceName);
    if (faceItr == faceNames.end()) {
      // see if we have arial
      DEBUG4("Requested font %s not found, trying Arial\n", faceName.c_str());
      faceName = "ARIAL";
      faceItr = faceNames.find(faceName);
      if (faceItr == faceNames.end()) {
	// hell we are outta luck, you just get the first one
	DEBUG4("Requested font %s not found, trying first-loaded\n", faceName.c_str());
	faceItr = faceNames.begin();
	if (faceItr == faceNames.end()) {
	  DEBUG2("No fonts loaded\n");
	  return -1;	// we must have NO fonts, so you are screwed no matter what
	}
      }
    }
  }

  return faceID = faceItr->second;
}

int FontManager::getNumFaces(void)
{
  return (int)fontFaces.size();
}

const char* FontManager::getFaceName(int faceID)
{
  if ((faceID < 0) || (faceID > getNumFaces())) {
    DEBUG2("Trying to fetch name for invalid Font Face ID %d\n", faceID);
    return NULL;
  }

  return fontFaces[faceID].begin()->second->getFaceName();
}

void FontManager::drawString(float x, float y, float z, int faceID, float size,
			     const std::string &text)
{
  if (text.size() == 0)
    return;

  if ((faceID < 0) || (faceID > getNumFaces())) {
    DEBUG2("Trying to draw with invalid Font Face ID %d\n", faceID);
    return;
  }

  TextureFont* pFont = getClosestRealSize(faceID, size, size);

  if (!pFont) {
    DEBUG2("Could not find applicable font size for rendering; font face ID %d, "
	   "requested size %f\n", faceID, size);
    return;
  }

  float scale = size / (float)pFont->getSize();
  
  // texture filtering is off by default for fonts.
  // if the font is large enough, and the scaling factor
  // is not an integer, then turn on the max filtering.
  bool textureFiltering = false;
  if ((size > 12.0f) && (fabsf(scale - floorf(scale)) > 0.001f)) {
    textureFiltering = true;
    int texID = pFont->getTextureID();
    TextureManager& tm = TextureManager::instance();
    tm.setTextureFilter(texID, OpenGLTexture::Max);
  } else {
    // no filtering - clamp to aligned coordinates
    x = floorf(x);
    y = floorf(y);
    z = floorf(z);
  }
  

  /*
   * Colorize text based on ANSI codes embedded in it
   * Break the text every time an ANSI code
   * is encountered and do a separate pFont->drawString code for
   * each segment, with the appropriate color parameter
   */

  // sane defaults
  bool bright = true;
  bool pulsating = false;
  bool underline = false;
  // negatives are invalid, we use them to signal "no change"
  GLfloat color[3] = {-1.0f, -1.0f, -1.0f};


  /*
   * ANSI code interpretation is somewhat limited, we only accept values
   * which have been defined in AnsiCodes.h
   */

  bool doneLastSection = false;
  int startSend = 0;
  int endSend = (int)text.find("\033[", startSend);
  bool tookCareOfANSICode = false;
  float width = 0;
  // run at least once
  if (endSend == -1) {
    endSend = (int)text.size();
    doneLastSection = true;
  }
  // split string into parts based on the embedded ANSI codes, render each separately
  // there has got to be a faster way to do this
  while (endSend >= 0) {
    // pulsate the text, if desired
    if (pulsating)
      getPulseColor(color, color);
    // render text
    int len = endSend - startSend;
    if (len > 0) {
      const char* tmpText = text.c_str();
      // get substr width, we may need it a couple times
      width = pFont->getStrLength(scale, &tmpText[startSend], len);
      glPushMatrix();
      glTranslatef(x, y, z);
      GLboolean depthMask;
      glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
      glDepthMask(0);
      pFont->drawString(scale, color, &tmpText[startSend], len);
      if (underline) {
	OpenGLGState::resetState();  // FIXME - full reset required?
	if (underlineColor[0] >= 0)
	  glColor3fv(underlineColor);
	else if (color[0] >= 0)
	  glColor3fv(color);
	// still have a translated matrix, these coordinates are
	// with respect to the string just drawn
	glBegin(GL_LINES);
	glVertex2f(0, 0.0f);
	glVertex2f(width, 0.0f);
	glEnd();
      }
      glDepthMask(depthMask);
      glPopMatrix();
      // x transform for next substr
      x += width;
    }
    if (!doneLastSection) {
      startSend = (int)text.find("m", endSend) + 1;
    }
    // we stopped sending text at an ANSI code, find out what it is
    // and do something about it
    if (endSend != (int)text.size()) {
      tookCareOfANSICode = false;
      std::string tmpText = text.substr(endSend, (text.find("m", endSend) - endSend) + 1);
      // colors
      for (int i = 0; i < 8; i++) {
	if (tmpText == ColorStrings[i]) {
	  if (bright) {
	    color[0] = BrightColors[i][0];
	    color[1] = BrightColors[i][1];
	    color[2] = BrightColors[i][2];
	  } else {
	    color[0] = BrightColors[i][0] * dimFactor;
	    color[1] = BrightColors[i][1] * dimFactor;
	    color[2] = BrightColors[i][2] * dimFactor;
	  }
	  tookCareOfANSICode = true;
	  break;
	}
      }
      // didn't find a matching color
      if (!tookCareOfANSICode) {
	// settings other than color
	if (tmpText == ANSI_STR_RESET) {
	  bright = true;
	  pulsating = false;
	  underline = false;
	  color[0] = BrightColors[WhiteColor][0];
	  color[1] = BrightColors[WhiteColor][1];
	  color[2] = BrightColors[WhiteColor][2];
	} else if (tmpText == ANSI_STR_RESET_FINAL) {
	  bright = false;
	  pulsating = false;
	  underline = false;
	  color[0] = BrightColors[WhiteColor][0] * dimFactor;
	  color[1] = BrightColors[WhiteColor][1] * dimFactor;
	  color[2] = BrightColors[WhiteColor][2] * dimFactor;
	} else if (tmpText == ANSI_STR_BRIGHT) {
	  bright = true;
	} else if (tmpText == ANSI_STR_DIM) {
	  bright = false;
	} else if (tmpText == ANSI_STR_UNDERLINE) {
	  underline = !underline;
	} else if (tmpText == ANSI_STR_PULSATING) {
	  pulsating = !pulsating;
	} else {
	  DEBUG2("ANSI Code %s not supported\n", tmpText.c_str());
	}
      }
    }
    endSend = (int)text.find("\033[", startSend);
    if ((endSend == -1) && !doneLastSection) {
      endSend = (int)text.size();
      doneLastSection = true;
    }
  }

  // revert the texture filtering state  
  if (textureFiltering) {
    int texID = pFont->getTextureID();
    TextureManager& tm = TextureManager::instance();
    tm.setTextureFilter(texID, OpenGLTexture::Off);
  }

  return;  
}

void FontManager::drawString(float x, float y, float z,
			     const std::string &face, float size,
			     const std::string &text)
{
  drawString(x, y, z, getFaceID(face), size, text);
}

float FontManager::getStrLength(int faceID, float size,	const std::string &text,
				bool alreadyStripped)
{
  if (text.size() == 0)
    return 0.0f;

  if ((faceID < 0) || (faceID > getNumFaces())) {
    DEBUG2("Trying to find length of string for invalid Font Face ID %d\n", faceID);
    return 0.0f;
  }

  TextureFont* pFont = getClosestRealSize(faceID, size, size);

  if (!pFont) {
    DEBUG2("Could not find applicable font size for sizing; font face ID %d, "
	   "requested size %f\n", faceID, size);
    return 0.0f;
  }

  float scale = size / (float)pFont->getSize();

  // don't include ansi codes in the length, but allow outside funcs to skip this step
  const std::string &stripped = alreadyStripped ? text : stripAnsiCodes(text);

  return pFont->getStrLength(scale, stripped.c_str(), (int)stripped.size());
}

float FontManager::getStrLength(const std::string &face, float size,
				const std::string &text, bool alreadyStripped)
{
  return getStrLength(getFaceID(face), size, text, alreadyStripped);
}

float FontManager::getStrHeight(int faceID, float size,
				const std::string & /* text */)
{
  // don't scale tiny fonts
  getClosestRealSize(faceID, size, size);

  return (size * 1.5f);
}

float FontManager::getStrHeight(std::string face, float size,
				const std::string &text)
{
  return getStrHeight(getFaceID(face), size, text);
}

void FontManager::unloadAll(void)
{
  FontFaceList::iterator faceItr = fontFaces.begin();

  while (faceItr != fontFaces.end()) {
    FontSizeMap::iterator itr = faceItr->begin();
    while (itr != faceItr->end()) {
      itr->second->free();
      itr++;
    }
    faceItr++;
  }
}

TextureFont* FontManager::getClosestSize(int faceID, float size)
{
  if (fontFaces[faceID].size() == 0)
    return NULL;

  // only have 1 so this is easy
  if (fontFaces[faceID].size() == 1)
    return fontFaces[faceID].begin()->second;

  TextureFont*	pFont = NULL;

  // find the first one that is equal or bigger
  FontSizeMap::iterator itr = fontFaces[faceID].begin();

  FontSizeMap::iterator lastFace = fontFaces[faceID].end();
  while (itr != lastFace) {
    if (size <= itr->second->getSize()) {
      pFont = itr->second;
      itr = lastFace;
    } else {
      itr++;
    }
  }
  // if we don't have one that is larger then take the largest one we have and pray for good scaling
  if (!pFont)
    pFont = fontFaces[faceID].rbegin()->second;

  return pFont;
}

TextureFont*    FontManager::getClosestRealSize(int faceID, float desiredSize, float &actualSize)
{
  /*
   * tiny fonts scale poorly, this function will return the nearest unscaled size of a font
   * if the font is too tiny to scale, and a scaled size if it's big enough.
   */

  TextureFont* font = getClosestSize(faceID, desiredSize);
  if (desiredSize < 14.0f) {
    // get the next biggest font size from requested
    if (!font) {
      DEBUG2("Could not find applicable font size for sizing; font face ID %d, "
	     "requested size %f\n", faceID, desiredSize);
      return NULL;
    }
    actualSize = (float)font->getSize();
  } else {
    actualSize = desiredSize;
  }
  return font;
}

void	    FontManager::getPulseColor(const GLfloat *color, GLfloat *pulseColor) const
{
  float pulseTime = TimeKeeper::getCurrent().getSeconds();

  // depth is how dark it should get (1.0 is to black)
  float pulseDepth = BZDBCache::pulseDepth;
  // rate is how fast it should pulsate (smaller is faster)
  float pulseRate = BZDBCache::pulseRate;

  float pulseFactor = fmodf(pulseTime, pulseRate) - pulseRate /2.0f;
  pulseFactor = fabsf(pulseFactor) / (pulseRate/2.0f);
  pulseFactor = pulseDepth * pulseFactor + (1.0f - pulseDepth);

  pulseColor[0] = color[0] * pulseFactor;
  pulseColor[1] = color[1] * pulseFactor;
  pulseColor[2] = color[2] * pulseFactor;
}

// Local Variables: ***
// mode: C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
