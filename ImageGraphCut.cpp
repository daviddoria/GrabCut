#include "ImageGraphCut.h"

#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>

#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include "ExpectationMaximization/vtkExpectationMaximization.h"

vtkStandardNewMacro(ImageGraphCut);

/*
int ImageGraphCut::SOURCE = 255;
int ImageGraphCut::SINK = 0;
int ImageGraphCut::ALWAYSSINK = 122;
*/
unsigned char ImageGraphCut::SOURCE = 255;
unsigned char ImageGraphCut::SINK = 0;
unsigned char ImageGraphCut::ALWAYSSINK = 122;

ImageGraphCut::ImageGraphCut()
{
  this->Image = vtkSmartPointer<vtkImageData>::New();
  this->SourceSinkMask = vtkSmartPointer<vtkImageData>::New();

  this->Lambda = 0.01;
}

int ImageGraphCut::RequestData(vtkInformation *vtkNotUsed(request),
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *outputVector)
{
  std::cout << "ImageGraphCut::RequestData" << std::endl;
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and ouptut
  vtkImageData *input = vtkImageData::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->Image->ShallowCopy(input);

  vtkImageData *output = vtkImageData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();
  image->ShallowCopy(input);

  this->CreateGraph();
  this->CutGraph();

  output->ShallowCopy(this->SourceSinkMask);

  return 1;
}

void ImageGraphCut::SetSourceSinkMask(vtkImageData* mask)
{
  this->SourceSinkMask->ShallowCopy(mask);
}

void ImageGraphCut::CreateGraph()
{
  int dims[3];
  this->Image->GetDimensions(dims);

  // Construct the graph

  // There are dimx*dimy vertices
  // There are dimx(dimy - 1) vertical edges and dimy(dimx-1) horizontal edges
  this->Graph = new GraphType(dims[0]*dims[1], dims[0]*(dims[1]-1)+dims[1]*(dims[0]-1));

  //vtkDataArray *scalar = this->Image->GetPointData()->GetScalars();

  // Add a node in the graph for every pixel
  for(int i = 0; i < dims[0]*dims[1]; i++)
    {
    this->Graph->add_node();
    }

  int extent[6];
  this->Image->GetExtent(extent);

  // Set n-edges (links between image nodes)
  double sigma = this->ComputeNoise();

  int pn[3] = {0,0,0}; // next pixel
  int p[3] = {0,0,0};
  double colorDistance;

  for(int i = extent[0]; i < extent[1]; i++)
    {
    for(int j = extent[2]; j < extent[3]; j++)
      {
      // Current node
      p[0] = i;
      p[1] = j;
      vtkIdType currentIndex = this->Image->ComputePointId(p);

      // Left node
      pn[0] = p[0]-1;
      pn[1] = p[1];

      if(IsValidPixel(pn))
	{
	vtkIdType nextIndex = this->Image->ComputePointId(pn);
	colorDistance = ColorDistance(p,pn);
	double w = exp(-colorDistance/sigma/2);
	this->Graph->add_edge(currentIndex,nextIndex,w,w);
	}

      // Right node
      pn[0] = p[0]+1;
      pn[1] = p[1];

      if(IsValidPixel(pn))
	{
	vtkIdType nextIndex = this->Image->ComputePointId(pn);
	colorDistance = ColorDistance(p,pn);
	double w = exp(-colorDistance/sigma/2);
	this->Graph->add_edge(currentIndex,nextIndex,w,w);
	}

      // Lower node
      pn[0] = p[0];
      pn[1] = p[1] - 1;
      if(IsValidPixel(pn))
	{
	vtkIdType nextIndex = this->Image->ComputePointId(pn);
	colorDistance = ColorDistance(p,pn);
	double w = exp(-colorDistance/sigma/2);
	this->Graph->add_edge(currentIndex,nextIndex,w,w);
	}

      // Upper node
      pn[0] = p[0];
      pn[1] = p[1] + 1;
      if(IsValidPixel(pn))
	{
	vtkIdType nextIndex = this->Image->ComputePointId(pn);
	colorDistance = ColorDistance(p,pn);
	double w = exp(-colorDistance/sigma/2);
	this->Graph->add_edge(currentIndex,nextIndex,w,w);
	}
      }
    } // end n-weight loop

  double LARGEWEIGHT = 10000000;
  // Set t-edges (links from image nodes to virtual background and virtual foreground node)

  for(int i = extent[0]; i < extent[1]; i++)
    {
    for(int j = extent[2]; j < extent[3]; j++)
      {
      unsigned char* pixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i,j,0));
      unsigned char* maskValue = static_cast<unsigned char*>(this->SourceSinkMask->GetScalarPointer(i,j,0));

      int p[3] = {i,j,0};
      vtkIdType currentIndex = this->Image->ComputePointId(p);

      if(maskValue[0] == ALWAYSSINK)
        {
        this->Graph->add_tweights(currentIndex,0,LARGEWEIGHT);
        }
      else if(maskValue[0] == SOURCE || maskValue[0] == SINK)
	{
	vnl_vector<double> rgb(3);
	rgb(0) = pixel[0];
	rgb(1) = pixel[1];
	rgb(2) = pixel[2];
	//std::cout << "rgb: " << rgb << std::endl;
	double backgroundLikelihood = this->BackgroundEM->WeightedEvaluate(rgb);
    if(IsNaN(backgroundLikelihood))
      {
      std::cout << "backgroundLikelihood: " << backgroundLikelihood << std::endl;
      exit(-1);
      }

	double backgroundWeight = -this->Lambda*log(this->BackgroundEM->WeightedEvaluate(rgb));
	double foregroundWeight = -this->Lambda*log(this->ForegroundEM->WeightedEvaluate(rgb));
	//std::cout << "backgroundWeight: " << backgroundWeight << std::endl;
	//std::cout << "foregroundWeight: " << foregroundWeight << std::endl;
	this->Graph->add_tweights(currentIndex,backgroundWeight,foregroundWeight);
	}
      else
	{
	std::cerr << "Invalid mask value " << (int)maskValue[0] << " (this should never happen)!" << std::endl;
	exit(-1);
	}
      }
    }
}

