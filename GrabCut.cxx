#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkActor.h>
#include <vtkBorderRepresentation.h>
#include <vtkBorderWidget.h>
#include <vtkImageActor.h>
#include <vtkImageClip.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkJPEGReader.h>
#include <vtkMath.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkXMLPolyDataReader.h>

#include "GrabCutInteractorStyle.h"
#include "vtkExpectationMaximization.h"

int main(int argc, char* argv[])
{
  // Parse input arguments
  if ( argc != 2 )
    {
    std::cerr << "Usage: " << argv[0]
              << " Filename(.jpg)" << std::endl;
    return EXIT_FAILURE;
    }

  std::string inputFilename = argv[1];

  // Read the image
  vtkSmartPointer<vtkJPEGReader> jPEGReader =
    vtkSmartPointer<vtkJPEGReader>::New();

  if(!jPEGReader->CanReadFile( inputFilename.c_str() ) )
    {
    std::cout << "Error: cannot read " << inputFilename << std::endl;
    return EXIT_FAILURE;
    }

  jPEGReader->SetFileName ( inputFilename.c_str() );
  jPEGReader->Update();
  
  std::cout << "Input image pixels are type: " << jPEGReader->GetOutput()->GetScalarTypeAsString() << std::endl;

  vtkSmartPointer<vtkImageActor> imageActor =
    vtkSmartPointer<vtkImageActor>::New();
  imageActor->SetInput(jPEGReader->GetOutput());

  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetSize(800,400);

  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();

  // Define viewport ranges in normalized coordinates
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = {0.0, 0.0, 0.5, 1.0};
  double rightViewport[4] = {0.5, 0.0, 1.0, 1.0};

  // Setup both renderers
  vtkSmartPointer<vtkRenderer> leftRenderer =
    vtkSmartPointer<vtkRenderer>::New();
  leftRenderer->SetBackground(1,0,0);
  renderWindow->AddRenderer(leftRenderer);
  leftRenderer->SetViewport(leftViewport);

  vtkSmartPointer<vtkRenderer> rightRenderer =
    vtkSmartPointer<vtkRenderer>::New();
  renderWindow->AddRenderer(rightRenderer);
  rightRenderer->SetViewport(rightViewport);

  leftRenderer->AddActor(imageActor);

  leftRenderer->ResetCamera();
  rightRenderer->ResetCamera();

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

  return EXIT_SUCCESS;
}