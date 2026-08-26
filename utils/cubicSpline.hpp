#ifndef __CUBIC_SPLINE_HPP__
#define __CUBIC_SPLINE_HPP__

#include <vector>
#include "utils.h"

/* natural cubic spline */
class CubicSpline: public Utils {
public:
    CubicSpline(const std::vector<float>& freqs, const std::vector<float>& db)
        : x(freqs.size())
        , a(x.size() - 1)
        , b(x.size() - 1)
        , c(x.size() - 1)
        , d(x.size() - 1) {

        int n = x.size();
        for (int i = 0; i < n; i++) {
            x[i] = freqs[i];
        }

        std::vector<double> h(n - 1);
        for (int i = 0; i < h.size(); i++) {
            h[i] = x[i + 1] - x[i];
            a[i] = db[i];
        }

        std::vector<double> M(n);
        std::vector<double> diag(n, 0), upper(n - 1, 0), lower(n - 1, 0), rhs(n, 0);
        
        for (int i = 1; i < n - 1; i++) {
            diag[i] = 2.0 * (h[i - 1] + h[i]);
            upper[i] = h[i];
            lower[i - 1] = h[i - 1];
            rhs[i] = 6.0 * ((db[i + 1] - db[i]) / h[i] - (db[i] - db[i - 1]) / h[i - 1]);
        }

        diag[0] = 1.0;
        diag[n - 1] = 1.0;
        rhs[0] = 0.0;
        rhs[n - 1] = 0.0;

        solveTridiagonal(diag, upper, lower, rhs);
        M = rhs;

        for (int i = 0; i < n - 1; i++) {
            c[i] = M[i] / 2.0;
            d[i] = (M[i + 1] - M[i]) / (6.0 * h[i]);
            b[i] = (db[i + 1] - db[i]) / h[i] - h[i] * M[i] / 2.0 - h[i] * (M[i + 1] - M[i]) / 6.0;
        }
    }

    ~CubicSpline() = default;

    float interpolate(float x) const {
        int n = this->x.size();

        if (x <= this->x[0]) {
            double dx = x - this->x[0];
            return a[0] + b[0] * dx + c[0] * dx * dx + d[0] * dx * dx * dx;
        }

        if (x >= this->x[n - 1]) {
            int i = n - 2;
            double dx = x - this->x[i];
            return a[i] + b[i] * dx + c[i] * dx * dx + d[i] * dx * dx * dx;
        }

        int i = 0;
        while (i < n - 2 && x > this->x[i + 1]) {
            i++;
        }

        double dx = x - this->x[i];
        return a[i] + b[i] * dx + c[i] * dx * dx + d[i] * dx * dx * dx;
    }

    std::vector<float> interpolate(const std::vector<float>& xs) {
        std::vector<float> result;
        result.reserve(xs.size());

        for (float x: xs) {
            result.push_back(interpolate(x));
        }

        return result;
    }

private:
    void solveTridiagonal(std::vector<double>& diag, std::vector<double>& upper, std::vector<double>& lower, std::vector<double>& rhs) {
        int n = diag.size();

        for (int i = 1; i < n; i++) {
            double factor = lower[i - 1] / diag[i - 1];
            diag[i] -= factor * upper[i - 1];
            rhs[i] -= factor * rhs[i - 1];
        }

        rhs[n - 1] /= diag[n - 1];
        for (int i = n - 2; i >= 0; i--) {
            rhs[i] = (rhs[i] - upper[i] * rhs[i + 1]) / diag[i];
        }
    }

    std::vector<double> x;
    std::vector<double> a, b, c, d;
};

#endif
