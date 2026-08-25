#pragma once

#include <QCS>

#include <cstddef>
#include <vector>

namespace KAN
{
struct TrainingResult
{
    double value = 0.0;
    int iterations = 0;
};

class QuantumActivation
{
public:
    explicit QuantumActivation(double inputScale = 1.0, double inputOffset = 0.0);

    double forward(double input);
    void setParameter(double parameter);
    double parameter() const;

private:
    QNN::QNN model_;
    double inputScale_;
    double inputOffset_;
    double parameter_ = 0.0;
};

double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets);
}