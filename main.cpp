#include <QCS>
#include <KAN/EVQKAN.hpp>
#include <KAN/QKAN.hpp>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <string>
#include <vector>

namespace
{
struct Benchmark
{
    std::string name;
    double trainMse = 0.0;
    double testMse = 0.0;
    double mae = 0.0;
    double maxError = 0.0;
    double rSquared = 0.0;
    double milliseconds = 0.0;
    std::size_t parameterCount = 0;
    std::vector<double> parameters;
    std::vector<double> predictions;
};

// Fixed datasets make the three model comparisons reproducible.
const std::vector<double> trainInputs = {
    -3.141592653589793, -2.748893571891069, -2.356194490192345,
    -1.963495408493621, -1.570796326794897, -1.178097245096172,
    -0.785398163397448, -0.392699081698724, 0.0, 0.392699081698724,
    0.785398163397448, 1.178097245096172, 1.570796326794897,
    1.963495408493621, 2.356194490192345, 2.748893571891069,
    3.141592653589793};

const std::vector<double> testInputs = {
    -2.945243112740431, -2.159844949342982, -1.374446785945534,
    -0.589048622548086, 0.196349540849362, 0.981747704246810,
    1.767145867644258, 2.552544031041707};

double target(double input)
{
    return std::sin(input);
}

std::vector<double> targetsFor(const std::vector<double>& inputs)
{
    std::vector<double> targets;
    targets.reserve(inputs.size());
    for (double input : inputs)
    {
        targets.push_back(target(input));
    }
    return targets;
}

double meanAbsoluteError(const std::vector<double>& predictions, const std::vector<double>& targets)
{
    double total = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index)
    {
        total += std::abs(predictions[index] - targets[index]);
    }
    return total / static_cast<double>(predictions.size());
}

double maximumAbsoluteError(const std::vector<double>& predictions, const std::vector<double>& targets)
{
    double maximum = 0.0;
    for (std::size_t index = 0; index < predictions.size(); ++index)
    {
        maximum = std::max(maximum, std::abs(predictions[index] - targets[index]));
    }
    return maximum;
}

double coefficientOfDetermination(const std::vector<double>& predictions, const std::vector<double>& targets)
{
    const double average = std::accumulate(targets.begin(), targets.end(), 0.0) /
                           static_cast<double>(targets.size());
    double residual = 0.0;
    double total = 0.0;
    for (std::size_t index = 0; index < targets.size(); ++index)
    {
        residual += std::pow(predictions[index] - targets[index], 2.0);
        total += std::pow(targets[index] - average, 2.0);
    }
    return 1.0 - residual / total;
}

void fillMetrics(Benchmark& benchmark, const std::vector<double>& trainPredictions,
                 const std::vector<double>& trainTargets, const std::vector<double>& testTargets)
{
    benchmark.trainMse = KAN::meanSquaredError(trainPredictions, trainTargets);
    benchmark.testMse = KAN::meanSquaredError(benchmark.predictions, testTargets);
    benchmark.mae = meanAbsoluteError(benchmark.predictions, testTargets);
    benchmark.maxError = maximumAbsoluteError(benchmark.predictions, testTargets);
    benchmark.rSquared = coefficientOfDetermination(benchmark.predictions, testTargets);
}

Benchmark runQNN()
{
    // The baseline deliberately uses the public QCS QNN API unchanged.
    QNN::QNN model;
    model.setFeatureMap(
        [](const std::vector<double>& features)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, features.at(0) - 1.570796326794897);
            return circuit;
        });
    model.setAnsatz(
        [](const std::vector<double>& parameters)
        {
            QuantumCircuit circuit(1);
            circuit.ry(0, parameters.at(0));
            return circuit;
        });
    model.setObservable(Variational::Hamiltonian({{1.0, "Z"}}));
    model.setOutputMode(QNN::OutputMode::Expectation);
    model.setParameters({0.4});

    const std::vector<std::vector<double>> features = [&]()
    {
        std::vector<std::vector<double>> result;
        result.reserve(trainInputs.size());
        for (double input : trainInputs) result.push_back({input});
        return result;
    }();
    const std::vector<double> targets = targetsFor(trainInputs);
    const auto start = std::chrono::steady_clock::now();
    const Variational::OptimizationResult result = model.train(features, targets, 80, 0.12);
    const auto end = std::chrono::steady_clock::now();
    std::vector<double> predictions;
    for (double input : testInputs) predictions.push_back(model.forward({input}));
    Benchmark benchmark{"QNN"};
    benchmark.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    benchmark.parameterCount = result.parameters.size();
    benchmark.parameters = result.parameters;
    benchmark.predictions = predictions;
    std::vector<double> trainPredictions;
    for (double input : trainInputs) trainPredictions.push_back(model.forward({input}));
    fillMetrics(benchmark, trainPredictions, targets, targetsFor(testInputs));
    return benchmark;
}

