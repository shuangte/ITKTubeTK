/*=========================================================================

Library:   TubeTK

Copyright 2012 Kitware Inc. 28 Corporate Drive,
Clifton Park, NY, 12065, USA.

All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=========================================================================*/

#include "tubeTestMain.h"

#include <iostream>

void RegisterTests( void )
{
  REGISTER_TEST( itkLabelMapToAcousticImpedanceImageFilterTest );
  REGISTER_TEST( itkAngleOfIncidenceImageFilterTest );
  REGISTER_TEST( itkGradientBasedAngleOfIncidenceImageFilterTest );
  REGISTER_TEST( itkAcousticImpulseResponseImageFilterTest );
  REGISTER_TEST( itkUltrasoundProbeGeometryCalculatorTest );
  REGISTER_TEST( itkUltrasoundProbeGeometryCalculatorTest2 );
  REGISTER_TEST( SyncRecordTest );
  REGISTER_TEST( itktubeInnerOpticToPlusImageReaderTest );
  REGISTER_TEST( itktubeMarkDuplicateFramesInvalidImageFilterTest );
}
