#include "cvqa_cal.h"
#include "ui_cvqa_cal.h"

CVQA_Cal::CVQA_Cal(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CVQA_Cal)
{
    ui->setupUi(this);
    ui->pushButton_acquire_image->installEventFilter(this);
    setUpCVQACalGUI();
    ui->graphicsView->setScene(scene);
    scene->setSceneRect(0,0,2448,2048);
}





CVQA_Cal::~CVQA_Cal()
{
    delete ui;
}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 * -------------------------------------------------------------------------------------------*/
void CVQA_Cal::setUpCVQACalGUI()
{

    ui->pushButtonSaveCalibration->setEnabled(false);
    ui->pushButton_acquire_image->setEnabled(false);
    ui->pushButton_calibrate->setEnabled(false);
    ui->pushButton_verify->setEnabled(false);
    ui->pushButton_verify_back->setEnabled(false);
    ui->pushButton_verify_forward->setEnabled(false);


//Make ChArUco board - for development
 /*
    cv::Mat boardImage;
    charucoBoard->draw(cv::Size(1800, 1125), boardImage);
    imshow("charucoBoard", boardImage);
    imwrite("..//..//config//charuco.bmp", boardImage);
    waitKey(1);
*/

}





/*!-------------------------------------------------------------------------------------------
 * \brief CVQA_Cal::on_pushButton_start_live_video_clicked()
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
void CVQA_Cal::on_pushButton_start_live_video_clicked()
{
    QString result;
    QFont serifFont("Times", 8, QFont::Normal);

    result = initialize();
    if(result.first(6) == "FAILED")
    {
      ui->label_image_count->setFont(serifFont);
      ui->label_image_count->setText(result);
    }
    else
    {
      liveVideoFlag = true;
      ui->pushButton_acquire_image->setEnabled(true);
      showLiveVideo();
    }
}





/*!-------------------------------------------------------------------------------------------
 * \brief CVQA_Cal::initialize
 * \return
 *
 *
 *
 * -------------------------------------------------------------------------------------------*/
QString CVQA_Cal::initialize()
{
   QString result;
   QFont serifFont("Times", 8, QFont::Normal);
   int error = 0;

   error = spin.initPGRCamera();

   if(error < 0)
   {
     ui->label_image_count->setFont(serifFont);
     ui->label_image_count->setText("No Camera");
     return "FAILED initializing Camera";
   }

   if(error>=0)
   {
     cameraIsInitialized = true;
     spin.setPGRCameraThroughputLimit(25120000);  //set camera frame rate to 5fps
   }

   cameraIsInitialized = true;
   imageCount = 0;

   return result;
}





/*!-------------------------------------------------------------------------------------------
 * \brief CVQA_Cal::showLiveVideo
 * \return
 *
 *
 -------------------------------------------------------------------------------------------*/
