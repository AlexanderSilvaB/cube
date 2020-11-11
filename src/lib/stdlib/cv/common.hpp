#ifndef _CUBE_CV_COMMON_HPP_
#define _CUBE_CV_COMMON_HPP_

#include <cube/cubeext.h>
#include <map>
#include <opencv2/opencv.hpp>

using namespace std;

typedef struct
{
    int id;
    int rows, cols, dims;
    int flags;
    unsigned char *data;
} __attribute__((__packed__)) CubeMat;

int addImage(cv::Mat &mat);
cv::Mat &getImage(int id);

#endif
