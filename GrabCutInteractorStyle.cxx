#include "GrabCutInteractorStyle.h"

#include <vtkObjectFactory.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>
#include <vtkImageData.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCoordinate.h>
#include <vtkMath.h>
#include <vtkBorderWidget.h>
#include <vtkBorderRepresentation.h>
#include <vtkImageActor.h>
#include <vtkImageClip.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkProperty2D.h>
#include <vtkJPEGWriter.h>

#include <vector>

#include <Model.h>
#include <GaussianND.h>
#include <vtkExpectationMaximization.h>
#include "ImageGraphCut.h"

vtkStandardNewMacro(GrabCutInteractorStyle);

GrabCutInteractorStyle::GrabCutInteractorStyle()
{
  this->BorderWidget = vtkSmartPointer<vtkBorderWidget>::New();

  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->GetBorderProperty()->SetColor(0,1,0);
  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->SetPosition(0.4,0.4);
  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->SetPosition2(0.2,0.2);
  this->BorderWidget->SelectableOff();

  this->Adapter = vtkMemberFunctionCommand<GrabCutInteractorStyle>::New();
  this->Adapter->SetCallback(*this, &GrabCutInteractorStyle::CatchWidgetEvent);
  this->BorderWidget->AddObserver(vtkCommand::InteractionEvent, this->Adapter);

  this->AlphaMask = vtkSmartPointer<vtkImageData>::New();

  this->MaskActor = vtkSmartPointer<vtkImageActor>::New();

  // Defaults
  this->NumberOfModels = 5;

  // Fixed values
  this->FOREGROUNDALPHA = 1;
  this->BACKGROUNDALPHA = 0;
}

