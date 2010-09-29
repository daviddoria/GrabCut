#ifndef IMAGEGRAPHCUT_H
#define IMAGEGRAPHCUT_H

#include <vtkImageAlgorithm.h> // Superclass
#include <vtkSmartPointer.h>

#include "graph.h"
typedef Graph<double,double,double> GraphType;

class vtkFloatArray;
class vtkImageData;
class vtkPolyData;
class vtkPoints;

class vtkExpectationMaximization;

class ImageGraphCut : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(ImageGraphCut,vtkImageAlgorithm);
  static ImageGraphCut *New();
  ImageGraphCut();

  void SetSourceSinkMask(vtkImageData*);
  bool IsValidPixel(int i, int j);
  bool IsValidPixel(int p[2]);

  // See definition in .cpp file
  /*
  static int SOURCE;
  static int SINK;
  static int ALWAYSSINK;
  */
  static unsigned char SOURCE;
  static unsigned char SINK;
  static unsigned char ALWAYSSINK;
  //enum NodeEnum{SOURCE, SINK, ALWAYSSINK};

  vtkExpectationMaximization* ForegroundEM;
  vtkExpectationMaximization* BackgroundEM;

protected:
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double ColorDistance(int a[2], int b[2]);

  double ComputeNoise();

  // Actually do computations
  void CreateGraph();
  void CutGraph();

  GraphType* Graph;

  vtkSmartPointer<vtkImageData> Image; // The input image
  vtkSmartPointer<vtkImageData> SourceSinkMask;

  float Lambda; // The weighting between unary and binary terms

  // Should not be in the class
  bool IsNaN(const double a);
};

#endif