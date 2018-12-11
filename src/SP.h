#ifndef _SP_H_
#define _SP_H_

#include "RC.h"
#include <iostream>

extern int sp_count;

template < typename T > class SP
{
    private:
        T*    pData;       // pointer
        RC* reference; // Reference count
        int id;

    public:
        SP() : pData(0), reference(0) 
        {
            // Create a new reference 
            reference = new RC();
            // Increment the reference count
            reference->AddRef();
            id = sp_count;
            sp_count++;
            std::cout << "Create: "  << id << " | " << pData << std::endl;            
        }

        SP(T* pValue) : pData(pValue), reference(0)
        {
            // Create a new reference 
            reference = new RC();
            // Increment the reference count
            reference->AddRef();
            id = sp_count;
            sp_count++;
            std::cout << "Create: "  << id << " | " << pData << std::endl;            
        }

        SP(const SP<T>& sp) : pData(sp.pData), reference(sp.reference)
        {
            // Copy constructor
            // Copy the data and reference pointer
            // and increment the reference count
            reference->AddRef();
        }

        ~SP()
        {
            // Destructor
            // Decrement the reference count
            // if reference become zero delete the data
            if(reference->Release() == 0)
            {
                delete pData;
                delete reference;
                std::cout << "Delete: "  << sp_count << std::endl;
                sp_count--;
            }
        }

        bool empty()
        {
            return pData == 0;
        }

        T& operator* ()
        {
            return *pData;
        }

        T* operator-> ()
        {
            std::cout << "Access: " << id << std::endl;
            return pData;
        }
        
        SP<T>& operator = (const SP<T>& sp)
        {
            // Assignment operator
            if (this != &sp) // Avoid self assignment
            {
                // Decrement the old reference count
                // if reference become zero delete the old data
                if(reference->Release() == 0)
                {
                    delete pData;
                    delete reference;
                    std::cout << "Delete: "  << id << std::endl;
                }

                // Copy the data and reference pointer
                // and increment the reference count
                pData = sp.pData;
                reference = sp.reference;
                reference->AddRef();

                std::cout << "Copy: "  << sp.id << " -> " << id << std::endl;            
            }
            return *this;
        }
};

#endif