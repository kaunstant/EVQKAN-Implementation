#include "KAN/QuantumActivation.hpp"

#include <cmath>
#include <stdexcept>

namespace KAN
{
namespace
{
constexpr double pi = 3.14159265358979323846;
}

QuantumActivation::QuantumActivation(double inputScale, double inputOffset)
    : inputScale_(inputScale), inputOffset_(inputOffset)
{
    model_.setFeatureMap(
        [this](const std::vector<double>& features)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, inputScale_ * features.at(0) + inputOffset_ - pi / 2.0);
            return circuit;
        });
    model_.setAnsatz(
        [](const std::vector<double>& parameters)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, parameters.at(0));
            return circuit;
        });
    model_.setObservable(Variational::Hamiltonian({{1.0, "Z"}}));
    model_.setOutputMode(QNN::OutputMode::Expectation);
}

double QuantumActivation::forward(double input)
{
    model_.setParameters({parameter_});
    return model_.forward({input});
}

void QuantumActivation::setParameter(double parameter)
{
    parameter_ = parameter;
}

double QuantumActivation::parameter() const
{
    return parameter_;
}

double meanSquaredError(
    const std::vector<double>& predictions,
    const std::vector<double>& targets)
{
    if (predictions.size() != targets.size() || predictions.empty())
    {
        throw std::invalid_argument("Prediction and target sets must have the same non-zero size");
    }

    double total = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index)
    {
        const double error = predictions[index] - targets[index];
        total += error * error;
    }
    return total / static_cast<double>(predictions.size());
}
}