bool ImageGraphCut::IsValidPixel(int i, int j)
{
  int size[3];
  this->Image->GetDimensions(size);

  if((i < 0) || (i >= size[0]) || (j < 0) || (j >= size[1]))
    {
    return false;
    }
  else
    {
    return true;
    }
}

bool ImageGraphCut::IsValidPixel(int p[2])
{
  return IsValidPixel(p[0], p[1]);
}

double ImageGraphCut::ComputeNoise()
{
  int size[3];
  this->Image->GetDimensions(size);

  //vtkDataArray *scalar = this->Image->GetPointData()->GetScalars();
  double sigma = 0.0;

  int sum = 0;
  for(int i = 0; i < size[0]; i++)
    {
    for(int j = 0; j < size[1]; j++)
      {
      unsigned char* currentPixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i,j,0));

      // Left node
      if(this->IsValidPixel(i-1,j))
	{
	unsigned char* pixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i-1,j,0));
	sigma += pow(pixel[0]-currentPixel[0],2);
	sum++;
	}

      // Right node
      if(this->IsValidPixel(i+1,j))
	{
	unsigned char* pixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i+1,j,0));
	sigma += pow(pixel[0]-currentPixel[0],2);
	sum++;
	}

      // Lower node
      if(this->IsValidPixel(i,j-1))
	{
	unsigned char* pixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i,j-1,0));
	sigma += pow(pixel[0]-currentPixel[0],2);
	sum++;
	}

      // Upper node
      if(this->IsValidPixel(i,j+1))
	{
	unsigned char* pixel = static_cast<unsigned char*>(this->Image->GetScalarPointer(i,j+1,0));
	sigma += pow(pixel[0]-currentPixel[0],2);
	sum++;
	}
      }
    }

  sigma /= sum;

  return sigma;
}

void ImageGraphCut::CutGraph()
{
  // Compute max-flow
  //int flow = this->Graph->maxflow();
  this->Graph->maxflow();

  int extent[6];
  this->Image->GetExtent(extent);

  for(int i = extent[0]; i <= extent[1]; i++)
    {
    for(int j = extent[2]; j <= extent[3]; j++)
      {
      int p[3] = {i,j,0};
      vtkIdType currentIndex = this->Image->ComputePointId(p);

      // Current mask value
      unsigned char* pixel = static_cast<unsigned char*>(this->SourceSinkMask->GetScalarPointer(i,j,0));
      if(pixel[0] == ALWAYSSINK)
	{
	continue;
	}

      if(this->Graph->what_segment(currentIndex) == GraphType::SOURCE)
	{
	//std::cout << "pixel " << i << " " << j << " is a source!" << std::endl;
	pixel[0] = SOURCE;
	}
      else if(this->Graph->what_segment(currentIndex) == GraphType::SINK)
	{
	pixel[0] = SINK;
	}
      else
	{
	std::cerr << "Pixel " << i << " not labeled. This should never happen!" << std::endl;
	exit(-1);
	}
      }
    }

  delete this->Graph;
}

double ImageGraphCut::ColorDistance(int a[2], int b[2])
{
  // a and b are pixel locations
  unsigned char* pixelA = static_cast<unsigned char*>(this->Image->GetScalarPointer(a[0],a[1],0));
  unsigned char* pixelB = static_cast<unsigned char*>(this->Image->GetScalarPointer(b[0],b[1],0));

  return pow(pixelA[0]-pixelB[0],2) + pow(pixelA[1]-pixelB[1],2);
}

bool ImageGraphCut::IsNaN(const double a)
{
  if(a!=a)
  {
    return true;
  }
  return false;
}