#ifndef _RC_H_
#define _RC_H_

class RC
{
    private:
        int count;

    public:
        void AddRef()
        {
            count++;
        }

        int Release()
        {
            return --count;
        }
};

#endif