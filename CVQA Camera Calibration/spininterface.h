#ifndef SPININTERFACE_H
#define SPININTERFACE_H

#include <Spinnaker.h>
#include <opencv2/opencv.hpp>
#include <QString>
#include <QDebug>


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
    t_acquireImageReturnStruct* acquireImage(std::string imageFile);
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

    cv::VideoCapture cap;



};

#endif // SPININTERFACE_H


