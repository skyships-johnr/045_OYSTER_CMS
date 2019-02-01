#ifndef __VECTOR_CLASS__
#define __VECTOR_CLASS__

#include <kanzi/kanzi.hpp>
using namespace kanzi;

#ifdef DEBUG

template <class T> class vector_class
{
public:

    T& operator[](size_t n)
    {
        if (n >= m_vector.size())
        {
            printf(">>>> vector out of range '[%d]' access, size = %d\n",
                (int)n, (int)m_vector.size());
            fflush(stdout);
            *((int *)NULL) = 0; // Force access violation
        }

        return m_vector[ n ];
    }

    T& at(size_t n)
    {
        if (n >= m_vector.size())
        {
            printf(">>>> vector out of range 'at(%d)' access, size = %d\n",
                (int)n, (int)m_vector.size());
            fflush(stdout);
            *((int *)NULL) = 0; // Force access violation
        }

        return m_vector[ n ];
    }

    size_t size()
    {
        return m_vector.size();
    }

    void clear()
    {
        m_vector.clear();
    }

    void push_front(const T &v)
    {
        m_vector.push_front(v);
    }

    void push_back(const T &v)
    {
        m_vector.push_back(v);
    }

protected:

    vector<T> m_vector;
};

#else

#define vector_class vector

#endif

#endif
