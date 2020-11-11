#include "common.hpp"

extern "C"
{
    int bilateralFilter(int src, int dst, int d, double sigmaColor, double sigmaSpace, int borderType)
    {

        cv::Mat &s = getImage(src);
        cv::Mat &ds = getImage(dst);
        bool create = ds.empty();
        if (create)
            ds = ds.clone();
        cv::bilateralFilter(s, ds, d, sigmaColor, sigmaSpace, borderType);
        if (create)
            dst = addImage(ds);
        return dst;
    }

    int blur(int src, int dst, int kw, int kh, int x, int y, int borderType)
    {
        if (kh == -1)
            kh = kw;

        cv::Mat &s = getImage(src);
        cv::Mat &ds = getImage(dst);
        bool create = ds.empty();
        if (create)
            ds = ds.clone();
        cv::blur(s, ds, cv::Size(kw, kh), cv::Point(x, y), borderType);
        if (create)
            dst = addImage(ds);
        return dst;
    }
}