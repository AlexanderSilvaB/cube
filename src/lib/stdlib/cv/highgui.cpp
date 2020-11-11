#include "common.hpp"

extern "C"
{
    // int createTrackbar(const char *trackbarname, const char *winname, int *value, int count, TrackbarCallback
    // onChange, void *userData);

    void destroyAllWindows()
    {
        cv::destroyAllWindows();
    }

    void destroyWindow(const char *winname)
    {
        cv::destroyWindow(winname);
    }

    int getMouseWheelDelta(int flags)
    {
        return cv::getMouseWheelDelta(flags);
    }

    int getTrackbarPos(const char *trackbarname, const char *winname)
    {
        return cv::getTrackbarPos(trackbarname, winname);
    }

    // Rect getWindowImageRect(const char *winname);
    double getWindowProperty(const char *winname, int prop_id)
    {
        return cv::getWindowProperty(winname, prop_id);
    }

    void imshow(const char *winname, int id)
    {
        cv::imshow(winname, getImage(id));
    }

    void moveWindow(const char *winname, int x, int y)
    {
        cv::moveWindow(winname, x, y);
    }

    void namedWindow(const char *winname, int flags)
    {
        cv::namedWindow(winname, flags);
    }

    void resizeWindow(const char *winname, int width, int height)
    {
        cv::resizeWindow(winname, width, height);
    }

    // selectROI
    // selectROI
    // selectROIs
    // setMouseCallback
    // setTrackbarMax
    // setTrackbarMin
    // setTrackbarPos

    void setWindowProperty(const char *winname, int prop_id, double prop_value)
    {
        cv::setWindowProperty(winname, prop_id, prop_value);
    }

    void setWindowTitle(const char *winname, const char *title)
    {
        cv::setWindowTitle(winname, title);
    }

    int startWindowThread()
    {
        return cv::startWindowThread();
    }

    int waitKey(int delay)
    {
        return cv::waitKey(delay);
    }

    int waitKeyEx(int delay)
    {
        return cv::waitKeyEx(delay);
    }
}