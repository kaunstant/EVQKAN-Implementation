#include "KAN/QKAN.hpp"

#include <cmath>

namespace KAN
{
namespace
{
double cost(QKAN& model, const std::vector<double>& inputs, const std::vector<double>& targets)
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

QKAN::QKAN(std::size_t activationCount)
    : weights_(activationCount, 1.0 / static_cast<double>(activationCount))
{
    activations_.reserve(activationCount);
    for (std::size_t index = 0; index < activationCount; ++index)
    {
        activations_.emplace_back(1.0, 0.0);
    }
}

double QKAN::forward(double input)
{
    double output = bias_;
    for (std::size_t index = 0; index < activations_.size(); ++index)
    {
        output += weights_[index] * activations_[index].forward(input);
    }
    return output;
}

std::vector<double> QKAN::parameters() const
{
    std::vector<double> result;
    for (const QuantumActivation& activation : activations_)
    {
        result.push_back(activation.parameter());
    }
    result.insert(result.end(), weights_.begin(), weights_.end());
    result.push_back(bias_);
    return result;
}

void QKAN::setParameters(const std::vector<double>& values)
{
    const std::size_t activationCount = activations_.size();
    for (std::size_t index = 0; index < activationCount; ++index)
    {
        activations_[index].setParameter(values[index]);
        weights_[index] = values[activationCount + index];
    }
    bias_ = values.back();
}

TrainingResult QKAN::train(
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

std::size_t QKAN::parameterCount() const
{
    return parameters().size();
}

std::vector<double> QKAN::parameterValues() const
{
    return parameters();
}
}