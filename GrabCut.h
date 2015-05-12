/*
Copyright (C) 2015 David Doria, daviddoria@gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GrabCut_H
#define GrabCut_H

// Submodules
#include "Mask/ForegroundBackgroundSegmentMask.h"
#include "ImageGraphCutSegmentation/ImageGraphCut.h"

// ITK
#include "itkImage.h"

// STL
#include <vector>

// Eigen
#include <Eigen/Dense>

/** Perform GrabCut segmentation on an image.  */
template <typename TImage>
class GrabCut
{
public:

    GrabCut();

    /** The type of a list of pixels/indexes. */
    typedef std::vector<itk::Index<2> > IndexContainer;

    /** Provide the image to segment. */
    void SetImage(TImage* const image);

    /** Provide the image to segment. */
    void SetInitialMask(ForegroundBackgroundSegmentMask* const mask);

    /** Get the image that we are segmenting. */
    TImage* GetImage();

    /** Do the GrabCut segmentation (The main driver function). */
    void PerformSegmentation();

    /** Do one iteration of the GrabCut algorithm. */
    void PerformIteration();

    /** Get the current/final segmentation mask. */
    ForegroundBackgroundSegmentMask* GetSegmentationMask();

    /** Get the resulting segmented image (the foreground pixels, with background pixels zeroed). */
    TImage* GetSegmentedImage();

protected:

    // Typedefs
    typedef typename TImage::PixelType PixelType;

    // Functions
    Eigen::MatrixXd CreateMatrixFromPixels(const std::vector<itk::Index<2> >& pixels);
    void ClusterPixels(const std::vector<itk::Index<2> >& pixels);
    void InitialClustering();

    // Data
    /** The object that will do the segmentation at each iteration. */
    ImageGraphCut<TImage> GraphCut;

    /** The segmentation mask. */
    ForegroundBackgroundSegmentMask::Pointer SegmentationMask;

    /** The input mask. */
    ForegroundBackgroundSegmentMask::Pointer InitialMask;

    /** The image to be segmented. */
    typename TImage::Pointer Image;

};

#include "GrabCut.hpp"

#endif
