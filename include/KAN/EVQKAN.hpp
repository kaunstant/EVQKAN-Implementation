#pragma once

#include "QuantumActivation.hpp"

#include <cstddef>
#include <vector>

namespace KAN
{
class EVQKAN
{
public:
    EVQKAN(std::size_t hiddenCount = 3, std::size_t outputCount = 2);

    double forward(double input);
    TrainingResult train(
        const std::vector<double>& inputs,
        const std::vector<double>& targets,
        int iterations = 40,
        double learningRate = 0.06);

    std::size_t parameterCount() const;
    std::vector<double> parameterValues() const;

private:
    std::vector<QuantumActivation> hiddenActivations_;
    std::vector<QuantumActivation> outputActivations_;
    QuantumActivation skipActivation_;
    std::vector<double> hiddenWeights_;
    std::vector<double> outputWeights_;
    double hiddenBias_ = 0.0;
    double outputBias_ = 0.0;
    double skipWeight_ = 1.0;

    std::vector<double> parameters() const;
    void setParameters(const std::vector<double>& parameters);
};
}