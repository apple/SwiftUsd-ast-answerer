#include <atomic>

template <class T>
class StaticTemplated {
private:
    mutable std::atomic<T *> _data;
};

extern StaticTemplated<int> staticInt;


struct StaticDouble {
private:
    mutable std::atomic<double*> _data;
};
