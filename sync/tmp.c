#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  double x;
  double y;
} DataPoint;

typedef struct {
  double a, b, c, d;
} SplineCoefficients;

SplineCoefficients* calculateSplineCoefficients(DataPoint* data, int n) {
  SplineCoefficients* coefficients =
      (SplineCoefficients*)malloc(sizeof(SplineCoefficients) * n);
  double* h = (double*)malloc(sizeof(double) * (n - 1));

  for (int i = 0; i < n - 1; i++) {
    h[i] = data[i + 1].x - data[i].x;
  }

  double* alpha = (double*)malloc(sizeof(double) * (n - 1));
  for (int i = 1; i < n - 1; i++) {
    alpha[i] = (3.0 / h[i]) * (data[i + 1].y - data[i].y) -
               (3.0 / h[i - 1]) * (data[i].y - data[i - 1].y);
  }

  double* l = (double*)malloc(sizeof(double) * n);
  double* mu = (double*)malloc(sizeof(double) * (n - 1));
  double* z = (double*)malloc(sizeof(double) * n);

  l[0] = 1.0;
  mu[0] = 0.0;
  z[0] = 0.0;

  for (int i = 1; i < n - 1; i++) {
    l[i] = 2.0 * (data[i + 1].x - data[i - 1].x) - h[i - 1] * mu[i - 1];
    mu[i] = h[i] / l[i];
    z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
  }

  l[n - 1] = 1.0;
  z[n - 1] = 0.0;
  coefficients[n - 1].c = 0.0;

  for (int j = n - 2; j >= 0; j--) {
    coefficients[j].c = z[j] - mu[j] * coefficients[j + 1].c;
    coefficients[j].b =
        (data[j + 1].y - data[j].y) / h[j] -
        h[j] * (coefficients[j + 1].c + 2.0 * coefficients[j].c) / 3.0;
    coefficients[j].d =
        (coefficients[j + 1].c - coefficients[j].c) / (3.0 * h[j]);
    coefficients[j].a = data[j].y;
  }

  free(h);
  free(alpha);
  free(l);
  free(mu);
  free(z);
  return coefficients;
}

int main() {
  double n = 11;
  double a = -1, b = 1;

  double h = (b - a) / (n - 1);
  printf("%lf\n", h);
  DataPoint* data = (DataPoint*)malloc(sizeof(DataPoint) * n);

  double delta = 0;
  for (int i = 0; i < n; i++) {
    data[i].x = a + delta;
    // data[i].y = fabs(data[i].x);
    if (data[i].x < 3) data[i].y = sin(data[i].x) + 1;
    if (data[i].x == 3) data[i].y = sin(data[i].x) + 0.5;
    if (data[i].x > 3) data[i].y = sin(data[i].x);
    // data[i].y = fabs(data[i].x);
    delta += h;
  }

  SplineCoefficients* coefficients = calculateSplineCoefficients(data, n);

  for (int i = 0; i < n - 1; i++) {
    printf("%lf + %lf(x - %lf) + %lf(x-%lf)^2 + %lf(x-%lf)^3 {%lf < x < %lf}\n",
           coefficients[i].a, coefficients[i].b, data[i].x, coefficients[i].c,
           data[i].x, coefficients[i].d, data[i].x, data[i].x, data[i + 1].x);
  }

  free(coefficients);

  return 0;
}