#ifndef FIELDDETECT_H
#define FIELDDETECT_H

#include <opencv2/opencv.hpp>
#include <spininterface.h>
#include <QtCore>

/*!class definition*/
class FieldDetect : public QObject
{
    Q_OBJECT

public:
    FieldDetect();
    ~FieldDetect();

    /*!structrue returned by private function findBoardPose().  It's also included in the referenceStructure so the info can be used in findArucoDisplacement() */
    struct BoardPose{
        /*!rotation vectors*/
        cv::Vec3d rvec;
        /*!translation vectors*/
        cv::Vec3d tvec;
        /*!vector length of translation vectors*/
        double translationLength;
        /*!Display image with refererence frame markers*/
        cv::Mat imageDisplay;
    };

    /*!definition of t_boardPose*/
    typedef  BoardPose t_boardPose;


    /*!reference structure filled in by acquireReferenceImage()*/
    struct ReferenceStruct{
    /*!homogeneous transformation matrix*/
    cv::Mat H;
    /*!four outer marker points detected by acquireReferenceImage()*/
    std::vector<cv::Point2d> markerPoints;
    /*!aruco grid center point of reference image*/
    cv::Point2d arucoCenterReference;
    /*!a valid transformation has previously been set*/
    bool referenceSet;
    /*!camera was successfully initialized*/
    bool cameraInitialized;
    /*!blur file path*/
    std::string savePathToBlurFile;
    /*!camera cal was loaded from file*/
    bool cameraCalLoaded;
    /*!crossHair Center Point of reference image*/
    cv::Point2d crosshairCenterReference;
    /*!crosshair vertical line (two points)*/
    std::vector<cv::Point2d> crosshairVertPointsReference;
    /*!crosshair horizontal line (two points)*/
    std::vector<cv::Point2d> crosshairHorzPointsReference;
    /*!reference image undistorted and flat (perspective transformed)*/
    cv::Mat imageReferenceFlat;
    /*!reference image undistorted*/
    cv::Mat imageReferenceUndistorted;
    /*!cv::Point3d that contains x and y displacement and z absolute distance based on aruco marker size - same info returned by findArucoDisplacement*/
    cv::Point3d referencePosition;
    /*!board pose of a board composed of markers 3 and 4 of the Aruco marker board*/
    t_boardPose boardPose;
    };


    /*!definition of the reference structure type*/
    typedef ReferenceStruct t_referenceStruct;

    /*!declaration of reference structure*/
    t_referenceStruct referenceStruct;

    /*!structure filled in by findLightFieldDimensions() function*/
    struct FindLightFieldReturnStruct{
        /*!two edge points per side of the field, or eight points total - the top right point (facing gantry) is 0 and they are numbered ccw around the field*/
        std::vector<cv::Point2d> edgePoints;
        /*!vertical length of the field acrss the first set of points*/
        double v1Len;
        /*!vertical length of the field acrss the second set of points*/
        double v2Len;
        /*!horizontal length of the field acrss the first set of points*/
        double h1Len;
        /*!horizontal length of the field acrss the second set of points*/
        double h2Len;
        /*!light field center point*/
        cv::Point2d lfCenterPoint;
        /*!top point of the vertical crosshair*/
        cv::Point2d chV0;
        /*!bottom point of the vertical crosshair*/
        cv::Point2d chV1;
        /*!left point of the horizontal crosshair*/
        cv::Point2d chH0;
        /*!right point of the horizontal crosshair*/
        cv::Point2d chH1;
        /*!crosshair center point*/
        cv::Point2d chCenterPoint;
        /*!the undistorted, unwarped (transformed) image*/
        cv::Mat imageCorrected;
        /*!error*/
        int error;
    };

    /*!definition of FindLightFieldReturnStructure type*/
    typedef  FindLightFieldReturnStruct t_FindLightFieldReturnStruct;


