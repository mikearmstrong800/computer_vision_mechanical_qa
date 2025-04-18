//----------------------------------------------------------------------------------------------
//The spinInterface class has the functions necessary to get a Mat image from a PGR camera
//They are:
//initPGRCamera() - finds and initialozes the first PGR camera found attached to the computer
//acquireImage()  - returns a pointer to a Mat that contains a single image
//----------------------------------------------------------------------------------------------


#include "spininterface.h"

/*!
 * The SpinInterface class interfaces with a Point Grey (Flir) Camera.
 */
SpinInterface::SpinInterface()
{
    calIsLoaded = false;
    cameraIsInitialized = false;
/*
    cap.open(0); //comment this out for production
    if(cap.isOpened())
    {
      calIsLoaded = true;
      cameraIsInitialized = true;
    }
*/
}

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
SpinInterface::~SpinInterface()
{
  //pCam->EndAcquisition();
  pCam = nullptr;
  camList.Clear();
  systemSpin->ReleaseInstance();

  cap.release();    //comment this out for production

}



/*!Acquire image from file imageFile
 *
*/
SpinInterface::t_acquireImageReturnStruct* SpinInterface::acquireImage(std::string imageFile)
{
    static t_acquireImageReturnStruct AIRS;
    //cv::Mat imgTmp;
    //cv::Size size(1288,964);
    AIRS.imageRaw = cv::imread(imageFile, cv::IMREAD_GRAYSCALE);
    if(AIRS.imageRaw.empty())
    {
        AIRS.error = -99;
        return &AIRS;
    }
    //cv::resize(imageGrabbed, imgTmp, size);
    //imgTmp.copyTo(imageGrabbed);
    //cv::imshow("test", imageGrabbed);     //diagnostic
    //cv::waitKey(0);                       //diagnostic
    return &AIRS;
}





/*!--------------------------------------------------------------------------------------------------------
 *
 *  For development with webcam - use the FLIR version below for production
 *
 *
 *
 --------------------------------------------------------------------------------------------------------*/
/*
SpinInterface::t_acquireImageReturnStruct*  SpinInterface::acquireImage()
{
  static cv::Mat imageGrabbed, imageGray, imageResized;
  static t_acquireImageReturnStruct AIRS;

  cap.read(imageGrabbed);
  if(!imageGrabbed.empty())
  {
    cvtColor(imageGrabbed,imageGray,cv::COLOR_RGB2GRAY);
    imageGray.copyTo(imageResized);
    resize(imageResized, imageResized, cv::Size(2448,2048));
    AIRS.imageRaw = imageResized;
  }

  return &AIRS;

}
*/





/*!Acquires one image from the camera
 *
 * The camera must have been previously intialized with initPGRCamera()
 *
 * Parameters: none
 * Return: a pointer to the cv::Mat image
*/
SpinInterface::t_acquireImageReturnStruct*  SpinInterface::acquireImage()
{
  static cv::Mat imageGrabbed;
  static t_acquireImageReturnStruct AIRS;

  if(!cameraIsInitialized)
  {
    AIRS.error = -1;
    return &AIRS;
  }

  pCam->BeginAcquisition();

  Spinnaker::ImagePtr pResultImage = pCam->GetNextImage();
  if(!pResultImage.IsValid())
  {
    AIRS.error = -2;
    return &AIRS;
  }

  if (pResultImage->IsIncomplete())
  {
    AIRS.error = -3;
    return &AIRS;
  }
  else
  {
    Spinnaker::ImagePtr convertedImage = pResultImage->Convert(Spinnaker::PixelFormat_Mono8, Spinnaker::HQ_LINEAR);

    size_t XPadding = convertedImage->GetXPadding();
    size_t YPadding = convertedImage->GetYPadding();
    size_t rowsize = convertedImage->GetWidth();
    size_t colsize = convertedImage->GetHeight();

    cv::Mat cvImg = cv::Mat(colsize + YPadding, rowsize + XPadding, CV_8UC1, convertedImage->GetData(), convertedImage->GetStride());
    cvImg.copyTo(imageGrabbed);
    pResultImage->Release();
    pCam->EndAcquisition();
  }

    //cv::imshow("test", imageGrabbed);     //diagnostic
    //cv::waitKey(0);                       //diagnostic
    AIRS.imageRaw = imageGrabbed;
    return &AIRS;
}






