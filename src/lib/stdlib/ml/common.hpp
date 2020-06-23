#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <mles/mles.hpp>

// Dataset
void clear_dataset();
int createDataset(unsigned int inputSize, unsigned int outputSize);
int cloneDataset(mles::DataSet &old);
mles::DataSet *findDataset(int id);

// NN
void clear_nn();
mles::NN *findNN(int id);
int createNN(unsigned int inputSize, unsigned int outputSize);

#endif
