#ifndef GENERIC_MATH_INTERPOLATION_HPP
#define GENERIC_MATH_INTERPOLATION_HPP
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/banded.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include "generic/common/Traits.hpp"
#include "MathUtility.hpp"
#include <vector>
namespace generic {
namespace math    {
using generic::common::float_type;
template <typename num_type>
class Interpolation
{
    using float_t = float_type<num_type>;
    using vector_t = boost::numeric::ublas::vector<float_t>;
    using band_matrix_t = boost::numeric::ublas::banded_matrix<float_t>;
public:
    enum class Method { Linear = 1, Cubic = 2};
    enum class BCType { FirstDeriv = 1, SecondDeriv = 2, NotAKnot = 3 };

    Interpolation(Method method = Method::Linear);
    Interpolation(const std::vector<num_type> & x, const std::vector<num_type> & y,
                    Method method, bool monotonic = false,
                    BCType left  = BCType::SecondDeriv, float_t lValue  = 0,
                    BCType right = BCType::SecondDeriv, float_t rValue = 0);

    void SetBoundary(BCType left, BCType right, float_t lVal, float_t rVal);
    void SetSamples(const std::vector<num_type> & x, const std::vector<num_type> & y);

    num_type operator() (num_type x) const { return Interpolate(x); }

private:
    void UpdateCoefficients();
    bool makeMonotonic();
    size_t FindClosest(num_type x) const;
    num_type Interpolate(num_type x) const;
    num_type Interpolate(num_type x, const num_integer_tag) const;
    float_t Interpolate(num_type x, const num_floating_tag) const;

private:
    Method m_method;
    bool m_monotonic;
    BCType m_left, m_right;
    float_t m_lVal, m_rVal;
    std::vector<num_type> m_x, m_y;
    float_t m_coef0 = 0;
    std::vector<float_t> m_coef1, m_coef2, m_coef3;
};

template <typename num_type>
inline Interpolation<num_type>::Interpolation(Method method)
 : m_method(method), m_monotonic(false), m_left(BCType::SecondDeriv), m_right(BCType::SecondDeriv), m_lVal(0), m_rVal(0)
{
}

template <typename num_type>
inline Interpolation<num_type>::Interpolation(const std::vector<num_type> & x, const std::vector<num_type> & y,
                                                Method method, bool monotonic,
                                                BCType left, float_t lValue, BCType right, float_t rValue)
 : m_method(method), m_monotonic(monotonic), m_left(left), m_right(right), m_lVal(lValue), m_rVal(rValue)
{
    SetSamples(x, y);
}

template <typename num_type>
inline void Interpolation<num_type>::SetBoundary(BCType left, BCType right, float_t lVal, float_t rVal)
{
    assert(m_x.size() == 0 && "should set boundary type before setting samples");
    m_left = left; m_right = right;
    m_lVal = lVal; m_rVal = rVal;
}

template <typename num_type>
inline void Interpolation<num_type>::SetSamples(const std::vector<num_type> & x, const std::vector<num_type> & y)
{
    const size_t size = x.size();
    assert(size > 0 && size == y.size());
    m_x = x; m_y = y;
    m_coef1.assign(size, 0); m_coef2.assign(size, 0), m_coef3.assign(size, 0);
    if(size < 2) return;
    if(size < 3) m_method = Method::Linear;
    if(Method::Linear == m_method){
        for(size_t i = 0, j = 1; i < size - 1; ++i, ++j)
            m_coef1[i] = float_t(m_y[j] - m_y[i]) / float_t(m_x[j] - m_x[i]);
        m_coef1[size - 1] = m_coef1[size - 2];
    }
    else if(Method::Cubic == m_method){
        size_t lower = (m_right == BCType::NotAKnot) ? 2 : 1;
        size_t upper = (m_left  == BCType::NotAKnot) ? 2 : 1;
        band_matrix_t m(size, size, lower, upper);
        vector_t rhs(size);
        for(size_t i = 0, j = 1, k = 2; k < size; ++i, ++j, ++k){
            m(j, i) = 1.0 / 3.0 * (m_x[j] - m_x[i]);
            m(j, j) = 2.0 / 3.0 * (m_x[k] - m_x[i]);
            m(j, k) = 1.0 / 3.0 * (m_x[k] - m_x[j]);
            rhs[j] = (m_y[k] - m_y[j]) / (m_x[k] - m_x[j]) - (m_y[j] - m_y[i]) / (m_x[j] - m_x[i]);
        }
        //BC
        if(m_left == BCType::FirstDeriv){
            m(0, 0) = 2.0 * (m_x[1] - m_x[0]);
            m(0, 1) = 1.0 * (m_x[1] - m_x[0]);
            rhs[0] = 3.0 * ((m_y[1] - m_y[0]) / (m_x[1] - m_x[0]) - m_lVal);
        }
        else if(m_left == BCType::SecondDeriv){
            m(0, 0) = 2.0;
            m(0, 1) = 0.0;
            rhs[0] = m_lVal;
        }
        else if(m_left == BCType::NotAKnot){
            m(0, 0) = - m_x[2] + m_x[1];
            m(0, 1) =   m_x[2] - m_x[0];
            m(0, 2) = - m_x[1] + m_x[0];
            rhs[0] = 0.0;
        }
        else { assert(false); }

        if(m_right == BCType::FirstDeriv){
            m(size - 1, size - 1) = 2.0 * (m_x[size - 1] - m_x[size - 2]);
            m(size - 1, size - 2) = 1.0 * (m_x[size - 1] - m_x[size - 2]);
            rhs[size - 1] = 3.0 * (m_rVal - (m_y[size - 1] - m_y[size - 2]) / (m_x[size - 1] - m_x[size - 2]));
        }
        else if(m_right == BCType::SecondDeriv){
            m(size - 1, size - 1) = 2.0;
            m(size - 1, size - 2) = 0.0;
            rhs[size - 1] = m_rVal;
        }
        else if(m_right == BCType::NotAKnot){
            m(size - 1, size - 3) = - m_x[size - 1] + m_x[size - 2];
            m(size - 1, size - 2) =   m_x[size - 1] - m_x[size - 3];
            m(size - 1, size - 1) = - m_x[size - 2] + m_x[size - 3]; 
        }
        else { assert(false); }

        boost::numeric::ublas::permutation_matrix<size_t> pm(size);
        lu_factorize(m, pm);
        lu_substitute(m, pm, rhs);
        for(size_t i = 0; i < size; ++i)
            m_coef2[i] = rhs[i];
        
        for(size_t i = 0, j = 1; j < size; ++i, ++j){
            m_coef1[i] = (m_y[j] - m_y[i]) / (m_x[j] - m_x[i]) - 1.0 / 3.0 * (2.0 * m_coef2[i] + m_coef2[j]) * (m_x[j] - m_x[i]);
            m_coef3[i] = 1.0 / 3.0 * (m_coef2[j] - m_coef2[i]) / (m_x[j] - m_x[i]);
        }

        float_t h = x[size - 1] - x[size - 2];
        m_coef3[size - 1] = 0.0;
        m_coef1[size - 1] = 3.0 * m_coef3[size - 2] * h * h + 2.0 * m_coef2[size - 2] * h + m_coef1[size - 2];
        if(m_right == BCType::FirstDeriv) m_coef2[size - 1] = 0.0;
    }
    m_coef0 = (m_left == BCType::FirstDeriv) ? 0 : m_coef3[0];
    
    if(!m_monotonic && size > 2) makeMonotonic();
}

template <typename num_type>
inline void Interpolation<num_type>::UpdateCoefficients()
{
    size_t size = m_coef1.size();
    for(size_t i = 0; i < size - 1; ++i){
        const float_t h = m_x[i + 1] - m_x[i];
        m_coef2[i] = (3.0 * (m_y[i + 1] - m_y[i]) / h - (2.0 * m_coef1[i] + m_coef1[i + 1])) / h;
        m_coef3[i] = ((m_coef1[i + 1] - m_coef1[i]) / (3.0 * h) - 2.0 / 3.0 * m_coef2[i]) / h;
    }
    m_coef0 = (m_left == BCType::FirstDeriv) ? 0 : m_coef3[0];
}

template <typename num_type>
inline bool Interpolation<num_type>::makeMonotonic()
{
    assert(m_x.size() > 2);
    bool modified = false;
    for(size_t i = 0; i < m_x.size(); ++i){
        size_t im1 = std::max(i - 1, size_t(0));
        size_t ip1 = std::min(i + 1, m_x.size() - 1);
        if((m_y[im1] <= m_y[i] && m_y[i] <= m_y[ip1] && m_coef1[i] < 0) ||
           (m_y[im1] >= m_y[i] && m_y[i] >= m_y[ip1] && m_coef1[i] > 0)) {
            modified = true;
            m_coef1[i] = 0;
        }
    }

    for(size_t i = 0; i < m_x.size() - 1; ++i){
        float_t avg = float_t(m_y[i + 1] - m_y[i]) / float_t(m_x[i + 1] - m_x[i]);
        if(EQ(avg, 0.0) && (!EQ(m_coef1[i], 0.0) || !EQ(m_coef1[i], 0.0))){
            modified = true;
            m_coef1[i] = 0;
            m_coef1[i + 1] = 0;
        }
        else if((GE(m_coef1[i], 0.0) && GE(m_coef1[i + 1], 0.0) && GE(avg, 0.0)) ||
                (LE(m_coef1[i], 0.0) && LE(m_coef1[i + 1], 0.0) && LE(avg, 0.0))) {
            float_t r = std::sqrt(m_coef1[i] * m_coef1[i] + m_coef1[i + 1] * m_coef1[i + 1]) / std::fabs(avg);
            if(r > 3.0){
                modified = true;
                m_coef1[i] *= (3.0 / r);
                m_coef1[i + 1] *= (3.0 / r);
            }
        }
    }

    if(modified == true){
        UpdateCoefficients();
        m_monotonic = true;
    }
    return modified;
}

template <typename num_type>
inline size_t Interpolation<num_type>::FindClosest(num_type x) const
{
    typename std::vector<num_type>::const_iterator iter;
    iter = std::upper_bound(m_x.begin(), m_x.end(), x);
    size_t index = std::max(int(iter - m_x.begin() - 1), 0);
    return index;
}

template <typename num_type>
inline num_type Interpolation<num_type>::Interpolate(num_type x) const
{
    return Interpolate(x, typename num_traits_float_or_int<num_type>::tag());
}

template <typename num_type>
inline num_type Interpolation<num_type>::Interpolate(num_type x, const num_integer_tag) const
{
    return num_type(std::round(Interpolate(x, num_floating_tag())));
}

template <typename num_type>
inline float_type<num_type> Interpolation<num_type>::Interpolate(num_type x, const num_floating_tag) const
{
    size_t n = m_x.size();
    size_t idx = FindClosest(x);
    float_t h = x - m_x[idx];
    if(x < m_x[0]) return (m_coef0 * h + m_coef1[0]) * h + m_y[0];
    else if(x > m_x[n - 1]) return (m_coef2[n-1] * h + m_coef1[n - 1]) * h + m_y[n - 1];
    else return ((m_coef3[idx] * h + m_coef2[idx]) * h + m_coef1[idx]) * h + m_y[idx];        
}

}//namespace math
}//namespace generic
#endif//GENERIC_MATH_INTERPOLATION_HPP