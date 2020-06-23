#include "common.hpp"
#include <cube/cubeext.h>
#include <mles/mles.hpp>

using namespace std;
using namespace mles;

extern "C"
{
    EXPORTED void cube_release()
    {
        clear_dataset();
        clear_nn();
    }
}
