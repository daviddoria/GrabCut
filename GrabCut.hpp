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
#include "Helpers/Helpers.h"
#include "ITKHelpers/ITKHelpers.h"

#include "ExpectationMaximization/ExpectationMaximization.h"
#include "ExpectationMaximization/GaussianModel.h"

// ITK
#include "itkImageRegionIterator.h"
#include "itkShapedNeighborhoodIterator.h"
#include "itkMaskImageFilter.h"

// STL
#include <cmath>
#include <sstream>

// Boost
#include <boost/graph/boykov_kolmogorov_max_flow.hpp>

template <typename TImage>
GrabCut<TImage>::GrabCut()
{
    this->Image = TImage::New();
    this->InitialMask = ForegroundBackgroundSegmentMask::New();
    this->SegmentationMask = ForegroundBackgroundSegmentMask::New();
}

template <typename TImage>
void GrabCut<TImage>::SetImage(TImage* const image)
{
    ITKHelpers::DeepCopy(image, this->Image.GetPointer());
}

template <typename TImage>
void GrabCut<TImage>::SetInitialMask(ForegroundBackgroundSegmentMask* const mask)
{
    // Save the initial mask
    ITKHelpers::DeepCopy(mask, this->InitialMask.GetPointer());

    // Initialize the segmentation mask from the initial mask
    ITKHelpers::DeepCopy(mask, this->SegmentationMask.GetPointer());
}

template <typename TImage>
Eigen::MatrixXd GrabCut<TImage>::CreateMatrixFromPixels(const std::vector<typename TImage::PixelType>& pixels)
{
    unsigned int dimensionality = this->GetDimensionality();
    Eigen::MatrixXd data(dimensionality, pixels.size());

    for(unsigned int i = 0; i < pixels.size(); i++)
    {
        Eigen::VectorXd v(dimensionality);
        PixelType p = pixels[i];
        for(unsigned int d = 0; d < dimensionality; ++d)
        {
            v(d) = p[d];
        }

        data.col(i) = v;
    }

    return data;
}

template <typename TImage>
void GrabCut<TImage>::InitializeModels(const unsigned int numberOfModels)
{
    unsigned int dimensionality = this->GetDimensionality();

    // Initialize the foreground and background mixture models
    std::vector<Model*> foregroundModels;
    std::vector<Model*> backgroundModels;
    foregroundModels.resize(numberOfModels);
    backgroundModels.resize(numberOfModels);

    for(unsigned int i = 0; i < numberOfModels; i++)
    {
      Model* foregroundModel = new GaussianModel(dimensionality);
      foregroundModels[i] = foregroundModel;

      Model* backgroundModel = new GaussianModel(dimensionality);
      backgroundModels[i] = backgroundModel;
    }

    this->ForegroundModels.SetModels(foregroundModels);
    this->BackgroundModels.SetModels(backgroundModels);
}

template <typename TImage>
MixtureModel GrabCut<TImage>::ClusterPixels(const std::vector<typename TImage::PixelType>& pixels, const MixtureModel& mixtureModel)
{
    Eigen::MatrixXd data = CreateMatrixFromPixels(pixels);

    ExpectationMaximization expectationMaximization;
    expectationMaximization.SetData(data);
    expectationMaximization.SetMixtureModel(mixtureModel);
    expectationMaximization.SetMinChange(1e-4); // Stop early if the model is doing well
    expectationMaximization.SetMaxIterations(this->NumberOfEMIterations);
    expectationMaximization.Compute();

    MixtureModel finalModel = expectationMaximization.GetMixtureModel();

    return finalModel;
}

