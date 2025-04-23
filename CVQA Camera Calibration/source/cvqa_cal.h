#ifndef CVQA_CAL_H
#define CVQA_CAL_H

#include <QMainWindow>
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QGraphicsScene>
#include <spininterface.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>
#include <fstream>
#include <QtCore>
#include <QMouseEvent>
#include <QGraphicsItemGroup>
#include <QGraphicsTextItem>
//#include <opencv2/aruco.hpp>          //delete - not needed for charuco
#include <opencv2/aruco/charuco.hpp>
#include <scene.h>
#include <sstream>

using namespace std;
using namespace cv;


QT_BEGIN_NAMESPACE
namespace Ui { class CVQA_Cal; }
QT_END_NAMESPACE

class CVQA_Cal : public QMainWindow
{
    Q_OBJECT

public:
    CVQA_Cal(QWidget *parent = nullptr);
    ~CVQA_Cal();

    SpinInterface spin;


    struct DetectedMarkers
    {
        size_t numberOfDetectedMarkers;
        std::vector<int> ids;
        std::vector<std::vector<cv::Point2f> > corners;
    };

    typedef DetectedMarkers t_DetectedMarkers;



    void setUpCVQACalGUI();


    QString initialize();
    bool  eventFilter(QObject *watched, QEvent *event);  //to determine whigh button pressed the acquire image button
    int acquireCharucoBoardImages();//get rid of this one
    Mat processCharucoBoardImage(Mat distortedCharucoBoardImage);
    bool calibrate();
    void verifyCalibration(int imageNumber);

    double RMSError;
    int baseFlags = CALIB_FIX_ASPECT_RATIO + CALIB_FIX_PRINCIPAL_POINT + CALIB_ZERO_TANGENT_DIST;
    int extraFlags = 0;

    double intrinsicX = 2.65e3;
    double intrinsicY = 2.65e3;
    double intrinsicPrincipalPointX = 2448/2;
    double intrinsicPrincipalPointY = 2048/2;

    int showLiveVideo();
    void saveConfigXML(std::string filename);
    QString loadCharucoBoardImages(QString directoryName);
    void displayImage(Mat image, bool resizeForDisplay);

    bool cameraIsInitialized;
    bool liveVideoFlag = false;
    bool calImagesWereLoadedFromFile = false;
    unsigned int imageCount = 0;
    unsigned int currentVerifyImage = 0;
    bool verificationInProgress = false;
    QString calImageDir = "..//..//calibration_images//";
    QString configDir = "..//..//config//cvqa_config.xml";


    /*
    std::vector<cv::Mat> images;
    std::vector<std::vector<cv::Point2f>> imagePoints;
    std::vector<std::vector<cv::Point3f>> objectPoints;
    */

    vector<Mat> allCharucoCorners;
    vector<Mat> allCharucoIds;
    vector<Mat> allImages;

    vector<double> stdDeviationsIntrinsics;
    vector<double> stdDeviationsExtrinsics;
    vector<double> perViewErrors;


    std::vector<double> reprojectionErrors;
    cv::Mat cameraMatrix;
    cv::Mat distortionCoefficients;




    cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_5X5_250);
    cv::Ptr<cv::aruco::CharucoBoard> charucoBoard = cv::aruco::CharucoBoard::create(14, 9, 20.0f, 15.0f, dictionary);

    cv::Ptr<cv::aruco::DetectorParameters> detectorParams = cv::aruco::DetectorParameters::create();


    std::vector<cv::Mat> rvecs, tvecs;


    Scene* scene = new(Scene);



private slots:

    void on_pushButton_start_live_video_clicked();
    void on_pushButton_verify_clicked();
    void on_pushButtonSaveCalibration_clicked();
    void on_pushButton_calibrate_clicked();




    void on_pushButton_verify_back_clicked();

    void on_pushButton_verify_forward_clicked();

private:
    Ui::CVQA_Cal *ui;



};
#endif // CVQA_CAL_H
