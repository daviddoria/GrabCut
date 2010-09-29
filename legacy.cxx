#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkJPEGReader.h>
#include <vtkImageActor.h>
#include <vtkCommand.h>
#include <vtkCallbackCommand.h>
#include <vtkSmartPointer.h>
#include <vtkInteractorStyleRubberBand2D.h>

void SelectionChangedCallbackFunction ( vtkObject* caller, long unsigned int eventId, void* clientData, void* callData );

int main(int argc, char* argv[])
{
  // Parse input arguments
  if ( argc != 2 )
  {
    std::cout << "Required parameters: Filename" << std::endl;
    exit(-1);
  }

  vtkstd::string inputFilename = argv[1];

  // Read the image
  vtkSmartPointer<vtkJPEGReader> reader =
    vtkSmartPointer<vtkJPEGReader>::New();
  reader->SetFileName(inputFilename.c_str());
  reader->Update();

  // Create an actor
  vtkSmartPointer<vtkImageActor> actor =
    vtkSmartPointer<vtkImageActor>::New();
  actor->SetInput(reader->GetOutput());

  // Setup the SelectionChangedEvent callback
  vtkSmartPointer<vtkCallbackCommand> selectionChangedCallback =
    vtkSmartPointer<vtkCallbackCommand>::New();
  selectionChangedCallback->SetCallback(SelectionChangedCallbackFunction);

   // Define viewport ranges
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = {0.0, 0.0, 0.5, 1.0};
  double rightViewport[4] = {0.5, 0.0, 1.0, 1.0};

  // Setup renderer
  vtkSmartPointer<vtkRenderer> leftRenderer =
    vtkSmartPointer<vtkRenderer>::New();
  leftRenderer->AddActor(actor);
  leftRenderer->ResetCamera();
  leftRenderer->SetViewport(leftViewport);

  // Setup renderer
  vtkSmartPointer<vtkRenderer> rightRenderer =
    vtkSmartPointer<vtkRenderer>::New();
  //rightRenderer->AddActor(actor);
  rightRenderer->ResetCamera();
  rightRenderer->SetViewport(rightViewport);

  // Setup render window
  vtkSmartPointer<vtkRenderWindow> renderWindow =
    vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer(leftRenderer);
  renderWindow->AddRenderer(rightRenderer);
  renderWindow->SetSize(800,400);

  // Setup 2D interaction style
  vtkSmartPointer<vtkInteractorStyleRubberBand2D> style =
    vtkSmartPointer<vtkInteractorStyleRubberBand2D>::New();
  style->AddObserver(vtkCommand::SelectionChangedEvent,selectionChangedCallback);

  // Setup render window interactor
  vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetInteractorStyle(style);

  // Render and start interaction
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindowInteractor->Initialize();

  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}

void SelectionChangedCallbackFunction ( vtkObject* caller, long unsigned int eventId, void* clientData, void* callData )
{
  vtkstd::cout << "Selection changed callback" << vtkstd::endl;

  unsigned int* rect = reinterpret_cast<unsigned int*> ( callData );
  unsigned int pos1X = rect[0];
  unsigned int pos1Y = rect[1];
  unsigned int pos2X = rect[2];
  unsigned int pos2Y = rect[3];

  vtkstd::cout << "Start x: " << pos1X << " Start y: " << pos1Y << " End x: " << pos2X << " End y: " << pos2Y << vtkstd::endl;
}

/*
//You could override this, but then you have to reimplement the functionality.
//Instead, you should use an observer

void vtkInteractorStyleRubberBand2D::OnLeftButtonUp()
{
  vtkstd::cout << "LeftButtonUp!" << vtkstd::endl;

  vtkstd::cout << "Start: " << this->StartPosition[0] << " " << this->StartPosition[1] << vtkstd::endl;
  vtkstd::cout << "End: " << this->EndPosition[0] << " " << this->EndPosition[1] << vtkstd::endl;

  //this->Superclass.OnLeftButtonUp(); //doesn't work

  InvokeEvent(vtkCommand::EndPickEvent);
  InvokeEvent(vtkCommand::SelectionChangedEvent);

}
*/
