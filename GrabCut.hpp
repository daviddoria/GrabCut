/*
Copyright (C) 2012 David Doria, daviddoria@gmail.com

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

#ifndef GrabCut_HPP
#define GrabCut_HPP

#include "GrabCut.h"

// Submodules
#include "Mask/ITKHelpers/Helpers/Helpers.h"
#include "Mask/ITKHelpers/ITKHelpers.h"

// ITK
#include "itkImageRegionIterator.h"
#include "itkShapedNeighborhoodIterator.h"
#include "itkMaskImageFilter.h"

// STL
#include <cmath>

// Boost
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>

template <typename TImage>
void GrabCut<TImage>::SetImage(TImage* const image)
{
  this->Image = TImage::New();
  ITKHelpers::DeepCopy(image, this->Image.GetPointer());
}

template <typename TImage>
void ImageGraphCut<TImage>::Initialize()
{
    std::vector<itk::Index<2> > foregroundPixels =
        ITKHelpers::GetPixelsWithValue(this->InitialMask.GetPointer(), ForegroundBackgroundSegmentMaskPixelTypeEnum::FOREGROUND);

    Eigen::MatrixXd foregroundData = CreateMatrixFromPixels(foregroundPixels);

    std::vector<itk::Index<2> > backgroundPixels =
        ITKHelpers::GetPixelsWithValue(this->InitialMask.GetPointer(), ForegroundBackgroundSegmentMaskPixelTypeEnum::BACKGROUND);

    Eigen::MatrixXd backgroundData = CreateMatrixFromPixels(backgroundPixels);
}

template <typename TImage>
Eigen::MatrixXd ImageGraphCut<TImage>::CreateMatrixFromPixels(const std::vector<itk::Index<2> >& pixels)
{
    // Create the data matrix from the foreground and background pixels
    Eigen::MatrixXd data(3, pixels.size());

    for(unsigned int i = 0; i < pixels.size(); i++)
    {
      Eigen::VectorXd v(3);
      PixelType p = this->Image->GetPixel(pixels[i]);
      for(unsigned int d = 0; d < 3; ++d)
      {
        v(d) = p[d];
      }

      data.col(i) = v;
    }

    return data;
}

template <typename TImage>
void ImageGraphCut<TImage>::PerformSegmentation()
{



  // Blank the output image
  ITKHelpers::SetImageToConstant(this->ResultingSegments.GetPointer(),
                                 ForegroundBackgroundSegmentMaskPixelTypeEnum::BACKGROUND);

}

template <typename TImage>
void ImageGraphCut<TImage>::PerformIteration()
{
    int dimensionality = 1;

    // Generate some data
    Eigen::MatrixXd data = GenerateData(1000, dimensionality);

    // Initialize the model
    std::vector<Model*> models(2);

    for(unsigned int i = 0; i < models.size(); i++)
    {
      Model* model = new GaussianModel(dimensionality);
      models[i] = model;
    }

    ExpectationMaximization expectationMaximization;
    expectationMaximization.SetData(data);
    expectationMaximization.SetModels(models);
    expectationMaximization.SetMinChange(1e-5);
    expectationMaximization.SetMaxIterations(100);

    expectationMaximization.Compute();

    std::cout << "Final models:" << std::endl;
    for(unsigned int i = 0; i < expectationMaximization.GetNumberOfModels(); ++i)
    {
      expectationMaximization.GetModel(i)->Print();
    }
}


template <typename TImage>
ForegroundBackgroundSegmentMask* ImageGraphCut<TImage>::GetSegmentMask()
{
  return this->ResultingSegments;
}


template <typename TImage>
TImage* ImageGraphCut<TImage>::GetImage()
{
  return this->Image;
}

template <typename TImage>
TImage* ImageGraphCut<TImage>::GetResultingForegroundImage()
{
    ForegroundBackgroundSegmentMask* segmentMask = GetSegmentMask();

    //segmentMask->Write<unsigned char>("resultingMask.png", ForegroundPixelValueWrapper<unsigned char>(0),
    //              BackgroundPixelValueWrapper<unsigned char>(255));

    ImageType::Pointer result = ImageType::New();
    ITKHelpers::DeepCopy(this->Image, result.GetPointer());
    ImageType::PixelType backgroundColor(3);
    backgroundColor.Fill(0);
    segmentMask->ApplyToImage(result.GetPointer(), backgroundColor);
    return result;
}


#endif