void GrabCutInteractorStyle::OnKeyPress()
{
  // Get the keypress
  std::string key = this->Interactor->GetKeySym();

  if(key.compare("s") == 0) // 's' for "Select"
    {
    this->BorderWidget->SetInteractor(this->Interactor);
    this->BorderWidget->On();
    this->UpdateCropping(this->BorderWidget);
    }
  else if(key.compare("c") == 0) // 'c' for "Cut"
    {

    // Setup the mask
    int extent[6];
    this->InputImage->GetExtent(extent);
    //PrintExtent("extent", extent);

    this->AlphaMask->SetExtent(extent);
    this->AlphaMask->SetNumberOfScalarComponents(1);
    //this->AlphaMask->SetScalarTypeToDouble();
    this->AlphaMask->SetScalarTypeToUnsignedChar();

    int clippedExtent[6];
    ClipFilter->GetOutput()->GetExtent(clippedExtent);
    //PrintExtent("clippedExtent", clippedExtent);

    // Initialize the mask (everything background)
    for (int y = extent[2]; y <= extent[3]; y++)
      {
      for (int x = extent[0]; x <= extent[1]; x++)
        {
        unsigned char* pixel = static_cast<unsigned char*>(this->AlphaMask->GetScalarPointer(x,y,0));
        pixel[0] = ImageGraphCut::ALWAYSSINK;
        }
      }

    // Mask the foreground
    for(int y = clippedExtent[2]; y <= clippedExtent[3]; y++)
      {
      for(int x = clippedExtent[0]; x <= clippedExtent[1]; x++)
        {
        unsigned char* pixel = static_cast<unsigned char*>(this->AlphaMask->GetScalarPointer(x,y,0));
        pixel[0] = ImageGraphCut::SOURCE;
        }
      }

    vtkSmartPointer<vtkJPEGWriter> writer =
      vtkSmartPointer<vtkJPEGWriter>::New();
    writer->SetInputConnection(this->AlphaMask->GetProducerPort());
    writer->SetFileName("InitialMask.jpg");
    writer->Write();

    std::vector<Model*> foregroundModels(3);

    for(unsigned int i = 0; i < foregroundModels.size(); i++)
      {
      Model* model = new GaussianND;
      model->SetDimensionality(3); // rgb
      model->Init();
      foregroundModels[i] = model;
      }

    std::vector<Model*> backgroundModels(3);

    for(unsigned int i = 0; i < backgroundModels.size(); i++)
      {
      Model* model = new GaussianND;
      model->SetDimensionality(3); // rgb
      model->Init();
      backgroundModels[i] = model;
      }

    vtkSmartPointer<vtkExpectationMaximization> emForeground =
      vtkSmartPointer<vtkExpectationMaximization>::New();
    emForeground->SetMinChange(2.0);
    emForeground->SetInitializationTechniqueToKMeans();

    vtkSmartPointer<vtkExpectationMaximization> emBackground =
      vtkSmartPointer<vtkExpectationMaximization>::New();
    emBackground->SetMinChange(2.0);
    emBackground->SetInitializationTechniqueToKMeans();

    vtkSmartPointer<ImageGraphCut> graphCutFilter =
      vtkSmartPointer<ImageGraphCut>::New();
    graphCutFilter->BackgroundEM = emBackground;
    graphCutFilter->ForegroundEM = emForeground;

    unsigned int totalIterations = 1;
    for(unsigned int i = 0; i < totalIterations; i++)
      {
      std::cout << "Grabcuts iteration " << i << std::endl;
      // Convert these RGB colors to XYZ points to feed to EM

      std::vector<vnl_vector<double> > foregroundRGBpoints = CreateRGBPoints(ImageGraphCut::SOURCE);
      std::vector<vnl_vector<double> > backgroundRGBpoints = CreateRGBPoints(ImageGraphCut::SINK);
      std::vector<vnl_vector<double> > alwaysBackgroundRGBpoints = CreateRGBPoints(ImageGraphCut::ALWAYSSINK);
      backgroundRGBpoints.insert(backgroundRGBpoints.end(), alwaysBackgroundRGBpoints.begin(), alwaysBackgroundRGBpoints.end());
      std::cout << "There are " << foregroundRGBpoints.size() << " foreground points." << std::endl;
      std::cout << "There are " << backgroundRGBpoints.size() << " background points." << std::endl;

      std::cout << "Foreground EM..." << std::endl;
      emForeground->SetData(foregroundRGBpoints);
      emForeground->SetModels(foregroundModels);
      emForeground->Update();

      std::cout << "Background EM..." << std::endl;
      emBackground->SetData(backgroundRGBpoints);
      emBackground->SetModels(backgroundModels);
      emBackground->Update();

      // Create image from models (this is for sanity only)
      CreateImageFromModels(emForeground, emBackground);

      std::cout << "Cutting graph..." << std::endl;
      graphCutFilter->SetInputConnection(this->InputImage->GetProducerPort());
      graphCutFilter->SetSourceSinkMask(this->AlphaMask);
      graphCutFilter->Update();

      std::cout << "Refreshing..." << std::endl;
      this->AlphaMask->ShallowCopy(graphCutFilter->GetOutput());

      graphCutFilter->Modified();
      emForeground->Modified();
      emBackground->Modified();


      this->RightRenderer->Render();
      this->Interactor->GetRenderWindow()->Render();

      std::stringstream ss;
      ss << i << ".jpg";
      writer->SetFileName(ss.str().c_str());
      writer->Write();
      }


    vtkSmartPointer<vtkLookupTable> lookupTable =
      vtkSmartPointer<vtkLookupTable>::New();
    lookupTable->SetNumberOfTableValues(3);
    lookupTable->SetRange(0.0,255.0);
    lookupTable->SetTableValue(0, 0.0, 0.0, 0.0, ImageGraphCut::SINK); //transparent
    lookupTable->SetTableValue(1, 0.0, 0.0, 0.0, ImageGraphCut::ALWAYSSINK); //transparent
    lookupTable->SetTableValue(2, 0.0, 1.0, 0.0, FOREGROUNDALPHA); //opaque and green
    lookupTable->Build();

    vtkSmartPointer<vtkImageMapToColors> mapTransparency =
      vtkSmartPointer<vtkImageMapToColors>::New();
    mapTransparency->SetLookupTable(lookupTable);
    mapTransparency->SetInput(graphCutFilter->GetOutput());
    mapTransparency->PassAlphaToOutputOn();

    this->MaskActor->SetInput(mapTransparency->GetOutput());
    this->RightRenderer->AddActor(this->MaskActor);
    this->RightRenderer->Render();
    this->Interactor->GetRenderWindow()->Render();
    } // end if 'c' pressed
}

void GrabCutInteractorStyle::CatchWidgetEvent(vtkObject* caller, long unsigned int eventId, void* callData)
{
  this->UpdateCropping(this->BorderWidget);
}