/*!Finds and initializes the first Point Grey camera attached to the system
 *
 * Parameters: none
 * Return: integer value 1 if there were no errors with initialization
*/
int SpinInterface::initPGRCamera()
{
    //TODO figure out logging or how to suppress log errors
    //Spinnaker::GenICam::gcstring logConfig = Spinnaker::GenICam::GetGenICamLogConfig();
    //Spinnaker::GenICam::SetGenICamLogConfig("C:\\Program Files\\FLIR Systems\\Spinnaker\\log\\config\\log4cpp.spinnaker.property");
    //Spinnaker::GenICam::gcstring logConfig = Spinnaker::GenICam::GetGenICamLogConfig();
    cameraIsInitialized = false;
    systemSpin = Spinnaker::System::GetInstance();
    camList = systemSpin->GetCameras();
    unsigned int numCameras = camList.GetSize();
    pCam = NULL;
    if(numCameras >0)
    {
      pCam = camList.GetByIndex(0);
      pCam->Init();
      nodeMap = &pCam->GetNodeMap();
    }
    else
    {
      cameraIsInitialized = false;
      return -1 ;
    }

    try
    {


      Spinnaker::GenICam::gcstring deviceVendorName = pCam->TLDevice.DeviceVendorName.GetValue();
      Spinnaker::GenICam::gcstring deviceModelName = pCam->TLDevice.DeviceModelName.GetValue();

      Spinnaker::GenICam::gcstring camID = pCam->GetUniqueID();
      //TODO make the cal file name the same as the serial number returned by GetUniqueID and check that they match

      pCam->AcquisitionMode.SetValue(Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame);

      double fr = pCam->AcquisitionFrameRate.GetValue();

      pCam->DeviceLinkThroughputLimit.SetValue(75360000);   //equates to 15 frames/sec on Chameleon3
      //pCam->DeviceLinkThroughputLimit.SetValue(25120000);   //equates to 5 frames/sec on Chameleon3

      fr = pCam->AcquisitionFrameRate.GetValue();

      //pCam->AcquisitionFrameRate.SetValue(15.0f);

      //auto exposure and auto gain on
      pCam->ExposureAuto.SetValue(Spinnaker::ExposureAutoEnums::ExposureAuto_Continuous);
      pCam->GainAuto.SetValue(Spinnaker::GainAutoEnums::GainAuto_Continuous);
    }
    catch (Spinnaker::Exception& e)
    {
      qDebug() << e.what()<<" <<<<<< spinInterface Failure";
      cameraIsInitialized = false;
      return -2 ;
    }

  cameraIsInitialized = true;
  return 1;
}

/*!
     * Sets the PGR camera frame rate and returns the framerate.
     * \param limit
     * \return frame rate
     */
    int SpinInterface::setPGRCameraThroughputLimit(int64 limit)
    {
      pCam->DeviceLinkThroughputLimit.SetValue(limit);   //equates to 15 frames/sec on Chameleon3
      double fr = pCam->AcquisitionFrameRate.GetValue();

      return fr;
    }


/*!loads the camera calibration data from file
 *
 * Parameter: calFileName - the file name of the OpenCV camera calibration that contains the
 * camera matrix and distortion coefficients.
 * These two matrices are loaded into class members cameraMatrix and distortionCoefficients.
 *
*/
int SpinInterface::loadCal(std::string calFileName)
{
    cv::FileStorage fs(calFileName, cv::FileStorage::READ); // Read the settings
    if (!fs.isOpened())
    {
      return -1;
    }

    fs["camera_matrix"] >> cameraMatrix;
    fs["distortion_coefficients"] >> distortionCoefficients;
    fs.release();
    calIsLoaded = true;
    return 1;
}



















