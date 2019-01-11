#ifndef __FAST_POINT_FEATURE_HISTOGRAMS__
#define	__FAST_POINT_FEATURE_HISTOGRAMS__
// ============================================================================
// Includes
#include <string>
#include <vector>
#include <Open3D/Core/Core.h>
#include <Eigen/Dense>
// ============================================================================
// Function prototypes
// Main function for organising the correspondences.
std::vector<Eigen::Vector2i> computeCorrespondancePair(
	open3d::PointCloud &model_0, open3d::PointCloud &model_1);

// Computation of Fast Point Feature Histograms
Eigen::MatrixXd computeFPFH(
	open3d::PointCloud &model, double radius, 
	open3d::KDTreeFlann &distTree);
void computePersistentPoints(
	open3d::PointCloud &model, std::vector<int> &P_0, Eigen::MatrixXd &FPFH);

// Compute the nearest neighbours of P_mod in P_q using FPFH as distance
std::vector<Eigen::Vector2i> nearestNeighbour(
	std::vector<int> P_mod, std::vector<int> P_q, 
	Eigen::MatrixXd FPFH_mod, open3d::KDTreeFlann &KDTree_q);

// Correction functions
std::vector<Eigen::Vector2i> mutualNN(
	std::vector<Eigen::Vector2i > K_0, std::vector<Eigen::Vector2i> K_1);
std::vector<Eigen::Vector2i> tupleTest(
	std::vector<Eigen::Vector2i> K_II, 
	open3d::PointCloud model_0, open3d::PointCloud model_1);


#endif	/* __FAST_POINT_FEATURE_HISTOGRAMS__ */
