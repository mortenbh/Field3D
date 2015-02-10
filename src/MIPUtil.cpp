//----------------------------------------------------------------------------//

/*
 * Copyright (c) 2009 Sony Pictures Imageworks Inc
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.  Neither the name of Sony Pictures Imageworks nor the
 * names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//----------------------------------------------------------------------------//

/*! \file MIPUtil.cpp
  Contains implementations of resampling-related functions.
*/

//----------------------------------------------------------------------------//

// Header include
#include "MIPUtil.h"

// System includes
#include <cmath>

// Library includes
#include <boost/foreach.hpp>

// Project includes
#include "CoordSys.h"

//----------------------------------------------------------------------------//

FIELD3D_NAMESPACE_OPEN

//----------------------------------------------------------------------------//

namespace detail {

  //--------------------------------------------------------------------------//

  V3i mipResolution(const V3i &baseRes, const size_t level)
  {
    const float factor = 1.0 / (1 << level);
    const V3f   floatRes(baseRes);
    return V3i(static_cast<int>(std::ceil(floatRes.x * factor)),
               static_cast<int>(std::ceil(floatRes.y * factor)),
               static_cast<int>(std::ceil(floatRes.z * factor)));
  }

  //--------------------------------------------------------------------------//

  FieldMapping::Ptr adjustedMIPFieldMapping(const FieldMapping::Ptr mapping, 
                                            const V3i &/*baseRes*/,
                                            const Box3i &extents, 
                                            const size_t level)
  {
    typedef MatrixFieldMapping::MatrixCurve MatrixCurve;

    const float mult = 1 << level;
    const V3i   res  = extents.size() + V3i(1);
      
    if (MatrixFieldMapping::Ptr mfm = 
        field_dynamic_cast<MatrixFieldMapping>(mapping)) {
      // Local space positions
      const V3d lsOrigin(0.0), lsX(1.0, 0.0, 0.0), lsY(0.0, 1.0, 0.0), 
        lsZ(0.0, 0.0, 0.1);
      // Find base voxel size
      const V3f wsBaseVoxelSize = mfm->wsVoxelSize(0, 0, 0);
      // Compute current levels' voxel size
      const V3f wsVoxelSize = wsBaseVoxelSize * mult;
      // Grab the matrices
      const MatrixCurve::SampleVec lsToWsSamples = mfm->localToWorldSamples();
      // New mapping to construct
      MatrixFieldMapping::Ptr newMapping(new MatrixFieldMapping);
      // For each time sample
      BOOST_FOREACH (const MatrixCurve::Sample &sample, lsToWsSamples){
        // Find origin and orientation vectors
        V3d wsOrigin, wsX, wsY, wsZ;
        mfm->localToWorld(lsOrigin, wsOrigin, sample.first);
        mfm->localToWorld(lsX, wsX, sample.first);
        mfm->localToWorld(lsY, wsY, sample.first);
        mfm->localToWorld(lsZ, wsZ, sample.first);
        // Normalize orientation vectors
        wsX = (wsX - wsOrigin).normalized();
        wsY = (wsY - wsOrigin).normalized();
        wsZ = (wsZ - wsOrigin).normalized();
        // Mult by voxel size
        wsX *= wsVoxelSize.x * res.x;
        wsY *= wsVoxelSize.y * res.y;
        wsZ *= wsVoxelSize.z * res.z;
        // Construct new mapping
        M44d mtx = coordinateSystem(wsX, wsY, wsZ, wsOrigin);
        // Update mapping
        newMapping->setLocalToWorld(sample.first, mtx);
      }
      // Done
      return newMapping;
    } else {
      // For non-uniform grids, there is nothing we can do. 
      return mapping;
    }
  }

  //--------------------------------------------------------------------------//

} // namespace detail

//----------------------------------------------------------------------------//

FIELD3D_NAMESPACE_SOURCE_CLOSE

//----------------------------------------------------------------------------//