Benchmark runQKAN()
{
    // QKAN composes several quantum edge functions above the simulator.
    KAN::QKAN model(3);
    const auto start = std::chrono::steady_clock::now();
    model.train(trainInputs, targetsFor(trainInputs), 120, 0.04);
    const auto end = std::chrono::steady_clock::now();
    std::vector<double> predictions;
    for (double input : testInputs) predictions.push_back(model.forward(input));
    Benchmark benchmark{"QKAN"};
    benchmark.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    benchmark.parameterCount = model.parameterCount();
    benchmark.parameters = model.parameterValues();
    benchmark.predictions = predictions;
    std::vector<double> trainPredictions;
    for (double input : trainInputs) trainPredictions.push_back(model.forward(input));
    fillMetrics(benchmark, trainPredictions, targetsFor(trainInputs), targetsFor(testInputs));
    return benchmark;
}

Benchmark runEVQKAN()
{
    // EVQKAN adds a nested correction branch and an identity-preserving skip path.
    KAN::EVQKAN model(3, 2);
    const auto start = std::chrono::steady_clock::now();
    model.train(trainInputs, targetsFor(trainInputs), 120, 0.03);
    const auto end = std::chrono::steady_clock::now();
    std::vector<double> predictions;
    for (double input : testInputs) predictions.push_back(model.forward(input));
    Benchmark benchmark{"EVQKAN"};
    benchmark.milliseconds = std::chrono::duration<double, std::milli>(end - start).count();
    benchmark.parameterCount = model.parameterCount();
    benchmark.parameters = model.parameterValues();
    benchmark.predictions = predictions;
    std::vector<double> trainPredictions;
    for (double input : trainInputs) trainPredictions.push_back(model.forward(input));
    fillMetrics(benchmark, trainPredictions, targetsFor(trainInputs), targetsFor(testInputs));
    return benchmark;
}

void printBenchmark(const Benchmark& benchmark)
{
    std::cout << "| " << std::left << std::setw(8) << benchmark.name
              << " | " << std::right << std::setw(10) << benchmark.trainMse
              << " | " << std::setw(10) << benchmark.testMse
              << " | " << std::setw(10) << benchmark.mae
              << " | " << std::setw(10) << benchmark.rSquared
              << " | " << std::setw(10) << benchmark.maxError
              << " | " << std::setw(10) << benchmark.parameterCount
              << " | " << std::setw(12) << benchmark.milliseconds << " ms |\n";
}

void printParameters(const Benchmark& benchmark)
{
    std::cout << benchmark.name << ": [";
    for (std::size_t index = 0; index < benchmark.parameters.size(); ++index)
    {
        if (index != 0) std::cout << ", ";
        std::cout << benchmark.parameters[index];
    }
    std::cout << "]\n";
}
}

int main()
{
    const Benchmark qnn = runQNN();
    const Benchmark qkan = runQKAN();
    const Benchmark evqkan = runEVQKAN();

    std::cout << "Quantum regression benchmark: target f(x) = sin(x)\n\n";
    std::cout << "| Model    | Train MSE   | Test MSE   | Test MAE    | Test R^2   | Max error   | Params     | Train time      |\n";
    std::cout << "|----------|-------------|------------|-------------|------------|-------------|------------|-----------------|\n";
    printBenchmark(qnn);
    printBenchmark(qkan);
    printBenchmark(evqkan);

    std::cout << "\nFinal parameters after training\n";
    std::cout << std::fixed << std::setprecision(6);
    printParameters(qnn);
    printParameters(qkan);
    printParameters(evqkan);

    std::cout << "\nTest predictions\n";
    std::cout << "| " << std::left << std::setw(12) << "x"
              << " | " << std::setw(16) << "target=sin(x)"
              << " | " << std::setw(12) << "QNN"
              << " | " << std::setw(12) << "QKAN"
              << " | " << std::setw(12) << "EVQKAN" << " |\n";
    std::cout << "|--------------|------------------|--------------|--------------|--------------|\n";
    for (std::size_t index = 0; index < testInputs.size(); ++index)
    {
        std::cout << "| " << std::fixed << std::setprecision(6)
              << std::setw(12) << testInputs[index] << " | "
              << std::setw(16) << target(testInputs[index]) << " | "
              << std::setw(12) << qnn.predictions[index] << " | "
              << std::setw(12) << qkan.predictions[index] << " | "
              << std::setw(12) << evqkan.predictions[index] << " |\n";
    }

    return 0;
}