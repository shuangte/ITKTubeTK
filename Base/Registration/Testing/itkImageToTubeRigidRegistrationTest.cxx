/*=========================================================================

Library:   TubeTK

Copyright 2010 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 ( the "License" );
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/
#include "itkImageToTubeRigidRegistration.h"
#include "itkSpatialObjectReader.h"
#include "itkSpatialObjectWriter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkSpatialObjectToImageFilter.h"
#include "itkTubeToTubeTransformFilter.h"
#include "itkMath.h"
#include "itkRecursiveGaussianImageFilter.h"
#include "itkVesselTubeSpatialObject.h"
#include "itkSubSampleTubeTreeSpatialObjectFilter.h"

int itkImageToTubeRigidRegistrationTest(int argc, char* argv [] )
{

  if ( argc < 4 )
    {
    std::cerr << "Missing Parameters: "
              << argv[0]
              << " Input_Image "
              << " Input_Vessel"
              << " Output_Tubes"
              << " Output_Image"
              << std::endl;
    return EXIT_FAILURE;
    }
  const char * inputImage = argv[1];
  const char * inputVessel = argv[2];
  const char * outputTubes = argv[3];
  const char * outputImage = argv[4];

  static const unsigned int Dimension = 3;
  typedef double            FloatType;

  typedef itk::VesselTubeSpatialObject< Dimension >      TubeType;
  typedef itk::GroupSpatialObject< Dimension >           TubeNetType;
  typedef itk::SpatialObjectReader< Dimension >          TubeNetReaderType;
  typedef itk::Image< FloatType, Dimension >             ImageType;
  typedef itk::ImageFileReader< ImageType >              ImageReaderType;
  typedef itk::ImageFileWriter< ImageType >              ImageWriterType;
  typedef itk::ImageToTubeRigidRegistration< ImageType, TubeNetType, TubeType >
                                                         RegistrationFilterType;
  typedef RegistrationFilterType::TransformType          TransformType;
  typedef itk::TubeToTubeTransformFilter< TransformType, Dimension >
                                                         TubeTransformFilterType;

  // read image
  ImageReaderType::Pointer imageReader = ImageReaderType::New();
  imageReader->SetFileName( inputImage );

  // Guassian blur the original input image to increase the likelihood of vessel
  // spatial object overlapping with the vessel image at their initial alignment.
  // this enlarges the convergence zone.
  typedef itk::RecursiveGaussianImageFilter<ImageType, ImageType>
                                                           GaussianBlurFilterType;
  GaussianBlurFilterType::Pointer blurFilters[Dimension];
  for ( unsigned int ii = 0; ii < Dimension; ++ii )
    {
    blurFilters[ii] = GaussianBlurFilterType::New();
    blurFilters[ii]->SetSigma( 3.0 );
    blurFilters[ii]->SetZeroOrder();
    blurFilters[ii]->SetDirection( ii );
    }
  blurFilters[0]->SetInput( imageReader->GetOutput() );
  blurFilters[1]->SetInput( blurFilters[0]->GetOutput() );
  blurFilters[2]->SetInput( blurFilters[1]->GetOutput() );
  try
    {
    blurFilters[2]->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Exception caught: " << err << std::endl;
    return EXIT_FAILURE;
    }

  // read vessel
  TubeNetReaderType::Pointer vesselReader = TubeNetReaderType::New();
  vesselReader->SetFileName( inputVessel );
  try
    {
    vesselReader->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Exception caught: " << err << std::endl;
    return EXIT_FAILURE;
    }

  // subsample points in vessel
  typedef itk::SubSampleTubeTreeSpatialObjectFilter< TubeNetType, TubeType >
    SubSampleTubeNetFilterType;
  SubSampleTubeNetFilterType::Pointer subSampleTubeNetFilter =
    SubSampleTubeNetFilterType::New();
  subSampleTubeNetFilter->SetInput( vesselReader->GetGroup() );
  subSampleTubeNetFilter->SetSampling( 100 );
  try
    {
    subSampleTubeNetFilter->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Exception caught: " << err << std::endl;
    return EXIT_FAILURE;
    }

  double initialPosition[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  if (argc > 10)
    {
    for (unsigned int ii = 0; ii < 6; ++ii)
      {
      initialPosition[ii] = atof( argv[5+ii] );
      }
    }

  RegistrationFilterType::Pointer  registrationFilter =
    RegistrationFilterType::New();

  registrationFilter->SetFixedImage( blurFilters[2]->GetOutput() );
  registrationFilter->SetMovingSpatialObject( subSampleTubeNetFilter->GetOutput() );
  registrationFilter->SetNumberOfIteration( 1000 );
  registrationFilter->SetLearningRate( 0.1 );
  registrationFilter->SetInitialPosition( initialPosition );
  try
    {
    registrationFilter->Initialize();
    registrationFilter->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Exception caught: " << err << std::endl;
    return EXIT_FAILURE;
    }

  // validate the registration result
  //! \todo validate against known real results.
  TransformType::Pointer outputTransform =
    dynamic_cast<TransformType *>(registrationFilter->GetTransform());
  outputTransform->SetParameters( registrationFilter->GetLastTransformParameters() );

  TransformType::Pointer inverseTransform = TransformType::New();
  outputTransform->GetInverse( inverseTransform );

  std::cout << "Registration result: ";
  for (unsigned int ii = 0; ii < 6; ++ii)
    {
    std::cout << outputTransform->GetParameters().GetElement(ii) << " ";
    }
  std::cout << std::endl;

  itk::Matrix< double, Dimension, Dimension > rotationMatrix;
  itk::Vector< double, Dimension > translation;
  rotationMatrix = outputTransform->GetMatrix();
  translation = outputTransform->GetTranslation();

  std::cout << "Rotation matrix: " << std::endl;
  std::string indent( "    " );
  std::cout << indent << rotationMatrix(0,0) << " "
    << rotationMatrix(0,1) << " " << rotationMatrix(0,2) << std::endl;
  std::cout << indent << rotationMatrix(1,0) << " "
    << rotationMatrix(1,1) << " " << rotationMatrix(1,2) << std::endl;
  std::cout << indent << rotationMatrix(2,0) << " "
    << rotationMatrix(2,1) << " " << rotationMatrix(2,2) << std::endl;

  std::cout << "Translations: " << std::endl;
  std::cout << indent << translation[0] << " "
    << translation[1] << " " << translation[2] << std::endl;

  // Transform the input tubes.
  TubeTransformFilterType::Pointer transformFilter = TubeTransformFilterType::New();
  transformFilter->SetInput( vesselReader->GetGroup() );
  transformFilter->SetScale( 1.0 );
  transformFilter->SetTransform( outputTransform );
  std::cout << "Outputting transformed tubes...";
  try
    {
    transformFilter->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Exception caught: " << err << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << " done.\n";

  // Write the transformed tube to file.
  typedef itk::SpatialObjectWriter< Dimension > TubeNetWriterType;
  TubeNetWriterType::Pointer tubesWriter = TubeNetWriterType::New();
  tubesWriter->SetInput( transformFilter->GetOutput() );
  tubesWriter->SetFileName( outputTubes );
  try
    {
    tubesWriter->Update();
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "ExceptionObject: " << err << std::endl;
    return EXIT_FAILURE;
    }

  typedef itk::SpatialObjectToImageFilter<TubeNetType, ImageType>
                                              SpatialObjectToImageFilterType;
  SpatialObjectToImageFilterType::Pointer vesselToImageFilter =
    SpatialObjectToImageFilterType::New();

  ImageType::Pointer img = imageReader->GetOutput();

  const double decimationFactor = 4.0;
  ImageType::SizeType size = img->GetLargestPossibleRegion().GetSize();
  size[0] = size[0] / decimationFactor;
  size[1] = size[1] / decimationFactor;
  size[2] = size[2] / decimationFactor;

  ImageType::SpacingType spacing = img->GetSpacing();
  spacing[0] = spacing[0] * decimationFactor;
  spacing[1] = spacing[1] * decimationFactor;
  spacing[2] = spacing[2] * decimationFactor;

  std::cout << "Converting transformed vessel model into a binary image ... ";
  vesselToImageFilter->SetInput( transformFilter->GetOutput() );
  vesselToImageFilter->SetSize( size );
  vesselToImageFilter->SetSpacing( spacing );
  vesselToImageFilter->SetOrigin( img->GetOrigin() );
  vesselToImageFilter->SetInsideValue( 1.0 );
  vesselToImageFilter->SetOutsideValue( 0.0 );
  vesselToImageFilter->Update();
  std::cout << "done." << std::endl;

  std::cout << "Outputting tube sampled as an image ... ";
  ImageWriterType::Pointer imageWriter = ImageWriterType::New();
  imageWriter->SetFileName( outputImage );
  imageWriter->SetInput(vesselToImageFilter->GetOutput());
  imageWriter->Update();
  std::cout << "done." << std::endl;

  return EXIT_SUCCESS;
}
