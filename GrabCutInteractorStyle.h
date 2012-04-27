#ifndef GrabCutInteractorStyle_h
#define GrabCutInteractorStyle_h

#include <vtkInteractorStyleImage.h>
#include <vtkSmartPointer.h>
//#include "vtkMemberFunctionCommand.h"

class vtkRenderer;
class vtkImageActor;
class vtkImageClip;
class vtkBorderWidget;
class vtkImageData;
class vtkExpectationMaximization;

#include <vnl/vnl_vector.h>
#include <vector>

class GrabCutInteractorStyle : public vtkInteractorStyleImage
{
public:
  static GrabCutInteractorStyle* New();
  vtkTypeMacro(GrabCutInteractorStyle, vtkInteractorStyleImage);

  GrabCutInteractorStyle();

  void OnKeyPress();

  void UpdateCropping(vtkBorderWidget* borderWidget);

  bool IsInitialized();

protected:

  // Constants
  int FOREGROUNDALPHA;
  int BACKGROUNDALPHA;

  // Functions
  void Refresh();
  std::vector<vnl_vector<double> > CreateRGBPoints(unsigned char pointType);
  void CreateImageFromModels(vtkExpectationMaximization* emForeground, vtkExpectationMaximization* emBackground); // For sanity only

  // Inputs
  vtkSmartPointer<vtkRenderer> LeftRenderer;
  vtkSmartPointer<vtkRenderer> RightRenderer;
  vtkSmartPointer<vtkImageActor> ImageActor;
  vtkSmartPointer<vtkImageActor> ClippedActor;
  vtkSmartPointer<vtkImageClip> ClipFilter;
  vtkSmartPointer<vtkImageData> InputImage;

  // Internal
  vtkSmartPointer<vtkImageActor> MaskActor;
  vtkSmartPointer<vtkImageData> AlphaMask;
  vtkSmartPointer<vtkBorderWidget> BorderWidget;

  void CatchWidgetEvent(vtkObject* caller, long unsigned int eventId, void* callData);

  int NumberOfModels;

  // Should not be in the class
  bool IsNaN(const double a);
};

void PrintExtent(std::string arrayName, int extent[6]);

#endif