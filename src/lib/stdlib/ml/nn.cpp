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

TrainingSettings getTrainingSettings(cube_native_var *var)
{
    TrainingSettings settings;

    if (IS_NATIVE_DICT(var))
    {
        for (int i = 0; i < var->size; i++)
        {
            string key(var->dict[i]->key);
            if (key == "epochs")
                settings.epochs = AS_NATIVE_NUMBER(var->dict[i]);
            else if (key == "maxError")
                settings.maxError = AS_NATIVE_NUMBER(var->dict[i]);
            else if (key == "learningRate")
                settings.learningRate = AS_NATIVE_NUMBER(var->dict[i]);
            else if (key == "batch")
                settings.batch = AS_NATIVE_NUMBER(var->dict[i]);
            else if (key == "localMinimaLimit")
                settings.localMinimaLimit = AS_NATIVE_NUMBER(var->dict[i]);
            else if (key == "allowReset")
                settings.allowReset = AS_NATIVE_BOOL(var->dict[i]);
        }
    }

    return settings;
}

cube_native_var *getTrainingResults(TrainingResults results)
{
    cube_native_var *var = NATIVE_DICT();
    ADD_NATIVE_DICT(var, COPY_STR("error"), NATIVE_NUMBER(results.error));
    ADD_NATIVE_DICT(var, COPY_STR("elapsedTime"), NATIVE_NUMBER(results.elapsedTime));
    ADD_NATIVE_DICT(var, COPY_STR("errorCode"), NATIVE_NUMBER(results.errorCode));
    ADD_NATIVE_DICT(var, COPY_STR("epochs"), NATIVE_NUMBER(results.epochs));
    ADD_NATIVE_DICT(var, COPY_STR("finished"), NATIVE_BOOL(results.finished));
    return var;
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

    EXPORTED void nn_output_activation(int id, const char *name, cube_native_var *args)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        vector<double> params;
        if (IS_NATIVE_LIST(args))
        {
            for (int i = 0; i < args->size; i++)
                params.push_back(AS_NATIVE_NUMBER(args->list[i]));
        }
        nn->setOutputActivationVec(name, params);
    }

    EXPORTED void nn_default_activation(int id, const char *name, cube_native_var *args)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        vector<double> params;
        if (IS_NATIVE_LIST(args))
        {
            for (int i = 0; i < args->size; i++)
                params.push_back(AS_NATIVE_NUMBER(args->list[i]));
        }
        nn->setDefaultActivationVec(name, params);
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

    EXPORTED void nn_add_layer(int id, unsigned int size, const char *name, cube_native_var *args)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        vector<double> params;
        if (IS_NATIVE_LIST(args))
        {
            for (int i = 0; i < args->size; i++)
                params.push_back(AS_NATIVE_NUMBER(args->list[i]));
        }
        nn->addLayerVec(size, name, params);
    }

    EXPORTED void nn_insert_layer(int id, unsigned int pos, unsigned int size, const char *name, cube_native_var *args)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        vector<double> params;
        if (IS_NATIVE_LIST(args))
        {
            for (int i = 0; i < args->size; i++)
                params.push_back(AS_NATIVE_NUMBER(args->list[i]));
        }
        nn->insertLayerVec(pos, size, name, params);
    }

    EXPORTED void nn_change_layer(int id, unsigned int pos, unsigned int size, const char *name, cube_native_var *args)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;
        vector<double> params;
        if (IS_NATIVE_LIST(args))
        {
            for (int i = 0; i < args->size; i++)
                params.push_back(AS_NATIVE_NUMBER(args->list[i]));
        }
        nn->changeLayerVec(pos, size, name, params);
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

    EXPORTED void nn_set_input_transformation(int id, cube_native_var *aList, cube_native_var *bList)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;

        if (!IS_NATIVE_LIST(aList) || !IS_NATIVE_LIST(bList))
            return;

        Eigen::VectorXd a = nn->createInputVector();
        if (a.size() != aList->size || a.size() != bList->size)
            return;

        Eigen::VectorXd b = nn->createInputVector();

        for (int i = 0; i < a.size(); i++)
        {
            a[i] = AS_NATIVE_NUMBER(aList->list[i]);
            b[i] = AS_NATIVE_NUMBER(bList->list[i]);
        }

        nn->setInputTransformation(a, b);
    }

    EXPORTED void nn_set_output_transformation(int id, cube_native_var *aList, cube_native_var *bList)
    {
        NN *nn = findNN(id);
        if (nn == NULL)
            return;

        if (!IS_NATIVE_LIST(aList) || !IS_NATIVE_LIST(bList))
            return;

        Eigen::VectorXd a = nn->createOutputVector();
        if (a.size() != aList->size || a.size() != bList->size)
            return;

        Eigen::VectorXd b = nn->createOutputVector();

        for (int i = 0; i < a.size(); i++)
        {
            a[i] = AS_NATIVE_NUMBER(aList->list[i]);
            b[i] = AS_NATIVE_NUMBER(bList->list[i]);
        }

        nn->setOutputTransformation(a, b);
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

    EXPORTED cube_native_var *nn_train(int id, int dataId, cube_native_var *dictSettings)
    {
        TrainingResults results;
        results.error = 1.0f;
        results.epochs = 0;
        results.finished = true;
        results.elapsedTime = 0;
        results.errorCode = 2;

        NN *nn = findNN(id);
        if (nn == NULL)
            return getTrainingResults(results);

        DataSet *data = findDataset(dataId);
        if (data == NULL)
            return getTrainingResults(results);

        TrainingSettings settings = getTrainingSettings(dictSettings);
        results = nn->train(*data, settings);
        return getTrainingResults(results);
    }
}