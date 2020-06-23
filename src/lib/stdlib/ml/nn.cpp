#include "common.hpp"
#include <cube/cubeext.h>
#include <mles/mles.hpp>

using namespace std;
using namespace mles;

vector<NN *> nns;
NN *findNN(int id)
{
    if (id >= 0 && id < nns.size())
        return nns[id];
    return NULL;
}

int createNN(unsigned int inputSize, unsigned int outputSize)
{
    NN *nn = new NN(inputSize, outputSize);
    nns.push_back(nn);
    return nns.size() - 1;
}

void deleteNN(int id)
{
    if (id >= 0 && id < nns.size())
    {
        NN *nn = findNN(id);
        if (nn != NULL)
            delete nn;

        vector<NN *>::iterator it = nns.begin();
        advance(it, id);
        nns.erase(it);
    }
}

void clear_nn()
{
    for (int i = 0; i < nns.size(); i++)
        delete nns[i];
    nns.clear();
}

extern "C"
{
    EXPORTED int nn_create(unsigned int in, unsigned int out)
    {
        return createNN(in, out);
    }

    EXPORTED void nn_remove(int id)
    {
        deleteNN(id);
    }

    EXPORTED void nn_build(int id)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->build();
    }

    EXPORTED void nn_verbose(int id, bool verbose)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->verbose(verbose);
    }

    EXPORTED void nn_output_activation(int id, const char *name)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->setOutputActivation(name);
    }

    EXPORTED void nn_default_activation(int id, const char *name)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->setDefaultActivation(name);
    }

    EXPORTED int nn_load(const char *fileName)
    {
        int id = createNN(0, 0);
        NN *nn = findNN(id);
        if (nn == NULL)
            return -1;
        if (nn->load(fileName) == false)
        {
            deleteNN(id);
            return -1;
        }
        return id;
    }

    EXPORTED bool nn_save(int id, const char *fileName)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return false;
        return nn->save(fileName);
    }

    EXPORTED void nn_add_layer(int id, unsigned int size, const char *name)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->addLayer(size, name);
    }

    EXPORTED void nn_insert_layer(int id, unsigned int pos, unsigned int size, const char *name)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->insertLayer(pos, size, name);
    }

    EXPORTED void nn_change_layer(int id, unsigned int pos, unsigned int size, const char *name)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->changeLayer(pos, size, name);
    }

    EXPORTED void nn_remove_layer(int id, unsigned int pos)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->removeLayer(pos);
    }

    EXPORTED void nn_print(int id)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        nn->print();
    }

    EXPORTED cube_native_var *nn_create_input(int id)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd in = nn->createInputVector();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < in.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(0));

        return list;
    }

    EXPORTED cube_native_var *nn_create_output(int id)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd out = nn->createOutputVector();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < out.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(0));

        return list;
    }

    EXPORTED int nn_create_dataset(int id)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return -1;

        DataSet data = nn->createDataSet();
        int dsId = cloneDataset(data);

        return dsId;
    }

    EXPORTED cube_native_var *nn_test(int id, cube_native_var *var)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return NATIVE_NULL();

        if (!IS_NATIVE_LIST(var))
            return NATIVE_NULL();

        Eigen::VectorXd in = nn->createInputVector();
        if (in.size() != var->size)
            return NATIVE_NULL();

        for (int i = 0; i < in.size(); i++)
            in[i] = AS_NATIVE_NUMBER(var->list[i]);

        Eigen::VectorXd out = nn->test(in);

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < out.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(out[i]));

        return list;
    }

    EXPORTED bool nn_test_dataset(int id, int dataId)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return false;

        DataSet *data = findDataset(dataId);
        if (data == NULL)
            return false;

        nn->test(*data);
        return true;
    }

    EXPORTED TrainingResults nn_train(int id, int dataId, TrainingSettings settings)
    {
        TrainingResults results;
        results.error = 1.0f;
        results.epochs = 0;
        results.finished = true;
        results.elapsedTime = 0;
        results.errorCode = 2;

        NN *nn = findNN(id);
        if (nn == NULL)
            return results;

        DataSet *data = findDataset(dataId);
        if (data == NULL)
            return results;

        results = nn->train(*data, settings);
        return results;
    }
}