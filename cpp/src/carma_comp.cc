#include <iostream>
#include <complex>
#include <sys/time.h>
#include <Eigen/Core>

#include "genrp/solvers.h"
#include "genrp/carma.h"

// Timer for the benchmark.
double get_timestamp ()
{
  struct timeval now;
  gettimeofday (&now, NULL);
  return double(now.tv_usec) * 1.0e-6 + double(now.tv_sec);
}

int main (int argc, char* argv[])
{
  srand(42);

  size_t nterms = 3;
  if (argc >= 2) nterms = atoi(argv[1]);
  size_t N_max = pow(2, 19);
  if (argc >= 3) N_max = atoi(argv[2]);
  size_t niter = 5;
  if (argc >= 4) niter = atoi(argv[3]);

  double sigma = 1.0;

  Eigen::VectorXd carma_arparams = Eigen::VectorXd::Random(nterms),
                  carma_maparams = Eigen::VectorXd::Random(nterms-1);

  // Set up the coefficients.
  size_t n_complex = nterms / 2,
         n_real = nterms - n_complex * 2;
  std::cout << n_real << " " << n_complex << "\n";
  Eigen::VectorXd alpha_real = Eigen::VectorXd::Random(n_real),
                  beta_real = Eigen::VectorXd::Random(n_real),
                  alpha_complex = Eigen::VectorXd::Random(n_complex),
                  beta_complex_real = Eigen::VectorXd::Random(n_complex),
                  beta_complex_imag = Eigen::VectorXd::Random(n_complex);
  if (n_real > 0) {
    alpha_real.array() += 1.0;
    beta_real.array() += 1.0;
  }
  if (n_complex > 0) {
    alpha_complex.array() += 1.0;
    beta_complex_real.array() += 1.0;
    beta_complex_imag.array() += 1.0;
  }

  // Generate some fake data.
  Eigen::VectorXd x0 = Eigen::VectorXd::Random(N_max),
                  yerr0 = Eigen::VectorXd::Random(N_max),
                  y0;

  // Set the scale of the uncertainties.
  yerr0.array() *= 0.1;
  yerr0.array() += 1.0;

  // The times need to be sorted.
  std::sort(x0.data(), x0.data() + x0.size());

  // Compute the y values.
  y0 = sin(x0.array());

  genrp::BandSolver<double> solver;

  for (size_t N = 64; N <= N_max; N *= 2) {
    Eigen::VectorXd x = x0.topRows(N),
                    yerr = yerr0.topRows(N),
                    y = y0.topRows(N);

    // Benchmark the solver.
    double strt,
           compute_time = 0.0, solve_time = 0.0, carma_time = 0.0;

    for (size_t i = 0; i < niter; ++i) {
      strt = get_timestamp();
      solver.compute(alpha_real, beta_real, alpha_complex, beta_complex_real, beta_complex_imag, x, yerr);
      compute_time += get_timestamp() - strt;
    }

    for (size_t i = 0; i < niter; ++i) {
      strt = get_timestamp();
      solver.dot_solve(y);
      solve_time += get_timestamp() - strt;
    }

    for (size_t i = 0; i < niter; ++i) {
      strt = get_timestamp();
      genrp::carma::CARMASolver carma_solver(0.0, carma_arparams, carma_maparams);
      carma_solver.setup();
      carma_solver.log_likelihood(x, y, yerr);
      carma_time += get_timestamp() - strt;
    }

    // Print the results.
    std::cout << N;
    std::cout << " ";
    std::cout << (compute_time + solve_time) / niter;
    std::cout << " ";
    std::cout << carma_time / niter;
    std::cout << "\n";
  }

  return 0;
}