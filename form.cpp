#include "form.h"

#include <vtkBorderRepresentation.h>
#include <vtkBorderWidget.h>
#include <vtkCoordinate.h>
#include <vtkImageActor.h>
#include <vtkImageClip.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkImageReader2Factory.h>
#include <vtkImageReader2.h>
#include <vtkInteractorStyleImage.h>
#include <vtkJPEGWriter.h>
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QFileDialog>

#include "ExpectationMaximization/Model.h"
#include "ExpectationMaximization/GaussianND.h"
#include "ExpectationMaximization/vtkExpectationMaximization.h"

#include "ImageGraphCut.h"

Form::Form(QWidget *parent)
{
  setupUi(this);
  connect( this->actionOpen, SIGNAL( triggered() ), this, SLOT(actionOpen_triggered()) );
  connect( this->btnCut, SIGNAL( clicked() ), this, SLOT(btnCut_clicked()) );

  // Setup renderers
  this->LeftRenderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkInteractorStyleImage> interactorStyleImage =
    vtkSmartPointer<vtkInteractorStyleImage>::New();
  this->qvtkWidgetLeft->GetInteractor()->SetInteractorStyle(interactorStyleImage);
  this->qvtkWidgetLeft->GetRenderWindow()->AddRenderer(this->LeftRenderer);

  this->RightRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->qvtkWidgetRight->GetRenderWindow()->AddRenderer(this->RightRenderer);

  // Instantiations
  this->GraphCut = vtkSmartPointer<ImageGraphCut>::New();
  this->OriginalImageActor = vtkSmartPointer<vtkImageActor>::New();
  this->OriginalImage = vtkSmartPointer<vtkImageData>::New();
  this->BorderWidget = vtkSmartPointer<vtkBorderWidget>::New();

  // Setup ClipFilter for cropping
  this->ClipFilter = vtkSmartPointer<vtkImageClip>::New();
  this->ClipFilter->SetInputData(this->OriginalImage);
  this->ClippedActor = vtkSmartPointer<vtkImageActor>::New();

  // Setup BorderWidget (the rectangle used to crop the left (original) image
  this->BorderWidget->SetInteractor(this->qvtkWidgetLeft->GetInteractor());
  this->BorderWidget->On();
  this->BorderWidget->AddObserver(vtkCommand::InteractionEvent, this, &Form::CatchWidgetEvent);

}

void Form::actionOpen_triggered()
{
  // Get a filename to open
  QString filename = QFileDialog::getOpenFileName(this,
     tr("Open Image"), "/media/portable/Projects/src/GrabCut/data", tr("Image Files (*.png *.jpg *.bmp)"));

  if(filename.isEmpty())
    {
    std::cout << "Cancelled." << std::endl;
    return;
    }

  std::cout << "Got filename: " << filename.toStdString() << std::endl;

  // Read file
  vtkSmartPointer<vtkImageReader2Factory> readerFactory =
    vtkSmartPointer<vtkImageReader2Factory>::New();
  vtkImageReader2* imageReader = readerFactory->CreateImageReader2(filename.toStdString().c_str());
  imageReader->SetFileName(filename.toStdString().c_str());
  imageReader->Update();

  this->OriginalImage->ShallowCopy(imageReader->GetOutput());

  this->OriginalImageActor->SetInputData(this->OriginalImage);

  imageReader->Delete();

  this->LeftRenderer->AddActor(OriginalImageActor);
  this->LeftRenderer->ResetCamera();
}

void Form::Refresh()
{
  this->LeftRenderer->Render();
  this->RightRenderer->Render();
  this->qvtkWidgetRight->GetRenderWindow()->Render();
  this->qvtkWidgetLeft->GetRenderWindow()->Render();
}