void GrabCutInteractorStyle::UpdateCropping(vtkBorderWidget *borderWidget)
{
  // Get the world coordinates of the two corners of the box
  vtkCoordinate* lowerLeftCoordinate =
    static_cast<vtkBorderRepresentation*>
    (borderWidget->GetRepresentation())->GetPositionCoordinate();
  double* lowerLeft =
    lowerLeftCoordinate->GetComputedWorldValue(this->LeftRenderer);

  vtkCoordinate* upperRightCoordinate =
    static_cast<vtkBorderRepresentation*>
    (borderWidget->GetRepresentation())->GetPosition2Coordinate();
  double* upperRight =
    upperRightCoordinate->GetComputedWorldValue(this->LeftRenderer);

  double* bounds = this->ImageActor->GetBounds();
  double xmin = bounds[0];
  double xmax = bounds[1];
  double ymin = bounds[2];
  double ymax = bounds[3];

  if( (lowerLeft[0] > xmin) &&
      (upperRight[0] < xmax) &&
      (lowerLeft[1] > ymin) &&
      (upperRight[1] < ymax) )
    {
    this->ClipFilter->SetOutputWholeExtent(
      vtkMath::Round(lowerLeft[0]),
      vtkMath::Round(upperRight[0]),
      vtkMath::Round(lowerLeft[1]),
      vtkMath::Round(upperRight[1]), 0, 1);
    }
  else
    {
    std::cout << "box is NOT inside image" << std::endl;
    }

  this->LeftRenderer->Render();
  this->RightRenderer->Render();

  this->RightRenderer->GetRenderWindow()->Render();

}

std::vector<vnl_vector<double> > GrabCutInteractorStyle::CreateRGBPoints(unsigned char pointType)
{
  std::vector<vnl_vector<double> > rgbPoints;

  int extent[6];
  this->InputImage->GetExtent(extent);
  //PrintExtent("extent", extent);

  // Loop over the mask image
  for(int y = extent[2]; y <= extent[3]; y++)
    {
    for(int x = extent[0]; x <= extent[1]; x++)
      {
      unsigned char* maskValue = static_cast<unsigned char*>(this->AlphaMask->GetScalarPointer(x,y,0));
      unsigned char* pixel = static_cast<unsigned char*>(this->InputImage->GetScalarPointer(x,y,0));
      if(maskValue[0] == pointType)
        {
        vnl_vector<double> v(3);
        for(unsigned int d = 0; d < 3; d++)
          {
          v(d) = pixel[d];
          if(v(d) < 0 || v(d) > 255)
            {
            std::cout << "Invalid! " << v(d) << " must be 0 < v(d) < 255!" << std::endl;
            exit(-1);
            }
          }
        rgbPoints.push_back(v);
        }
      }
    }

  return rgbPoints;
}

void PrintExtent(std::string arrayName, int extent[6])
{
     std::cout << arrayName << ": " << extent[0] << " " << extent[1] << " " << extent[2] << " "
	      << extent[3] << " " << extent[4] << " " << extent[5] << std::endl;
}

bool GrabCutInteractorStyle::IsNaN(const double a)
{
  if(a!=a)
  {
    return true;
  }
  return false;
}

void GrabCutInteractorStyle::CreateImageFromModels(vtkExpectationMaximization* emForeground, vtkExpectationMaximization* emBackground)
{
  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();
  image->DeepCopy(this->AlphaMask);

  int extent[6];
  image->GetExtent(extent);

  unsigned int counter = 0;
  for(int i = extent[0]; i < extent[1]; i++)
    {
    for(int j = extent[2]; j < extent[3]; j++)
      {
      unsigned char* outputPixel = static_cast<unsigned char*>(image->GetScalarPointer(i,j,0));
      unsigned char* inputPixel = static_cast<unsigned char*>(this->InputImage->GetScalarPointer(i,j,0));

      int p[3] = {i,j,0};
      vtkIdType currentIndex = image->ComputePointId(p);

      vnl_vector<double> rgb(3);
      rgb(0) = inputPixel[0];
      rgb(1) = inputPixel[1];
      rgb(2) = inputPixel[2];
      double backgroundLikelihood = emBackground->WeightedEvaluate(rgb);
      double foregroundLikelihood = emForeground->WeightedEvaluate(rgb);

      if(foregroundLikelihood > backgroundLikelihood)
        {
        //std::cout << "foreground pixel" << std::endl;
        counter++;
        outputPixel[0] = 255;
        outputPixel[1] = 255;
        outputPixel[2] = 255;
        }
      else //background
        {
        outputPixel[0] = 0;
        outputPixel[1] = 0;
        outputPixel[2] = 0;
        }
      }
    }

  std::cout << "There are " << counter << " foreground pixels." << std::endl;
  vtkSmartPointer<vtkJPEGWriter> writer =
    vtkSmartPointer<vtkJPEGWriter>::New();
  writer->SetInputConnection(image->GetProducerPort());
  writer->SetFileName("BeforeGraphCuts.jpg");
  writer->Write();
}