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

#include "ExpectationMaximization/MixtureModel.h"

/** Perform GrabCut segmentation on an image.  */
template <typename TImage>
class GrabCut
{
public:
    // Typedefs
    typedef typename TImage::PixelType PixelType;

    /** Constructor */
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

    /** Get the current/final segmentation mask. */
    ForegroundBackgroundSegmentMask* GetSegmentationMask();

    /** Get the resulting segmented image (the foreground pixels, with background pixels zeroed). */
    void GetSegmentedImage(TImage* result);

    /** Compute the likelihood that a pixel belongs to the foreground mixture model. */
    float ForegroundLikelihood(const typename TImage::PixelType& pixel);

    /** Compute the likelihood that a pixel belongs to the background mixture model. */
    float BackgroundLikelihood(const typename TImage::PixelType& pixel);

    /** Specify how many EM iterations to run during each GrabCut iteration. */
    void SetNumberOfEMIterations(const unsigned int numberOfEMIterations)
    {
        this->NumberOfEMIterations = numberOfEMIterations;
    }

protected:

    /** Create random models and add them to the mixture models.*/
    void InitializeModels(const unsigned int numberOfModels);

    /** Construct a data matrix from a selection of pixel indices. Every pixel is a column in the matrix. */
    Eigen::MatrixXd CreateMatrixFromPixels(const std::vector<typename TImage::PixelType>& pixels);

    /** Perform EM on a collection of pixels according to a mixture model. */
    MixtureModel ClusterPixels(const std::vector<typename TImage::PixelType>& pixels, const MixtureModel& mixtureModel);

    /** Compute the GMMs for both the foreground pixels and background pixels. */
    void ClusterForegroundAndBackground();

    /** Do one iteration of the GrabCut algorithm. */
    void PerformIteration();

    /** The segmentation mask. */
    ForegroundBackgroundSegmentMask::Pointer SegmentationMask;

    /** The input mask. */
    ForegroundBackgroundSegmentMask::Pointer InitialMask;

    /** The image to be segmented. */
    typename TImage::Pointer Image;

    /** The mixture model for the foreground. */
    MixtureModel ForegroundModels;

    /** The mixture model for the background. */
    MixtureModel BackgroundModels;

    /** The number of EM iterations to run for each GrabCut iteration. */
    unsigned int NumberOfEMIterations = 5;

    unsigned int GetDimensionality()
    {
        if(this->Image)
        {
            return TImage::PixelType::Dimension;
        }
        return 0;
    }
};

#include "GrabCut.hpp"

#endif