#if 0

  vtkSmartPointer<vtkImageClip> imageClip =
    vtkSmartPointer<vtkImageClip>::New();
  imageClip->SetInputConnection(jPEGReader->GetOutputPort());
  imageClip->SetOutputWholeExtent(jPEGReader->GetOutput()->GetWholeExtent());
  imageClip->ClipDataOn();

  vtkSmartPointer<vtkImageActor> clipActor =
    vtkSmartPointer<vtkImageActor>::New();
  clipActor->SetInput(imageClip->GetOutput());
  rightRenderer->AddActor(clipActor);

  vtkSmartPointer<GrabCutInteractorStyle> style =
    vtkSmartPointer<GrabCutInteractorStyle>::New();
  interactor->SetInteractorStyle(style);
  style->SetLeftRenderer(leftRenderer);
  style->SetRightRenderer(rightRenderer);
  style->SetImageActor(imageActor);
  style->SetClipFilter(imageClip);
  style->SetInputImage(jPEGReader->GetOutput());

  interactor->SetRenderWindow(renderWindow);
  renderWindow->Render();

  interactor->Start();
#endif

void Form::btnCut_clicked()
{
  // Setup the mask
  int extent[6];
  this->OriginalImage->GetExtent(extent);
  //PrintExtent("extent", extent);

  this->AlphaMask->SetExtent(extent);
  this->AlphaMask->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
  
  int clippedExtent[6];
  ClipFilter->GetOutput()->GetExtent(clippedExtent);
  //PrintExtent("clippedExtent", clippedExtent);

  // Initialize the mask (everything background)
  for(int y = extent[2]; y <= extent[3]; y++)
    {
    for(int x = extent[0]; x <= extent[1]; x++)
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
  writer->SetInputData(this->AlphaMask);
  writer->SetFileName("InitialMask.jpg");
  writer->Write();

  unsigned int numberOfMixtures = 5;

  std::vector<Model*> foregroundModels(numberOfMixtures);

  for(unsigned int i = 0; i < foregroundModels.size(); i++)
    {
    Model* model = new GaussianND;
    model->SetDimensionality(3); // rgb
    model->Init();
    foregroundModels[i] = model;
    }

  std::vector<Model*> backgroundModels(numberOfMixtures);

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

  unsigned int totalIterations = 3;
  for(unsigned int i = 0; i < totalIterations; i++)
    {
    std::cout << "Grabcuts iteration " << i << std::endl;

    // Convert these RGB colors to XYZ points to feed to EM

    std::vector<vnl_vector<double> > foregroundRGBpoints = CreateRGBPoints(ImageGraphCut::SOURCE);
    std::vector<vnl_vector<double> > backgroundRGBpoints = CreateRGBPoints(ImageGraphCut::SINK);
    std::vector<vnl_vector<double> > alwaysBackgroundRGBpoints = CreateRGBPoints(ImageGraphCut::ALWAYSSINK);
    backgroundRGBpoints.insert(backgroundRGBpoints.end(), alwaysBackgroundRGBpoints.begin(), alwaysBackgroundRGBpoints.end());
    std::cout << "There are " << foregroundRGBpoints.size() << " foreground points." << std::endl;
    if(foregroundRGBpoints.size() < 10)
      {
      std::cerr << "There are not enough foreground points!" << std::endl;
      exit(-1);
      }
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
    graphCutFilter->SetInputData(this->OriginalImage);
    graphCutFilter->SetSourceSinkMask(this->AlphaMask);
    graphCutFilter->Update();

    std::cout << "Refreshing..." << std::endl;
    this->AlphaMask->ShallowCopy(graphCutFilter->GetOutput());

    graphCutFilter->Modified();
    emForeground->Modified();
    emBackground->Modified();

    this->Refresh();

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
  mapTransparency->SetInputData(graphCutFilter->GetOutput());
  mapTransparency->PassAlphaToOutputOn();

  this->MaskActor->SetInputData(mapTransparency->GetOutput());

  this->RightRenderer->AddActor(this->MaskActor);
  this->Refresh();
}


void Form::CatchWidgetEvent(vtkObject* caller, long unsigned int eventId, void* callData)
{
  this->UpdateCropping();
}

void Form::UpdateCropping()
{
  //this->ClipFilter->SetInputConnection(this->OriginalImage->GetProducerPort());

  // Get the world coordinates of the two corners of the box
  vtkCoordinate* lowerLeftCoordinate =
    static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->GetPositionCoordinate();
  lowerLeftCoordinate->SetViewport(this->LeftRenderer);
  double* lowerLeft =
    lowerLeftCoordinate->GetComputedWorldValue(this->LeftRenderer);

  vtkCoordinate* upperRightCoordinate =
    static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->GetPosition2Coordinate();
  double* upperRight =
    upperRightCoordinate->GetComputedWorldValue(this->LeftRenderer);

  double* bounds = this->OriginalImageActor->GetBounds();
  double xmin = bounds[0];
  double xmax = bounds[1];
  double ymin = bounds[2];
  double ymax = bounds[3];

  this->ClipFilter->SetOutputWholeExtent(
      vtkMath::Round(std::max(lowerLeft[0], static_cast<double>(0))),
      vtkMath::Round(std::min(upperRight[0], xmax)),
      vtkMath::Round(std::max(lowerLeft[1], static_cast<double>(0))),
      vtkMath::Round(std::min(upperRight[1], ymax)),
      0, 0);

  if( (lowerLeft[0] > xmax) ||
      (upperRight[0] < xmin) ||
      (lowerLeft[1] > ymax) ||
      (upperRight[1] < ymin) )
    {
    this->ClipFilter->SetOutputWholeExtent(0,0,0,0,0,0);
    std::cout << "box is NOT inside image" << std::endl;
    }

  this->ClipFilter->Update();
  this->ClippedActor->SetInputData(this->ClipFilter->GetOutput());
  this->RightRenderer->AddActor(this->ClippedActor);
  this->RightRenderer->ResetCamera();

  this->Refresh();

}



std::vector<vnl_vector<double> > Form::CreateRGBPoints(unsigned char pointType)
{
  std::vector<vnl_vector<double> > rgbPoints;

  int extent[6];
  this->OriginalImage->GetExtent(extent);
  //PrintExtent("extent", extent);

  // Loop over the mask image
  for(int y = extent[2]; y <= extent[3]; y++)
    {
    for(int x = extent[0]; x <= extent[1]; x++)
      {
      unsigned char* maskValue = static_cast<unsigned char*>(this->AlphaMask->GetScalarPointer(x,y,0));
      unsigned char* pixel = static_cast<unsigned char*>(this->OriginalImage->GetScalarPointer(x,y,0));
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

bool Form::IsNaN(const double a)
{
  if(a!=a)
  {
    return true;
  }
  return false;
}

void Form::CreateImageFromModels(vtkExpectationMaximization* emForeground, vtkExpectationMaximization* emBackground)
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
      unsigned char* inputPixel = static_cast<unsigned char*>(this->OriginalImage->GetScalarPointer(i,j,0));

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
  writer->SetInputData(image);
  writer->SetFileName("BeforeGraphCuts.jpg");
  writer->Write();
}

/*
  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->GetBorderProperty()->SetColor(0,1,0);
  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->SetPosition(0.4,0.4);
  static_cast<vtkBorderRepresentation*>
    (this->BorderWidget->GetRepresentation())->SetPosition2(0.2,0.2);
  this->BorderWidget->SelectableOff();



  this->AlphaMask = vtkSmartPointer<vtkImageData>::New();

  this->MaskActor = vtkSmartPointer<vtkImageActor>::New();

  // Defaults
  this->NumberOfModels = 5;

  // Fixed values
  this->FOREGROUNDALPHA = 1;
  this->BACKGROUNDALPHA = 0;

  this->LeftRenderer = NULL;
  this->RightRenderer = NULL;
  this->ImageActor = NULL;
  this->InputImage = NULL;

*/