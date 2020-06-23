#include "common.hpp"
#include <cube/cubeext.h>
#include <mles/mles.hpp>

using namespace std;
using namespace mles;

vector<DataSet *> datasets;
DataSet *findDataset(int id)
{
    if (id >= 0 && id < datasets.size())
        return datasets[id];
    return NULL;
}

int createDataset(unsigned int inputSize, unsigned int outputSize)
{
    DataSet *dataset = new DataSet(inputSize, outputSize);
    datasets.push_back(dataset);
    return datasets.size() - 1;
}

int cloneDataset(DataSet &old)
{
    DataSet *dataset = new DataSet(old);
    datasets.push_back(dataset);
    return datasets.size() - 1;
}

void deleteDataset(int id)
{
    if (id >= 0 && id < datasets.size())
    {
        DataSet *dataset = findDataset(id);
        if (dataset != NULL)
            delete dataset;

        vector<DataSet *>::iterator it = datasets.begin();
        advance(it, id);
        datasets.erase(it);
    }
}

void clear_dataset()
{
    for (int i = 0; i < datasets.size(); i++)
        delete datasets[i];
    datasets.clear();
}

extern "C"
{
    EXPORTED int dataset_create(unsigned int in, unsigned int out)
    {
        return createDataset(in, out);
    }

    EXPORTED void dataset_remove(int id)
    {
        deleteDataset(id);
    }

    EXPORTED unsigned int dataset_input_size(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return 0;
        return ds->getInputSize();
    }

    EXPORTED unsigned int dataset_output_size(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return 0;
        return ds->getOutputSize();
    }

    EXPORTED void dataset_clear(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return;
        return ds->clear();
    }

    EXPORTED bool dataset_add(int id, cube_native_var *inList, cube_native_var *outList)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return false;

        if (outList == NULL || outList->type == TYPE_NULL)
        {
            Eigen::VectorXd entry = ds->createEntry();
            if (entry.size() != inList->size)
                return false;

            for (int i = 0; i < inList->size; i++)
                entry[i] = AS_NATIVE_NUMBER(inList->list[i]);

            ds->add(entry);
            return true;
        }

        Eigen::VectorXd in = ds->createInput();
        for (int i = 0; i < min((int)in.size(), inList->size); i++)
            in[i] = AS_NATIVE_NUMBER(inList->list[i]);

        Eigen::VectorXd out = ds->createOutput();
        for (int i = 0; i < min((int)out.size(), outList->size); i++)
            out[i] = AS_NATIVE_NUMBER(outList->list[i]);

        ds->add(in, out);
        return true;
    }

    EXPORTED bool dataset_add_entry(int id, cube_native_var *entryList)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return false;

        Eigen::VectorXd entry = ds->createEntry();
        for (int i = 0; i < min((int)entry.size(), entryList->size); i++)
            entry[i] = AS_NATIVE_NUMBER(entryList->list[i]);

        ds->add(entry);
        return true;
    }

    EXPORTED void dataset_print(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return;
        ds->print();
    }

    EXPORTED cube_native_var *dataset_create_input(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd in = ds->createInput();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < in.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(0));

        return list;
    }

    EXPORTED cube_native_var *dataset_create_output(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd out = ds->createOutput();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < out.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(0));

        return list;
    }

    EXPORTED cube_native_var *dataset_create_entry(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd entry = ds->createEntry();

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < entry.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(0));

        return list;
    }

    EXPORTED unsigned int dataset_size(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return 0;
        return ds->size();
    }

    EXPORTED cube_native_var *dataset_get_input(int id, unsigned int pos)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd in = ds->getInput(pos);

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < in.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(in[i]));

        return list;
    }

    EXPORTED cube_native_var *dataset_get_output(int id, unsigned int pos)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        Eigen::VectorXd out = ds->getOutput(pos);

        cube_native_var *list = NATIVE_LIST();
        for (int i = 0; i < out.size(); i++)
            ADD_NATIVE_LIST(list, NATIVE_NUMBER(out[i]));

        return list;
    }

    EXPORTED int dataset_to_testset(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return -1;

        DataSet data = ds->toTestSet();
        return cloneDataset(data);
    }

    EXPORTED int dataset_load(const char *fileName)
    {
        int id = createDataset(0, 0);
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return -1;
        if (ds->load(fileName) == false)
        {
            deleteDataset(id);
            return -1;
        }
        return id;
    }

    EXPORTED bool dataset_save(int id, const char *fileName)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return false;
        return ds->save(fileName);
    }

    EXPORTED int dataset_from_csv(const char *fileName, bool header, int chucksize)
    {
        int id = createDataset(0, 0);
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return -1;
        if (ds->fromCSV(fileName, header, chucksize) == false)
        {
            deleteDataset(id);
            return -1;
        }
        return id;
    }

    EXPORTED bool dataset_to_csv(int id, const char *fileName)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return false;
        return ds->toCSV(fileName);
    }

    EXPORTED void dataset_binarize(int id, double threshold)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return;
        ds->binarize(threshold);
    }

    EXPORTED void dataset_classify(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return;
        ds->classify();
    }

    EXPORTED void dataset_binarize_to_max(int id)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return;
        ds->binarizeToMax();
    }

    EXPORTED cube_native_var *dataset_split(int id, double ratio)
    {
        DataSet *ds = findDataset(id);
        if (ds == NULL)
            return NATIVE_NULL();

        int trainingSet = createDataset(0, 0);
        int testSet = createDataset(0, 0);

        DataSet *dsTrain = findDataset(trainingSet);
        if (dsTrain == NULL)
        {
            deleteDataset(trainingSet);
            deleteDataset(testSet);
            return NATIVE_NULL();
        }

        DataSet *dsTest = findDataset(testSet);
        if (dsTest == NULL)
        {
            deleteDataset(trainingSet);
            deleteDataset(testSet);
            return NATIVE_NULL();
        }

        ds->split(*dsTrain, *dsTest, ratio);

        cube_native_var *list = NATIVE_LIST();
        ADD_NATIVE_LIST(list, NATIVE_NUMBER(trainingSet));
        ADD_NATIVE_LIST(list, NATIVE_NUMBER(testSet));

        return list;
    }
}