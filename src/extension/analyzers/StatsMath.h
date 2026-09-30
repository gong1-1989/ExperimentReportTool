/**
 * @file StatsMath.h
 * @brief 统计分布数学工具（头文件内联实现）
 *
 * 仅依赖 <cmath>，MinGW 可编译。实现：
 *  - 不完全 Beta 函数（正则化）btai（连分数近似）
 *  - t 分布累积分布函数 tCdf / 双尾 p 值
 *  - F 分布累积分布函数 fCdf
 *  - 基础统计量（均值/标准差/中位数）
 */

#ifndef STATS_MATH_H
#define STATS_MATH_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <limits>
#include <QVariant>

namespace StatsMath {

// ---- 不完全 Beta 函数（正则化）连分数 ----
inline double betacf(double a, double b, double x)
{
    const int MAXIT = 200;
    const double EPS = 3.0e-12;
    const double FPMIN = 1.0e-300;

    const double qab = a + b;
    const double qap = a + 1.0;
    const double qam = a - 1.0;
    double c = 1.0;
    double d = 1.0 - qab * x / qap;
    if (std::fabs(d) < FPMIN) d = FPMIN;
    d = 1.0 / d;
    double h = d;
    for (int m = 1; m <= MAXIT; ++m) {
        const int m2 = 2 * m;
        double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < FPMIN) d = FPMIN;
        c = 1.0 + aa / c;
        if (std::fabs(c) < FPMIN) c = FPMIN;
        d = 1.0 / d;
        h *= d * c;
        aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
        d = 1.0 + aa * d;
        if (std::fabs(d) < FPMIN) d = FPMIN;
        c = 1.0 + aa / c;
        if (std::fabs(c) < FPMIN) c = FPMIN;
        d = 1.0 / d;
        const double del = d * c;
        h *= del;
        if (std::fabs(del - 1.0) < EPS) break;
    }
    return h;
}

inline double btai(double a, double b, double x)
{
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    const double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) - std::lgamma(b)
                               + a * std::log(x) + b * std::log(1.0 - x));
    if (x < (a + 1.0) / (a + b + 2.0))
        return bt * betacf(a, b, x) / a;
    return 1.0 - bt * betacf(b, a, 1.0 - x) / b;
}

// ---- t 分布 CDF（单尾 F(t)），df > 0 ----
inline double tCdf(double t, double df)
{
    if (t <= 0.0) return 0.5 * btai(df / 2.0, 0.5, df / (df + t * t));
    return 1.0 - 0.5 * btai(df / 2.0, 0.5, df / (df + t * t));
}

/// 双尾 p 值
inline double tTwoTailP(double t, double df)
{
    return 2.0 * (1.0 - tCdf(std::fabs(t), df));
}

// ---- F 分布 CDF ----
inline double fCdf(double f, double df1, double df2)
{
    if (f <= 0.0) return 0.0;
    return btai(df1 / 2.0, df2 / 2.0, df1 * f / (df1 * f + df2));
}

// ---- 基础统计量 ----
inline double mean(const std::vector<double>& v)
{
    if (v.empty()) return 0.0;
    double s = 0.0;
    for (double x : v) s += x;
    return s / static_cast<double>(v.size());
}

/// 样本标准差（n-1），n<2 返回 0
inline double stddev(const std::vector<double>& v)
{
    const size_t n = v.size();
    if (n < 2) return 0.0;
    const double m = mean(v);
    double s = 0.0;
    for (double x : v) s += (x - m) * (x - m);
    return std::sqrt(s / static_cast<double>(n - 1));
}

inline double median(std::vector<double> v)
{
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    if (n % 2 == 1) return v[n / 2];
    return (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

/// 数值转 double（QVariant 兼容；失败返回 NaN）
inline double toDouble(const QVariant& v, bool* ok = nullptr)
{
    bool localOk = false;
    const double d = v.toDouble(&localOk);
    if (ok) *ok = localOk;
    return localOk ? d : std::numeric_limits<double>::quiet_NaN();
}

} // namespace StatsMath

#endif // STATS_MATH_H
