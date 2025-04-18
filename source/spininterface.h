#ifndef SPININTERFACE_H
#define SPININTERFACE_H

#include "Spinnaker.h"          // IWYU pragma: keep
#include "opencv2/opencv.hpp"   // IWYU pragma: keep

class SpinInterface
{
public:
    SpinInterface();
    ~SpinInterface();

    struct acquireImageReturnStruct{
        cv::Mat imageRaw;
        int error;
    };

    typedef acquireImageReturnStruct t_acquireImageReturnStruct;

    int initPGRCamera();
    int setPGRCameraThroughputLimit(int64 limit);
    t_acquireImageReturnStruct* acquireImage();
    cv::Mat* acquireImage(std::string imageFile);
    int loadCal(std::string calFileName);

    cv::Mat cameraMatrix;
    cv::Mat distortionCoefficients;

    bool calIsLoaded;
    bool cameraIsInitialized;



private:
    Spinnaker::SystemPtr systemSpin;
    Spinnaker::CameraPtr pCam;
    Spinnaker::CameraList camList;
    Spinnaker::GenApi::INodeMap* nodeMap;


};

#endif // SPININTERFACE_H


