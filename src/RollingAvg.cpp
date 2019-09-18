#include <RollingAvg.h>

RollingAvg::RollingAvg()
    : sum(0)
{

}

RollingAvg::~RollingAvg()
{

}

void RollingAvg::Push(const size_t& sample)
{
    if (data.size() >= APU_ROLLING_AVG_SAMPLE_SIZE)
    {
        sum -= data.front();
        data.pop();
    }
    data.push(sample);
    sum += sample;
}

size_t RollingAvg::GetRollingAvg()
{
    return sum / data.size();
}