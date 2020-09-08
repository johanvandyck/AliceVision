// This file is part of the AliceVision project.
// Copyright (c) 2016 AliceVision contributors.
// Copyright (c) 2012 openMVG contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <aliceVision/feature/metric.hpp>
#include <aliceVision/matching/ArrayMatcher.hpp>
#include <aliceVision/matching/CascadeHasher.hpp>
#include <aliceVision/matching/IndMatch.hpp>

#include <aliceVision/system/Logger.hpp>

#include <memory>
#include <random>
#include <cmath>

namespace aliceVision {
namespace matching {

//------------------
//-- Bibliography --
//------------------
//- [1] "Fast and Accurate Image Matching with Cascade Hashing for 3D Reconstruction"
//- Authors: Jian Cheng, Cong Leng, Jiaxiang Wu, Hainan Cui, Hanqing Lu.
//- Date: 2014.
//- Conference: CVPR.
//

// Implementation of descriptor matching using the cascade hashing method of [1].
// If you use this matcher, please cite the paper.
// template Metric parameter is ignored (by default compute square(L2 distance)).
template < typename Scalar = float, typename Metric = feature::L2_Simple<Scalar> >
class ArrayMatcher_cascadeHashing  : public ArrayMatcher<Scalar, Metric>
{
  public:
  typedef typename Metric::ResultType DistanceType;

  ArrayMatcher_cascadeHashing()   {}
  virtual ~ArrayMatcher_cascadeHashing() {
    memMapping.reset();
  }

  /**
   * Build the matching structure
   *
   * \param[in] dataset   Input data.
   * \param[in] nbRows    The number of component.
   * \param[in] dimension Length of the data contained in the dataset.
   *
   * \return True if success.
   */
  bool Build(std::mt19937 & gen, const Scalar * dataset, int nbRows, int dimension) {
    if (nbRows < 1) {
      memMapping.reset(nullptr);
      return false;
    }
    memMapping.reset(new Eigen::Map<BaseMat>( (Scalar*)dataset, nbRows, dimension) );

    // Init the cascade hasher (hashing projection matrices)
    cascade_hasher_.Init(gen, dimension);
    // Index the input descriptors
    zero_mean_descriptor_ = CascadeHasher::GetZeroMeanDescriptor(*memMapping);
    hashed_base_ = cascade_hasher_.CreateHashedDescriptions(
      *memMapping, zero_mean_descriptor_);

    return true;
  };

  /**
   * Search the nearest Neighbor of the scalar array query.
   *
   * \param[in]   query     The query array
   * \param[out]  indice    The indice of array in the dataset that
   *  have been computed as the nearest array.
   * \param[out]  distance  The distance between the two arrays.
   *
   * \return True if success.
   */
  bool SearchNeighbour( const Scalar * query,
                        int * indice, DistanceType * distance)
  {
    ALICEVISION_LOG_WARNING("This matcher is not made to match a single query");
    return false;
  }


/**
   * Search the N nearest Neighbor of the scalar array query.
   *
   * \param[in]   query     The query array
   * \param[in]   nbQuery   The number of query rows
   * \param[out]  indices   The corresponding (query, neighbor) indices
   * \param[out]  distances  The distances between the matched arrays.
   * \param[out]  NN        The number of maximal neighbor that will be searched.
   *
   * \return True if success.
   */
  bool SearchNeighbours
  (
    const Scalar * query, int nbQuery,
    IndMatches * pvec_indices,
    std::vector<DistanceType> * pvec_distances,
    size_t NN
  )
  {
    if (memMapping.get() == nullptr)  {
      return false;
    }

    if (NN > (*memMapping).rows() || nbQuery < 1) {
      return false;
    }

    // Matrix representation of the input data;
    Eigen::Map<BaseMat> mat_query((Scalar*)query, nbQuery, (*memMapping).cols());

    pvec_distances->reserve(nbQuery * NN);
    pvec_indices->reserve(nbQuery * NN);

    // Index the query descriptors
    const HashedDescriptions hashed_query = cascade_hasher_.CreateHashedDescriptions(
      mat_query,
      zero_mean_descriptor_);
    // Match the query descriptors to the database
    cascade_hasher_.Match_HashedDescriptions(
      hashed_query, mat_query,
      hashed_base_, *memMapping,
      pvec_indices, pvec_distances,
      NN);

    return true;
  };

private:
  typedef Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> BaseMat;
  /// Use a memory mapping in order to avoid memory re-allocation
  std::unique_ptr< Eigen::Map<BaseMat> > memMapping;
  CascadeHasher cascade_hasher_;
  HashedDescriptions hashed_base_;
  Eigen::VectorXf zero_mean_descriptor_;
};

}  // namespace matching
}  // namespace aliceVision
