#include "common.hpp"

int imageId = 0;
map<int, cv::Mat> images;
cv::Mat emptyImage;

int addImage(cv::Mat &mat)
{
    int id = imageId;
    images[imageId] = mat;
    imageId++;
    return id;
}

cv::Mat &getImage(int id)
{
    if (images.find(id) != images.end())
        return images[id];
    emptyImage = cv::Mat();
    return emptyImage;
}

extern "C"
{
    void cube_init()
    {
        images.clear();
    }

    void cube_release()
    {
        images.clear();
    }

    int absdiff(int src1, int src2, int dst)
    {

        cv::Mat &s1 = getImage(src1);
        cv::Mat &s2 = getImage(src2);
        cv::Mat &d = getImage(dst);
        bool create = d.empty();
        if (create)
            d = d.clone();
        cv::absdiff(s1, s2, d);
        if (create)
            dst = addImage(d);
        return dst;
    }

    int add(int src1, int src2, int dst, int mask, int dtype)
    {
        cv::Mat &s1 = getImage(src1);
        cv::Mat &s2 = getImage(src2);
        cv::Mat &d = getImage(dst);
        bool create = d.empty();
        if (create)
            d = d.clone();
        cv::Mat &m = getImage(mask);
        cv::add(s1, s2, d, m, dtype);
        if (create)
            dst = addImage(d);
        return dst;
    }

    int addWeighted(int src1, double alpha, int src2, double beta, double gamma, int dst, int dtype)
    {
        cv::Mat &s1 = getImage(src1);
        cv::Mat &s2 = getImage(src2);
        cv::Mat &d = getImage(dst);
        bool create = d.empty();
        if (create)
            d = d.clone();
        cv::addWeighted(s1, alpha, s2, beta, gamma, d, dtype);
        if (create)
            dst = addImage(d);
        return dst;
    }

    int batchDistance(int src1, int src2, int dist, int dtype, int nidx, int normType, int K, int mask, int update,
                      bool crosscheck)
    {
        cv::Mat &s1 = getImage(src1);
        cv::Mat &s2 = getImage(src2);
        cv::Mat &d = getImage(dist);
        bool create = d.empty();
        if (create)
            d = d.clone();
        cv::Mat &m = getImage(mask);
        cv::Mat &n = getImage(nidx);
        cv::batchDistance(s1, s2, d, dtype, n, normType, K, m, update, crosscheck);
        if (create)
            dist = addImage(d);
        return dist;
    }
}