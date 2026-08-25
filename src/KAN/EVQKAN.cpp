#include "KAN/EVQKAN.hpp"

#include <cstddef>

namespace KAN
{
namespace
{
double cost(EVQKAN& model, const std::vector<double>& inputs, const std::vector<double>& targets)
{
    std::vector<double> predictions;
    predictions.reserve(inputs.size());
    for (double input : inputs)
    {
        predictions.push_back(model.forward(input));
    }
    return meanSquaredError(predictions, targets);
}
}

EVQKAN::EVQKAN(std::size_t hiddenCount, std::size_t outputCount)
        : hiddenWeights_(hiddenCount, 0.0),
            outputWeights_(outputCount, 0.0)
{
    for (std::size_t index = 0; index < hiddenCount; ++index)
    {
        hiddenActivations_.emplace_back(static_cast<double>(index + 1), 0.0);
    }
    for (std::size_t index = 0; index < outputCount; ++index)
    {
        outputActivations_.emplace_back(1.0, 0.0);
    }
}

double EVQKAN::forward(double input)
{
    double hidden = hiddenBias_;
    for (std::size_t index = 0; index < hiddenActivations_.size(); ++index)
    {
        hidden += hiddenWeights_[index] * hiddenActivations_[index].forward(input);
    }

    double output = outputBias_ + skipWeight_ * skipActivation_.forward(input);
    for (std::size_t index = 0; index < outputActivations_.size(); ++index)
    {
        output += outputWeights_[index] * outputActivations_[index].forward(hidden);
    }
    return output;
}

std::vector<double> EVQKAN::parameters() const
{
    std::vector<double> result;
    for (const QuantumActivation& activation : hiddenActivations_)
    {
        result.push_back(activation.parameter());
    }
    result.insert(result.end(), hiddenWeights_.begin(), hiddenWeights_.end());
    result.push_back(hiddenBias_);
    for (const QuantumActivation& activation : outputActivations_)
    {
        result.push_back(activation.parameter());
    }
    result.insert(result.end(), outputWeights_.begin(), outputWeights_.end());
    result.push_back(outputBias_);
    result.push_back(skipWeight_);
    return result;
}

void EVQKAN::setParameters(const std::vector<double>& values)
{
    std::size_t offset = 0;
    for (QuantumActivation& activation : hiddenActivations_)
    {
        activation.setParameter(values[offset++]);
    }
    for (double& weight : hiddenWeights_)
    {
        weight = values[offset++];
    }
    hiddenBias_ = values[offset++];
    for (QuantumActivation& activation : outputActivations_)
    {
        activation.setParameter(values[offset++]);
    }
    for (double& weight : outputWeights_)
    {
        weight = values[offset++];
    }
    outputBias_ = values[offset++];
    skipWeight_ = values[offset];
}

TrainingResult EVQKAN::train(
    const std::vector<double>& inputs,
    const std::vector<double>& targets,
    int iterations,
    double learningRate)
{
    std::vector<double> values = parameters();
    constexpr double epsilon = 1.0e-3;
    for (int iteration = 0; iteration < iterations; ++iteration)
    {
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            const double original = values[index];
            values[index] = original + epsilon;
            setParameters(values);
            const double plus = cost(*this, inputs, targets);
            values[index] = original - epsilon;
            setParameters(values);
            const double minus = cost(*this, inputs, targets);
            values[index] = original - learningRate * (plus - minus) / (2.0 * epsilon);
            setParameters(values);
        }
    }
    return {cost(*this, inputs, targets), iterations};
}

std::size_t EVQKAN::parameterCount() const
{
    return parameters().size();
}

std::vector<double> EVQKAN::parameterValues() const
{
    return parameters();
}
}