/* bzflag
 * Copyright (c) 1993 - 2004 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* TetraBuilding:
 *	Encapsulates a tetrahederon in the game environment.
 */

#ifndef	BZF_TETRA_BUILDING_H
#define	BZF_TETRA_BUILDING_H

#include "common.h"
#include <string>
#include "Obstacle.h"

class TetraBuilding : public Obstacle {
  public:
			TetraBuilding(const float (*vertices)[3], const bool *visible,
			              const bool *colored, const float (*colors)[4],
			              bool drive = false, bool shoot = false);
			~TetraBuilding();

    const char*		getType() const;
    static const char*	getClassName(); // const
    bool                isValid() const;

    float		intersect(const Ray&) const;
    void		getNormal(const float* p, float* n) const;
    void		get3DNormal(const float* p, float* n) const;

    bool                inCylinder(const float* p, float radius, float height) const;
    bool                inBox(const float* p, float angle,
                              float halfWidth, float halfBreadth, float height) const;
    bool                inMovingBox(const float* oldP, float oldAngle,
                                    const float *newP, float newAngle,
                                    float halfWidth, float halfBreadth, float height) const;
    bool                isCrossing(const float* p, float angle,
                                   float halfWidth, float halfBreadth, float height,
                                   float* plane) const;
                                   
    bool		getHitNormal(
				const float* pos1, float azimuth1,
				const float* pos2, float azimuth2,
				float halfWidth, float halfBreadth,
				float height,
				float* normal) const;

    void		getCorner(int index, float* pos) const;
    
    const float*   getPlane(int plane) const;
    const float*   getVertex(int vertex) const;
    const float  (*getPlanes() const)[4];
    const float  (*getVertices() const)[3];
    bool           isVisiblePlane(int plane) const;
    bool           isColoredPlane(int plane) const;
    const float*   getPlaneColor(int plane) const;

    std::string	        userTextures[1];

  private:
    static const char*	typeName;
    float vertices[4][3];
    float planes[4][4];   // planes are numbered to the opposite vertex
    float mins[3];        // minimum extents
    float maxs[3];        // maximum extents
    bool visible[4];      // is this plane visible?
    bool colored[4];      // is this plane colored?
    float colors[4][4];   // RGBA color specifications per plane
};  


inline const float *TetraBuilding::getPlane(int plane) const
{
  return planes[plane];
}

inline const float *TetraBuilding::getVertex(int vertex) const
{
  return vertices[vertex];
}

inline bool TetraBuilding::isVisiblePlane(int plane) const
{
  return visible[plane];
}

inline bool TetraBuilding::isColoredPlane(int plane) const
{
  return colored[plane];
}

inline const float *TetraBuilding::getPlaneColor(int plane) const
{
  return colors[plane];
}

inline const float (*TetraBuilding::getPlanes() const)[4]
{
  return planes;
}

inline const float (*TetraBuilding::getVertices() const)[3]
{
  return vertices;
}


#endif // BZF_TETRA_BUILDING_H

// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8

