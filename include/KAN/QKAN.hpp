#pragma once

#include "QuantumActivation.hpp"

#include <cstddef>
#include <vector>

namespace KAN
{
class QKAN
{
public:
    explicit QKAN(std::size_t activationCount = 3);

    double forward(double input);
    TrainingResult train(
        const std::vector<double>& inputs,
        const std::vector<double>& targets,
        int iterations = 40,
        double learningRate = 0.08);

    std::size_t parameterCount() const;
    std::vector<double> parameterValues() const;

private:
    std::vector<QuantumActivation> activations_;
    std::vector<double> weights_;
    double bias_ = 0.0;

    std::vector<double> parameters() const;
    void setParameters(const std::vector<double>& parameters);
};
}