int CVQA_Cal::showLiveVideo()
{
  SpinInterface::t_acquireImageReturnStruct* airs;
  cv::Mat imageWorking(Size(2048,2448), CV_8SC3);

  ui->graphicsView->centerOn(0, 0);

  while(liveVideoFlag)
  {

    airs = spin.acquireImage();
    QCoreApplication::processEvents();
    if(!airs->imageRaw.empty())
    {
      airs->imageRaw.copyTo(imageWorking);
      cv::putText(imageWorking, "Live", cv::Point2f(50,150), 0, 2, cv::Scalar(0,0,255),5);
      displayImage(imageWorking, true);
    }
  }
  scene->clear();


  return 0;
}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
bool  CVQA_Cal::eventFilter(QObject *watched, QEvent *event)
{
  if ((watched == ui->pushButton_acquire_image) && (event->type() == QEvent::MouseButtonPress))
  {
    auto ev = static_cast<QMouseEvent*>(event);

    if (ev->button() == Qt::LeftButton)
    {
        int error = acquireCharucoBoardImages();
        qDebug()<<"acquireAndSave spin.acquireImage() error " << error;
        ui->pushButton_calibrate->setEnabled(true);
    }

    if (ev->button() == Qt::RightButton)
    {
        QString dir = QFileDialog::getExistingDirectory(this, tr(""), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        loadCharucoBoardImages(dir);
        ui->pushButton_calibrate->setEnabled(true);
    }
  }
  return false;
}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
int CVQA_Cal::acquireCharucoBoardImages()
{

  Mat image, returnedImage;
  SpinInterface::t_acquireImageReturnStruct airs;

  airs = *spin.acquireImage();
  if(!airs.imageRaw.empty())
  {
    returnedImage = processCharucoBoardImage(airs.imageRaw);
    if(!returnedImage.empty())
    {
      imwrite(calImageDir.toStdString() + "cal_image_" + std::to_string(QDateTime::currentMSecsSinceEpoch()) + ".png", airs.imageRaw);
      displayImage(returnedImage, true);
      //Sleep(250);
      QCoreApplication::processEvents();
      liveVideoFlag = true;

    }
    else
    {
      ui->label_image_count->setText("No image");
    }
  }



  return 0;
}





/*!-------------------------------------------------------------------------------------------
 *
 * Detects chessboard points and makes image and object points
 *
 *
 -------------------------------------------------------------------------------------------*/
Mat CVQA_Cal::processCharucoBoardImage(Mat distortedCharucoBoardImage)
{
  Mat displayImage, newCameraMatrix;

  if(!distortedCharucoBoardImage.empty())
  {
      distortedCharucoBoardImage.copyTo(displayImage);
      cvtColor(displayImage,displayImage,cv::COLOR_GRAY2RGB);

      vector< int > ids;
      vector< vector< Point2f > > corners, rejected;


      detectorParams->cornerRefinementMethod = cv::aruco::CORNER_REFINE_SUBPIX;
      aruco::detectMarkers(distortedCharucoBoardImage, makePtr<aruco::Dictionary>(dictionary), corners, ids, detectorParams, rejected);
      aruco::refineDetectedMarkers(distortedCharucoBoardImage, charucoBoard, corners, ids, rejected);



      Mat interpolatedCharucoCorners, interpolatedCharucoIds;

      if(ids.size() > 0)
      {
        aruco::interpolateCornersCharuco(corners, ids, distortedCharucoBoardImage, charucoBoard, interpolatedCharucoCorners, interpolatedCharucoIds);
        aruco::drawDetectedMarkers(displayImage, corners);

        if(interpolatedCharucoCorners.total() > 0)
          aruco::drawDetectedCornersCharuco(displayImage, interpolatedCharucoCorners, interpolatedCharucoIds);

        allCharucoCorners.push_back(interpolatedCharucoCorners);
        allCharucoIds.push_back(interpolatedCharucoIds);

        if(!calImagesWereLoadedFromFile)
          allImages.push_back(distortedCharucoBoardImage);

        imageCount++;

      }


        QFont serifFont("Times", 12, QFont::Normal);
        ui->label_image_count->setFont(serifFont);
        ui->label_image_count->setText(QString::number(imageCount));

        liveVideoFlag = true;
        return displayImage;

    }
  //Mat noImage(Size(2048,2448), CV_8UC3);
  //putText(noImage, "No Chessboard" , cv::Point2i((noImage.cols/2)-500, noImage.rows/2), 0, 4, cv::Scalar(255,0,0),5);

  //return noImage;
  liveVideoFlag = true;
  return Mat();
}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
void CVQA_Cal::on_pushButton_calibrate_clicked()
{
    liveVideoFlag = false;
    calibrate();
    ui->pushButtonSaveCalibration->setEnabled(true);
    ui->pushButton_verify->setEnabled(true);
    ui->pushButton_verify_back->setEnabled(true);
    ui->pushButton_verify_forward->setEnabled(true);

}





/*!-------------------------------------------------------------------------------------------
 * \brief CVQA_Cal::calibrate
 * \return
 *
 *
 -------------------------------------------------------------------------------------------*/
bool CVQA_Cal::calibrate()
{

    if(allCharucoCorners.size()<1 || allCharucoIds.size()<1) return false;
    liveVideoFlag = false;

    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    cameraMatrix.at<double>(0,0) = 1.f;    //were using fixed aspect ratio in calibrateCameraRO so we need to initialize fx
    distortionCoefficients = cv::Mat::zeros(8, 1, CV_64F);   //initialize distortion coeffs to zero
    //TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 );

    cameraMatrix.at<double>(0, 0) = intrinsicX;
    cameraMatrix.at<double>(1, 1) = intrinsicY;

    cameraMatrix.at<double>(0, 2) = intrinsicPrincipalPointX;
    cameraMatrix.at<double>(1, 2) = intrinsicPrincipalPointY;


    double rms = aruco::calibrateCameraCharuco(allCharucoCorners, allCharucoIds, charucoBoard, Size(2448,2048),
                                               cameraMatrix, distortionCoefficients, rvecs, tvecs ,
                                               stdDeviationsIntrinsics, stdDeviationsExtrinsics, perViewErrors,
                                               baseFlags + extraFlags,
                                               TermCriteria(TermCriteria::COUNT+TermCriteria::EPS, 30, DBL_EPSILON));


    //CALIB_FIX_ASPECT_RATIO + CALIB_FIX_PRINCIPAL_POINT + CALIB_ZERO_TANGENT_DIST + CALIB_USE_INTRINSIC_GUESS +CALIB_FIX_FOCAL_LENGTH

    qDebug()<<"per view errors " << perViewErrors;
    //qDebug()<<"std dev Intrinsics " << stdDeviationsIntrinsics;
    //qDebug()<<"std dev Extrisics " << stdDeviationsExtrinsics;

    RMSError = rms;

    ui->textEdit->append("RMS Error: " + QString::number(rms));


return true;

}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 *------------------------------------------------------------------------------------------- */
void CVQA_Cal::on_pushButton_verify_clicked()
{
    if(verificationInProgress)
    {
      verificationInProgress = false;
      currentVerifyImage--;
    }
    else if(!verificationInProgress) verificationInProgress = true;

    while(currentVerifyImage < allImages.size() && verificationInProgress)
    {
      qDebug()<<"verification in progress "<<verificationInProgress;
      qDebug()<<"current verification image "<<currentVerifyImage;


      verifyCalibration(currentVerifyImage);
      currentVerifyImage++;

    }



}





/*!----------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 ----------------------------------------------------------------------------------------------------------------*/
void CVQA_Cal::on_pushButton_verify_back_clicked()
{
  qDebug()<<"currentVerifyImage "<<currentVerifyImage;


  if(currentVerifyImage > 0)
  {
    qDebug()<<"verification in progress "<<verificationInProgress;
    qDebug()<<"current verification image "<<currentVerifyImage;

    currentVerifyImage --;
    verificationInProgress = true;
    verifyCalibration(currentVerifyImage);
    verificationInProgress = false;

  }
}





/*!----------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 ----------------------------------------------------------------------------------------------------------------*/
void CVQA_Cal::on_pushButton_verify_forward_clicked()
{
  if(currentVerifyImage <= allImages.size() -1)
  {
    currentVerifyImage ++;
    verificationInProgress = true;
    verifyCalibration(currentVerifyImage);
    verificationInProgress = false;

  }

}





/*!-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 *------------------------------------------------------------------------------------------- */
void CVQA_Cal::verifyCalibration(int imageNumber)
{

    cv::Mat imageUndistorted;
    cv::Mat imageResized;

    //qDebug()<<"allImages Size " << allImages.size();

        if(verificationInProgress)
        {
          //currentVerifyImage = imageNumber;
          //cv::imshow("verification image", images[i]);
          //cv::waitKey(0);
          //qDebug()<<"verification i: "<<imageNumber;

          cv::Mat imageOrigCopy;
          allImages[imageNumber].copyTo(imageOrigCopy);
          cv::putText(imageOrigCopy, "Original - " + std::to_string(imageNumber+1), cv::Point2i(50,150), 0, 2, cv::Scalar(255,255,255),5);
          displayImage(imageOrigCopy, true);
          QCoreApplication::processEvents();

          Sleep(500);

          cv::undistort(allImages[imageNumber], imageUndistorted, cameraMatrix, distortionCoefficients);

          //cv::imshow("undistorted", imageUndistorted);
          //cv::waitKey(1);

          cv::putText(imageUndistorted, "Undistorted - " + std::to_string(imageNumber+1)+"   Reprojection Error: " + std::to_string(perViewErrors[imageNumber]), cv::Point2i(50,150), 0, 2, cv::Scalar(255,255,255),5);
          displayImage(imageUndistorted, true);
          ui->textEdit->append(QString::number(imageNumber+1)+ " - " + QString::number( perViewErrors[imageNumber]));
          QCoreApplication::processEvents();

          Sleep(500);
      }




}





/*-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
void CVQA_Cal::on_pushButtonSaveCalibration_clicked()
{


    saveConfigXML(configDir.toStdString());
    ui->textEdit->setText("cal saved to " + configDir);

}





/*-------------------------------------------------------------------------------------------
 *
 *
 *
 *
 -------------------------------------------------------------------------------------------*/
void CVQA_Cal::saveConfigXML(std::string filename)
{

  cv::FileStorage fs(filename, cv::FileStorage::WRITE);

  QString qs;
  QDateTime now = QDateTime::currentDateTime();
  qs = now.toString();

  fs << "calibrationDate" << qs.toStdString();
  fs << "RMSError" << RMSError;
  fs << "camera_matrix" << cameraMatrix;
  fs << "distortion_coefficients" << distortionCoefficients;

  fs << "light_level_low_threshold" << 0;
  fs << "light_level_high_threshold" << 500;





  fs.release();


}





/*!----------------------------------------------------------------------------------------------------------------
 *
 *
 *  Converts an OpenCV Mat to a QImage suitable for use with the graphicsView and displays the image.
 *  If it's an 8 bit single channel image, it's converted to a color image
 *
 ----------------------------------------------------------------------------------------------------------------*/
void CVQA_Cal::displayImage(Mat image, bool resizeForDisplay)
{
  if(!image.empty())
  {
    if(image.type() == CV_8UC1)
      cv::cvtColor(image, image, cv::COLOR_GRAY2RGB);

    if(resizeForDisplay)
    {
      cv::resize(image, image, Size(ui->graphicsView->width(), ui->graphicsView->height()));
    }

    QPixmap pixmap =  QPixmap::fromImage(QImage(image.data, image.cols, image.rows, image.step, QImage::Format::Format_RGB888));
    scene->clear();
    scene->addPixmap(pixmap);


  }
}





/*!----------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *
 *
 ----------------------------------------------------------------------------------------------------------------*/
QString CVQA_Cal::loadCharucoBoardImages(QString directoryName)
{
    Mat imageDisplay, imageResized;
    QDirIterator it(directoryName, QDirIterator::Subdirectories);

    calImagesWereLoadedFromFile = true;
    allImages.clear();
    imageCount = 0;
    ui->textEdit->clear();

    while (it.hasNext())
    {
      QCoreApplication::processEvents();
      QString dir = it.next();
      QFileInfo fi(dir);
      QString fileName = fi.fileName();

      if(fileName != "." && fileName != "..")
      {
        Mat image = imread(dir.toStdString(), IMREAD_GRAYSCALE);
        if(!image.empty())
        {
          ui->textEdit->append(fileName);
          imageDisplay = processCharucoBoardImage(image); //image and object points are made here
          if(!imageDisplay.empty())
          {
            allImages.push_back(image);
            displayImage(imageDisplay, true);
          }
        }
      }
    }
  return QString();
}


















