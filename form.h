#ifndef FILEMENUFORM_H
#define FILEMENUFORM_H

#include "ui_form.h"

#include "ImageGraphCut.h"

#include <vtkSmartPointer.h>

#include <vnl/vnl_vector.h>

class vtkBorderWidget;
class vtkImageActor;
class vtkRenderer;
class vtkImageData;
class vtkImageClip;

class Form : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
public:
    Form(QWidget *parent = 0);

public slots:
    void actionOpen_triggered();
    void btnCut_clicked();

protected:
  vtkSmartPointer<ImageGraphCut> GraphCut;

  vtkSmartPointer<vtkImageActor> OriginalImageActor;
  vtkSmartPointer<vtkRenderer> LeftRenderer;
  vtkSmartPointer<vtkRenderer> RightRenderer;
  vtkSmartPointer<vtkImageData> OriginalImage;

  // Functions
  void Refresh();
  std::vector<vnl_vector<double> > CreateRGBPoints(unsigned char pointType);
  void CreateImageFromModels(vtkExpectationMaximization* emForeground, vtkExpectationMaximization* emBackground); // For sanity only
  void UpdateCropping();

  // Inputs
  vtkSmartPointer<vtkImageActor> ClippedActor;
  vtkSmartPointer<vtkImageClip> ClipFilter;

  // Internal
  vtkSmartPointer<vtkImageActor> MaskActor;
  vtkSmartPointer<vtkImageData> AlphaMask;
  vtkSmartPointer<vtkBorderWidget> BorderWidget;

  void CatchWidgetEvent(vtkObject* caller, long unsigned int eventId, void* callData);

  // Constants
  int FOREGROUNDALPHA;
  int BACKGROUNDALPHA;

  // Variables
  int NumberOfModels;

  // Shouldn't be in class:
  bool IsNaN(const double a);

};

#endif
