#ifndef SRC_ROLLING_AVG_H
#define SRC_ROLLING_AVG_H

#include <queue>
#include <cstddef>

#define APU_ROLLING_AVG_SAMPLE_SIZE 20

class RollingAvg
{
public:
    RollingAvg();
    virtual ~RollingAvg();

    void Push(const size_t& sample);
    size_t GetRollingAvg();

private:
    std::queue<size_t> data;
    size_t sum;
};

#endif // SRC_ROLLING_AVG_H