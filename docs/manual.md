# Quantum Regression Manual

## 1. What is predicted?

The task is supervised regression. Given a scalar input $x$, each model predicts a real number $\hat{y}$ for the continuous target

$$y = f(x) = \sin(x).$$

The training set contains 17 evenly spaced points from $-\pi$ to $\pi$. The test set contains 8 points between the training points. The test points are not used to update parameters.

The primary metric is mean squared error (MSE):

$$\text{MSE} = \frac{1}{N} \sum_{i} (\hat{y}_i - y_i)^2.$$

Lower test MSE means that the learned function is closer to $\sin(x)$ on the held-out inputs. Train time and parameter count are secondary engineering metrics; they are not proof of quantum advantage.

## 2. Quantum model

A one-qubit activation prepares $|0\rangle$, applies a feature rotation and a trainable rotation, then measures the expectation of Pauli-$Z$:

$$\phi(x; \theta) = \langle 0 | U(x, \theta)^\dagger Z U(x, \theta) | 0 \rangle.$$

The feature map is $R_Y(x - \pi/2)$ and the ansatz is $R_Y(\theta)$. Since a one-qubit $R_Y(a)$ state has $Z$ expectation $\cos(a)$, the activation is

$$\phi(x; \theta) = \cos\left(x - \frac{\pi}{2} + \theta\right) = \sin(x + \theta).$$

The QCS simulator performs the circuit execution. The new KAN code only constructs models from the public QCS and QNN APIs.

## 3. The three models

### QNN baseline

The baseline uses one quantum activation as the regressor:

$$\hat{y} = \phi(x; \theta).$$

It has one trainable parameter. This is a small parameterized quantum circuit, not a quantum version of XGBoost or clustering.

### QKAN

The QKAN is a one-layer Kolmogorov-Arnold-style composition:

$$\hat{y} = b + \sum_j w_j \phi_j(x; \theta_j).$$

Each edge has a one-dimensional trainable quantum function. The three activations use input scales 1, 2, and 3, giving the model multiple frequency components. Their trainable parameters, the three mixing weights, and the output bias are optimized together.

### EVQKAN

The enhanced model composes two KAN-like stages. First, a scalar hidden representation is formed:

$$h(x) = b_h + \sum_j a_j \phi_j(x; \theta_j).$$

The output stage applies additional quantum activations to that representation:

$$\hat{y} = b_o + \sum_k v_k \psi_k(h(x); \beta_k).$$

This adds depth and another set of trainable edge functions. It is called enhanced here because it extends the one-layer construction; exact research reproduction would require fixing the target paper and matching its stated architecture.

The implementation also includes a trainable residual quantum path from `x` to the output. Its initial weight is 1 and the correction-branch weights start at 0. This identity-preserving initialization is important for this benchmark: the target is already exactly represented by the one-qubit `sin(x)` activation, so adding depth should not make the result worse before the extra branch has learned a useful correction.

## 4. Training

The exported QNN implementation already supports forward evaluation, loss calculation, parameter-shift gradients, and training. The benchmark uses that implementation directly for the baseline.

The composed QKAN and EVQKAN models need gradients through sums, weights, and nested activations. This project uses a simple central finite-difference coordinate update for those small demonstrations:

$$\frac{\partial L}{\partial \theta} \approx \frac{L(\theta + \epsilon) - L(\theta - \epsilon)}{2 \epsilon}.$$

Each parameter is updated sequentially for the configured number of iterations. This is intentionally simple and transparent, but it is slower and less sophisticated than backpropagation through the full computational graph. The default finite-difference step is $\epsilon = 0.001$.

## 5. Reading the output

![Console Output showing comprision between the 3 Models](figure_1.png)

The executable prints:

- **Test MSE**: held-out regression error.
- **Parameters**: number of trainable scalar parameters.
- **Train time**: wall-clock time spent in the model's training loop.
- **Prediction table**: target values beside each model's predictions.

The first bordered table reports train MSE, held-out test MSE, test MAE, test $R^2$, maximum absolute test error, parameter count, and wall-clock training time. The second bordered table labels the target column `target=sin(x)` and shows every held-out prediction. The final-parameter section prints the trained scalar values in each model's internal parameter order.

For the included configuration, QNN and QKAN can reach near-zero error because their shared one-frequency activation basis contains the target exactly. EVQKAN has more expressive capacity and more parameters; its residual initialization preserves that solution while the nested branch is available for harder targets.

A meaningful comparison should use the same dataset, number of iterations, stopping policy, and hardware. One run is not enough to establish statistical superiority because initialization and optimizer choice affect results. For a stronger study, repeat each model with several seeds and report mean and standard deviation.

## 6. Project boundaries

The versioned `qcs/debug` and `qcs/release` directories provide the QCS 0.3.5 installed backend. CMake selects the matching library for each build configuration, while `include/KAN` contains the public project-level model headers and `src/KAN` contains their implementations.

The experiment demonstrates function approximation. It does not claim that these small simulator runs outperform classical neural networks, nor that simulator runtime represents hardware runtime.