    /*!definition of walkout structure - walkout information emitted from findWalkout()*/
    struct WOStruct{
        /*!sample number*/
        double sampleNumber;
        /*!collimator angle*/
        double angle;
        /*!walkout*/
        double vecLen;
        /*!coordinate of walkout point*/
        cv::Point2d woPt;
    };
    /*!definition of walkout struct type*/
    typedef WOStruct t_woStruct;

    /*!definition of return structure of findWalkout()*/
    struct FindWalkoutReturnStruct{
        /*!error code*/
        int error;
        /*!number of frames taken that are within the 90-270 range of rotation*/
        int numberOfFrames;
        /*!actual walkout data*/
        std::vector<WOStruct> walkoutStruct;
        /*!vector that contains the walkout vector length at 90, 0 and 180 degree collimator points*/
        std::vector<cv::Point2d> cardinalPointsVect;
        /*!the minimum vector length of walkout*/
        double minWOVectorLength;
        /*!the maximum vector length of walkout*/
        double maxWOVectorLength;
    };

    /*!definition of walkout structure type*/
    typedef FindWalkoutReturnStruct t_findWalkoutRetStruct;

    /*!structure returned by findArucoDisplacement()*/
    struct FindArucoDisplacementReturnStruct{
        /*!error code - see findArucoDisplacement docs*/
        int error;
        /*!X and Y are lat and long displacement.  Z is absolute vert*/
        cv::Point3d disp;
        /*!image*/
        cv::Mat imageDisplay;
    };

    /*!definition of t_findArucoDisplacementReturnStruct type*/
    typedef FindArucoDisplacementReturnStruct t_findArucoDisplacementReturnStruct;

    /*!structure returned by findODIDistance()*/
    struct FindODIDistanceReturnStruct{
        /*!error code - see findODIDistance function docs*/
        int error;
        /*!odi distance*/
        double distance;
        /*!image with roi and markers where the tick marks are detected*/
        cv::Mat imageDisplay;
    };
    /*!definition of t_findODIDistanceReturnStruct*/
    typedef FindODIDistanceReturnStruct t_findODIDistanceReturnStruct;

    /*!structure returned by findBoardAngle()*/
    struct FindBoardAngleReturnStructure{
        /*!board angle*/
        double angle;
        /*!error code*/
        int error;
        /*!display image*/
        cv::Mat imageDisplay;
    };

    /*!definition of t_findBoardAngleReturnStructure*/
    typedef FindBoardAngleReturnStructure t_findBoardAngleReturnStructure;


    int initializeCamera();
    int loadConfigFromFile(std::string configFileName);
    double warmUpCamera();
    int acquireReferenceImage();
    int acquireReferenceImage(std::string imageFileName);
    t_FindLightFieldReturnStruct findLightFieldDimensions(std::string imageFileName);
    t_findArucoDisplacementReturnStruct findArucoDisplacement();
    t_findArucoDisplacementReturnStruct findArucoDisplacement(cv::Mat *imageUndistorted);
    t_findWalkoutRetStruct findWalkout(bool isTableWalkOut);
    t_findWalkoutRetStruct findWalkout(std::string videoFileName);
    void findGantryWalkout(std::string videoFileName);
    t_findODIDistanceReturnStruct findODIDistance();
    cv::Mat* getImage();
    cv::Mat* getImage(std::string imageFileName);
    cv::Mat* getImage(cv::Mat *image);
    t_findBoardAngleReturnStructure findBoardAngle();
    double findBoardAngle(std::string imageFileName);
    double findBoardAngle(cv::Mat* image);

    int readConfigXML(std::string filename);

    int lightLevelLow;
    int lightLevelHigh;


    //int checkCameraCalibration();
    //float checkBlurriness();
    t_findODIDistanceReturnStruct findODI();

    cv::aruco::Dictionary dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);

signals:
    /*!signal emitted from findWalkout() function*/
    void walkoutDataEmitter(WOStruct* woData);



