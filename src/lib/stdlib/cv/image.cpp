#include "common.hpp"
#include <opencv2/imgcodecs.hpp>

extern "C"
{
    int imcreate(int rows, int cols, int type, cube_native_var *s)
    {
        cv::Scalar scalar;
        if (s->is_list)
        {
            if (s->size == 1)
            {
                scalar[0] = AS_NATIVE_NUMBER(s->list[0]);
                scalar[1] = scalar[2] = scalar[0];
            }
            else if (s->size == 2)
            {
                scalar[0] = AS_NATIVE_NUMBER(s->list[0]);
                scalar[1] = AS_NATIVE_NUMBER(s->list[1]);
                scalar[2] = scalar[0];
            }
            else if (s->size == 3)
            {
                scalar[0] = AS_NATIVE_NUMBER(s->list[0]);
                scalar[1] = AS_NATIVE_NUMBER(s->list[1]);
                scalar[2] = AS_NATIVE_NUMBER(s->list[2]);
            }
            else if (s->size == 4)
            {
                scalar[0] = AS_NATIVE_NUMBER(s->list[0]);
                scalar[1] = AS_NATIVE_NUMBER(s->list[1]);
                scalar[2] = AS_NATIVE_NUMBER(s->list[2]);
                scalar[3] = AS_NATIVE_NUMBER(s->list[3]);
            }
        }
        cv::Mat img(rows, cols, type, scalar);
        return addImage(img);
    }

    int imread(const char *filename, int flags)
    {
        cv::Mat img = cv::imread(filename, flags);
        return addImage(img);
    }

    bool imwrite(const char *filename, int id)
    {
        cv::Mat &img = getImage(id);
        if (img.empty())
            return false;
        return cv::imwrite(filename, img);
    }

    cube_native_var *props(int id)
    {
        cube_native_var *dict = NATIVE_DICT();
        cv::Mat &img = getImage(id);
        ADD_NATIVE_DICT(dict, COPY_STR("cols"), NATIVE_NUMBER(img.cols));
        ADD_NATIVE_DICT(dict, COPY_STR("rows"), NATIVE_NUMBER(img.rows));
        ADD_NATIVE_DICT(dict, COPY_STR("dims"), NATIVE_NUMBER(img.dims));
        ADD_NATIVE_DICT(dict, COPY_STR("flags"), NATIVE_NUMBER(img.flags));
        return dict;
    }

    unsigned char *getData(int id)
    {
        cv::Mat &img = getImage(id);
        return img.data;
    }
}