template <typename TImage>
void GrabCut<TImage>::ClusterForegroundAndBackground()
{
    // Foreground
    std::vector<itk::Index<2> > foregroundPixelIndices =
        ITKHelpers::GetPixelsWithValue(this->SegmentationMask.GetPointer(), ForegroundBackgroundSegmentMaskPixelTypeEnum::FOREGROUND);

    std::vector<typename TImage::PixelType> foregroundPixels = ITKHelpers::GetPixelValues(this->Image.GetPointer(), foregroundPixelIndices);

    std::cout << "Starting foreground EM..." << std::endl;
    this->ForegroundModels = ClusterPixels(foregroundPixels, this->ForegroundModels);

    // Background
    std::vector<itk::Index<2> > backgroundPixelIndices =
        ITKHelpers::GetPixelsWithValue(this->SegmentationMask.GetPointer(), ForegroundBackgroundSegmentMaskPixelTypeEnum::BACKGROUND);

    std::vector<typename TImage::PixelType> backgroundPixels = ITKHelpers::GetPixelValues(this->Image.GetPointer(), backgroundPixelIndices);

    std::cout << "Starting background EM..." << std::endl;
    this->BackgroundModels = ClusterPixels(backgroundPixels, this->BackgroundModels);
}

template <typename TImage>
void GrabCut<TImage>::PerformSegmentation()
{
  InitializeModels(5); // The GrabCut paper suggests using 5 models per mixture model

  unsigned int iteration = 0;

  while(iteration < 10)
  {
      std::cout << "GrabCut iteration " << iteration << "..." << std::endl;
      PerformIteration();

      // Get and write the result
      std::stringstream ssOutput;
      ssOutput << "result_" << iteration << ".png";
      typename TImage::Pointer result = TImage::New();
      this->GetSegmentedImage(result);
      ITKHelpers::WriteImage(result.GetPointer(), ssOutput.str());

      iteration++;
  }

}

template <typename TImage>
void GrabCut<TImage>::PerformIteration()
{
    ClusterForegroundAndBackground();

    // The originally specified background pixels are the only ones that are definitely background (unless there is interactive refining performed)
    std::vector<itk::Index<2> > backgroundPixels =
        ITKHelpers::GetPixelsWithValue(this->InitialMask.GetPointer(), ForegroundBackgroundSegmentMaskPixelTypeEnum::BACKGROUND);

    // Perform the graph cut
    ImageGraphCut<TImage> graphCut;
    graphCut.SetImage(this->Image);
    graphCut.SetForegroundLikelihoodFunction(boost::bind(
                &GrabCut::
                ForegroundLikelihood, this, _1));

    graphCut.SetBackgroundLikelihoodFunction(boost::bind(
                &GrabCut::
                BackgroundLikelihood, this, _1));
    graphCut.SetSinks(backgroundPixels);
    graphCut.PerformSegmentation();

    ITKHelpers::DeepCopy(graphCut.GetSegmentMask(), this->SegmentationMask.GetPointer());
}

template <typename TImage>
ForegroundBackgroundSegmentMask* GrabCut<TImage>::GetSegmentationMask()
{
    return this->Segmentationmask;
}

template <typename TImage>
TImage* GrabCut<TImage>::GetImage()
{
    return this->Image;
}

template <typename TImage>
void GrabCut<TImage>::GetSegmentedImage(TImage* result)
{
    ITKHelpers::DeepCopy(this->Image.GetPointer(), result);
    typename TImage::PixelType backgroundColor(this->GetDimensionality());
    backgroundColor.Fill(0);
    this->SegmentationMask->ApplyToImage(result, backgroundColor);
}

template <typename TImage>
float GrabCut<TImage>::ForegroundLikelihood(const typename TImage::PixelType& pixel)
{
    Eigen::VectorXd p(this->GetDimensionality());
    p(0) = pixel[0];
    p(1) = pixel[1];
    p(2) = pixel[2];

    float likelihood = this->ForegroundModels.WeightedEvaluate(p);

    return likelihood;
}

template <typename TImage>
float GrabCut<TImage>::BackgroundLikelihood(const typename TImage::PixelType& pixel)
{
    Eigen::VectorXd p(this->GetDimensionality());
    p(0) = pixel[0];
    p(1) = pixel[1];
    p(2) = pixel[2];

    float likelihood = this->BackgroundModels.WeightedEvaluate(p);

    return likelihood;
}

#endif