//-------------------------------------------------------------------------------------
private:

    struct findLightFieldRefinedCornersReturnStruct{
        std::vector<cv::Point> corners;
        cv::Mat imageFlat;
        int error;
    };

    typedef findLightFieldRefinedCornersReturnStruct t_findLightFieldRefinedCornersReturnStruct;


    struct PolyReturnStruct{
        /*!valid field has four points*/
        bool validField;
        /*!corner points of the field*/
        std::vector<cv::Point> pts;

    };

    typedef  PolyReturnStruct t_polyReturnStruct;

    struct roiStruct_t{
        cv::Rect roiRect0;
        cv::Rect roiRect1;
        cv::Mat     roiMat0;
        cv::Mat     roiMat1;
    };

    struct APRetStrut{
        /*!transformed image*/
        cv::Mat imageFlat;
        /*!16 points of the aruco marker corners*/
        std::vector<std::vector<cv::Point2f>> arucoCorners;
        /*!marker id's*/
        std::vector<int> arucoIds;
        /*!true if all four markers were detected*/
        int error;

    };

    typedef APRetStrut t_apRetStruct;


    t_FindLightFieldReturnStruct findLightFieldDimensions(cv::Mat* image);
    FieldDetect::t_polyReturnStruct findLightField(cv::Mat *imageIn);
    t_findLightFieldRefinedCornersReturnStruct findLightFieldRefinedCorners(cv::Mat* imageUndistorted);
    t_findLightFieldRefinedCornersReturnStruct findLightFieldRefinedCorners(std::string imageFileName);
    t_apRetStruct* arucoPerspective(cv::Mat *img, bool calcTransform);
    cv::Mat* cannyThreshold(cv::Mat *imageRaw);
    double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0);
    cv::Point2d rotatePoint(cv::Point2d pt, cv::Point2d around, double angle);
    void rotateImage(const cv::Mat &input, cv::Mat &output, double alpha, double beta, double gamma, double dx, double dy, double dz, double f);
    std::vector<cv::Point> sortPointsAngle(std::vector<cv::Point> points);
    std::vector<cv::Point> sortPoints(std::vector<cv::Point> points, std::vector<cv::Point> corners);
    std::vector<cv::Point2d> processCrosshairMat(cv::Mat imageCrosshair, double angle);
    std::vector<cv::Point2d> calcROIPoints(cv::Point2d p1, cv::Point2d p2, double percentFromEnd);
    std::vector<cv::Point2d> rotateAndFindMedian(cv::Mat roi, double angle, int index);
    void writeCSV(std::string filename, cv::Mat m);
    const double lfSquare = 55; //this is the light field ROI size squared
    const double chSquare = 55; //this is the crosshair ROI size squared
    cv::aruco::DetectorParameters params = cv::aruco::DetectorParameters();

    std::vector<cv::Point2d> findMarkerPoints(cv::Mat *imageFlat);
    std::vector<cv::Point2d> findTickCenters(cv::Mat tickROI, bool isCrosshairROI);

    #define pdd std::pair<double, double>
    cv::Point2d lineLineIntersection(cv::Point2d A1, cv::Point2d B1, cv::Point2d C1, cv::Point2d D1);
    cv::Mat computeProjectionMatrix(cv::Mat camMat, cv::Vec3d rotVec, cv::Vec3d transVec);
    void computeC2MC1(const cv::Mat &R1, const cv::Mat &tvec1, const cv::Mat &R2, const cv::Mat &tvec2, cv::Mat &R_1to2, cv::Mat &tvec_1to2);
    t_boardPose findBoardPose(cv::Mat *imageUndistorted);
    cv::Rect2d findImageLocationInImage(cv::Mat image, cv::Mat imageTemplate );
    template<class T> void PolyFit(const T*X, const T*Y, int n,int m, T*C);
    double correlationCoefficient(double X[], double Y[], int n);
    int setCameraFrameRate(int frameRate);


    SpinInterface spin;


};

#endif // FIELDDETECT_H
