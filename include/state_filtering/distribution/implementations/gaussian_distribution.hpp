/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014 Max-Planck-Institute for Intelligent Systems,
 *                     University of Southern California
 *    Manuel Wuthrich (manuel.wuthrich@gmail.com)
 *    Jan Issac (jan.issac@gmail.com)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @date 05/25/2014
 * @author Manuel Wuthrich (manuel.wuthrich@gmail.com)
 * @author Jan Issac (jan.issac@gmail.com)
 * Max-Planck-Institute for Intelligent Systems, University of Southern California
 */

#ifndef STATE_FILTERING_DISTRIBUTION_IMPLEMENTATIONS_GAUSSIAN_DISTRIBUTION_HPP
#define STATE_FILTERING_DISTRIBUTION_IMPLEMENTATIONS_GAUSSIAN_DISTRIBUTION_HPP

// eigen
#include <Eigen/Dense>

// boost
#include <boost/assert.hpp>
#include <boost/utility/enable_if.hpp>

// state_filtering
#include <state_filtering/distribution/distribution.hpp>
#include <state_filtering/distribution/features/moments_solvable.hpp>
#include <state_filtering/distribution/features/evaluable.hpp>
#include <state_filtering/distribution/features/gaussian_mappable.hpp>
#include <state_filtering/distribution/features/gaussian_sampleable.hpp>

namespace filter
{

/**
 * @brief GaussianDistribution is a parametrized distribution
 */
template <typename ScalarType_, int SIZE>
class GaussianDistribution:
        public AnalyticalMoments<ScalarType_, SIZE>,
        public Evaluable<ScalarType_, SIZE>,
        public GaussianMappable<ScalarType_, SIZE, SIZE>
        //public GaussianSampleable<GaussianMappable<ScalarType_, SIZE, SIZE> >
{
public: /* distribution traits */
    typedef AnalyticalMoments<ScalarType_, SIZE>        MomentsBaseType;
    typedef GaussianMappable<ScalarType_, SIZE, SIZE>   MappableBaseType;

    typedef typename MomentsBaseType::ScalarType        ScalarType;
    typedef typename MomentsBaseType::VariableType      VariableType;
    typedef typename MomentsBaseType::CovarianceType    CovarianceType;
    typedef typename MappableBaseType::RandomsType      RandomsType;

public:
    GaussianDistribution()
    {
        DISABLE_IF_DYNAMIC_SIZE(VariableType);

        setNormal();
    }

    explicit GaussianDistribution(int variable_size)
    {
        DISABLE_IF_FIXED_SIZE(VariableType);

        mean_.resize(variable_size, 1);
        covariance_.resize(variable_size, variable_size);
        precision_.resize(variable_size, variable_size);
        L_.resize(variable_size, variable_size);

        setNormal();
    }

    virtual ~GaussianDistribution() { }

    virtual VariableType mapFromGaussian(const RandomsType& sample) const
    {
        return mean_ + L_ * sample;
    }

    virtual void setNormal()
    {
        mean(VariableType::Zero(variableSize()));
        covariance(CovarianceType::Identity(variableSize(), variableSize()));
    }

    virtual void mean(const VariableType& mean)
    {
        mean_ = mean;
    }

    virtual void covariance(const CovarianceType& covariance)
    {
        // TODO: it should be able to deal with SEMI definite matrices as well!!
        // check the rank
        BOOST_ASSERT_MSG(covariance.colPivHouseholderQr().rank() == covariance.rows() ||
                         covariance.rows() == covariance.cols(),
                         "covariance matrix is not full rank");

        covariance_ = covariance;
        precision_ = covariance_.inverse();
        L_ = covariance_.llt().matrixL();

        // check the rank
        BOOST_ASSERT_MSG(covariance_.isApprox(L_*L_.transpose()),
                         "LLT decomposition went wrong. Matrix might not positive definite!");

        log_normalizer_ = -0.5
                * ( log(covariance_.determinant()) + double(covariance.rows()) * log(2.0 * M_PI) );
    }

    virtual VariableType mean() const
    {
        return mean_;
    }

    virtual CovarianceType covariance() const
    {
        return covariance_;
    }

    virtual ScalarType logProbability(const VariableType& sample) const
    {
        return log_normalizer_ - 0.5 * (sample - mean_).transpose() * precision_ * (sample - mean_);
    }

    virtual int variableSize() const
    {
        return mean_.rows();
    }

    virtual int randomsSize() const
    {
        return variableSize();
    }

protected:
    VariableType mean_;
    CovarianceType covariance_;
    CovarianceType precision_;
    CovarianceType L_;
    double log_normalizer_;
};

}

#endif