
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

#include "GrabCut.h"

// Submodules
#include "Mask/ITKHelpers/Helpers/Helpers.h"
#include "Mask/ITKHelpers/ITKHelpers.h"
#include "Mask/StrokeMask.h"

// ITK
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionConstIteratorWithIndex.h"
#include "itkVectorImage.h"


int main(int argc, char*argv[])
{
  // Verify arguments
  if(argc != 4)
    {
    std::cerr << "Required: image.png mask.fgmask output.png" << std::endl;
    return EXIT_FAILURE;
    }

  // Parse arguments
  std::string imageFilename = argv[1];

  // This image should have white pixels indicating foreground pixels and be black elsewhere.
  std::string maskFilename = argv[2];

  std::string outputFilename = argv[3];

  // Output arguments
  std::cout << "imageFilename: " << imageFilename << std::endl
            << "maskFilename: " << maskFilename << std::endl
            << "outputFilename: " << outputFilename << std::endl;

  // The type of the image to segment
  typedef itk::Image<itk::CovariantVector<unsigned char, 3>, 2> ImageType;

  // Read the image
  typedef itk::ImageFileReader<ImageType> ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(imageFilename);
  reader->Update();

  // Read the mask
  ForegroundBackgroundSegmentMask::Pointer mask =
      ForegroundBackgroundSegmentMask::New();
  mask->Read(maskFilename);

  // Perform the segmentation
  std::cout << "Starting GrabCut..." << std::endl;
  GrabCut<ImageType> grabCut;
  grabCut.SetImage(reader->GetOutput());
  grabCut.SetMask(mask);
  grabCut.PerformSegmentation();

  // Get and write the result


  ITKHelpers::WriteImage(result.GetPointer(), outputFilename);

}
