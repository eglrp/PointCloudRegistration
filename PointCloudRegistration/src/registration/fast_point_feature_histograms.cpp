// ============================================================================
// INCLUDES

#include <iostream>
#include <math.h>
#include <numeric>
#include <random>
#include <Open3D/Core/Core.h>
#include <Eigen/Dense>

#include "fast_point_feature_histograms.h"


using namespace std;
using namespace Eigen;
using namespace open3d;


// ============================================================================
// COMPUTE CORRESPONDENCES

vector<vector<Vector2i>> computeCorrespondences(vector<PointCloud> models)
{
	// Define the vectors holding information
	size_t nSurfaces = models.size();

	vector<vector<Vector2i>>	K(nSurfaces - 1);
	vector<vector<int>>			P(nSurfaces);
	vector<MatrixXd>			FPFH(nSurfaces);
	vector<KDTreeFlann>			KDTree(nSurfaces);

	// Compute the first persistent points
	computePersistentPoints(models[0], P[0], FPFH[0]);
	KDTree[0].SetMatrixData(FPFH[0].transpose());

	// Compute correspondences
	for (size_t s = 0; s < models.size() - 1; s++)
	{
		computePersistentPoints(models[s + 1], P[s + 1], FPFH[s + 1]);
		KDTree[s + 1].SetMatrixData(FPFH[s + 1].transpose());

		vector<Vector2i> K_I_0, K_I_1, K_II, K_III;
		K_I_0 = nearestNeighbour(P[s], P[s + 1], FPFH[s], KDTree[s + 1]);
		K_I_1 = nearestNeighbour(P[s + 1], P[s], FPFH[s + 1], KDTree[s]);

		K_II = mutualNN(K_I_0, K_I_1);
		K_III = tupleTest(K_II, models[s], models[s + 1]);

		// Set up correspondances
		if (K_III.size() != 0)
			K[s] = K_III;
		else
		{
			cerr << "No Correspondences found for " << s << " " << s + 1 << endl;
			s = models.size() + 1;
			K = vector<vector<Vector2i>>(0);
		}
	}
	return K;
}


// ============================================================================
// COMPUTE FPFH

MatrixXd computeFPFH(PointCloud &model, double radius, KDTreeFlann &distTree)
{
	int N = model.points_.size();
	double s1 = 0.0, s2 = 0.0, s3 = 0.0;	// Tolerances for feature cutoff

	// Allocate space for needed values
	MatrixXd SPFH = MatrixXd::Zero(N, 6);
	vector<vector<int>> Neighbours(N);
	// ================================================================= \\
		// Compute the simplified features for all points.
	for (int idx = 0; idx < N; idx++)
	{
		int p_idx = idx;

		// Read the current point and the normal of the point.
		Vector3d p = model.points_[p_idx];
		Vector3d n = model.normals_[p_idx];

		// Neighbours are the indices of the neighbours in the model.
		vector<int> neighbours;
		vector<double> distances;
		distTree.SearchRadius(p, radius, neighbours, distances);

		// Make sure the point itself is not a neighbour
		for (int k = 0; k < neighbours.size(); k++)
		{
			if (neighbours[k] == p_idx)
			{
				neighbours.erase(neighbours.begin() + k);
				distances.erase(distances.begin() + k);
			}
		}
		if (neighbours.size() == 0)
			Neighbours[idx] = vector<int>{ -1 };
		else
			Neighbours[idx] = neighbours;

		// For each neighbour compute the local SPFH values
		VectorXd spfh = VectorXd::Zero(6);
		for (int k = 0; k < neighbours.size(); k++)
		{
			int pk_idx = neighbours[k];
			Vector3d p_k = model.points_[pk_idx];
			Vector3d n_k = model.normals_[pk_idx];

			// Determine the order of p and p_k
			Vector3d p_i, p_j, n_i, n_j;
			Vector3d line = p - p_k;

			double angle_p = acos(n.dot(line) /
				(n.norm()*line.norm()));
			double angle_pk = acos(n_k.dot(line) /
				(n_k.norm()*line.norm()));
			if (angle_p <= angle_pk)
			{
				p_i = p; p_j = p_k;
				n_i = n; n_j = n_k;
			}
			else {
				p_i = p_k; p_j = p;
				n_i = n_k; n_j = n;
			}


			// Define the Darboux frame
			Vector3d u = n_i;
			Vector3d v = (p_j - p_i).cross(u);
			Vector3d w = u.cross(v);

			// Compute features
			double f_1 = v.dot(n_j);
			double f_2 = (u.dot(p_j - p_i)) / ((p_j - p_i).norm());
			double f_3 = atan2(w.dot(n_j), u.dot(n_j));
			if (line.norm() == 0) {
				f_1 = 0.0; f_2 = 0.0; f_3 = 0.0;
			}

			// Compute simplified histogram
			if (f_1 < s1) spfh(0)++;
			if (f_1 >= s1) spfh(1)++;
			if (f_2 < s2) spfh(2)++;
			if (f_2 >= s2) spfh(3)++;
			if (f_3 < s3) spfh(4)++;
			if (f_3 >= s3) spfh(5)++;
		}

		if (neighbours.size() != 0)
			SPFH.row(idx) = spfh / neighbours.size();
	}

	// ================================================================= \\
		// Compute the FPFH for each point
	MatrixXd FPFH_new = MatrixXd::Zero(N, 6);
	for (int idx = 0; idx < N; idx++)
	{
		int p_idx = idx;
		Vector3d p = model.points_[p_idx];

		VectorXd fpfh = VectorXd::Zero(6);
		size_t K;
		if (Neighbours[idx][0] != -1)
			K = Neighbours[idx].size();
		else K = 0;

		if (K > 0) {
			for (int k = 0; k < K; k++)
			{
				int pk_idx = Neighbours[idx][k];
				Vector3d pk = model.points_[pk_idx];
				fpfh += SPFH.row(pk_idx) / (1 + (p - pk).norm());
			}
			VectorXd spfh = SPFH.row(idx);
			fpfh = spfh + 1.0 / ((double)K)*fpfh;
			FPFH_new.row(p_idx) = fpfh.cwiseMax(1e-6);

		}
		else FPFH_new.row(p_idx) = (SPFH.row(idx)).cwiseMax(1e-6);
	}
	return FPFH_new;
}


// ============================================================================
// COMPUTE PERSISTENT POINTS
void computePersistentPoints(PointCloud &model, vector<int> &P, MatrixXd &FPFH)
{
	// ********************************************************************* \\
	// Initialization of the different values used in the computations.
	double R = 0.5*(model.GetMaxBound() - model.GetMinBound()).norm();
	double ini_R = atof(getenv("INI_R"));	// Maximal proportion of R to use.
	double end_R = atof(getenv("END_R"));	// Minimal proportion of R to use.
	double num_R = atof(getenv("NUM_R"));	// Multiplicative step size for R.
	double alpha = atof(getenv("ALPHA"));	// Cutoff STD to mark persistent.

	int n_scales = 0;
	int N = model.points_.size();

	// Initialize the persistent set as all points in the set.
	P = vector<int>(model.points_.size());

	MatrixXd FPFH_new;
	for (int id = 0; id < P.size(); id++)
		P[id] = id;

	// Precompute the KDTree used for distance search.
	KDTreeFlann distTree;
	distTree.SetGeometry(model);

	// ********************************************************************* \\
	// For each radius determine the FPFH and persistent features.

	double step_r = R * (end_R - ini_R) / num_R;
	double min_r = R * min(ini_R, end_R);
	double max_r = R * max(ini_R, end_R);
	for (double r = ini_R * R; min_r <= r && r <= max_r; r += step_r)
	{
		// ================================================================= \\
		// Compute the FPFH features
		string FPFH_ver = string(getenv("FPFH_VERSION"));
		if (FPFH_ver.compare("open3d") == 0)
		{
			Feature feature_0 = *ComputeFPFHFeature(model);
			FPFH_new = feature_0.data_.transpose();
			r = max_r + 1;
		}
		else
		{
			FPFH_new = computeFPFH(model, r, distTree);
		}

		// ================================================================= \\
		// Compute the mean and standard deviation of the feature histograms.
		VectorXd mu = VectorXd::Zero(FPFH_new.cols());

		// Mean FPFH value
		for (int idx = 0; idx < N; idx++)
		{
			int p_idx = idx;
			VectorXd fpfh_new = FPFH_new.row(p_idx);
			mu += fpfh_new / (double)N;
		}

		// Setup distance from mu.
		VectorXd dist_vec = VectorXd::Zero(N);
		for (int idx = 0; idx < N; idx++)
		{
			int p_idx = idx;
			VectorXd fpfh_new = FPFH_new.row(p_idx);
			dist_vec(idx) = 0.0;
			for (int i = 0; i < FPFH_new.cols(); i++)
				dist_vec(idx) += fpfh_new(i) - mu(i);
		}

		// Standard Deviation of distances
		VectorXd ones = VectorXd::Ones(dist_vec.size());
		dist_vec -= ones * dist_vec.mean();
		double sigma = (dist_vec).norm() / sqrt(P.size() - 1);

		// ================================================================= \\
		// Determine which points have persistent features.
		vector<int> P_new;
		for (int idx = 0; idx < P.size(); idx++)
		{
			int p_idx = P[idx];
			if (abs(dist_vec(p_idx)) > alpha*sigma)
			{
				P_new.push_back(p_idx);
			}
		}

		// Use only persistent features for next iteration.
		if (P_new.size() >= 0.005*model.points_.size())
		{
			P = P_new;
			if (n_scales == 0)
				FPFH = FPFH_new;
			else
				FPFH += FPFH_new;
			n_scales++;
		}
	}

	MatrixXd FPFH_temp = MatrixXd::Zero(P.size(), FPFH_new.cols());
	for (int idx = 0; idx < P.size(); idx++)
	{
		int p_idx = P[idx];
		VectorXd fpfh = FPFH.row(p_idx);
		FPFH_temp.row(idx) = fpfh;
	}
	FPFH = FPFH_temp / (double)n_scales;
}

// ============================================================================
// LIST NEAREST NEIGHBOUR IN FPFH SPACE
vector<Vector2i> nearestNeighbour(vector<int> P_0, vector<int> P_1,
	MatrixXd FPFH_0, KDTreeFlann &KDTree_1)
{
	vector<Vector2i> K_near;

	for (int idx = 0; idx < P_0.size(); idx++)
	{
		int p_idx = P_0[idx];
		VectorXd fpfh_0 = FPFH_0.row(idx);
		vector<int> neighbour;
		vector<double> distance;
		Vector2i k = Vector2i::Zero();

		int knn = 1;
		KDTree_1.SearchKNN(fpfh_0, knn, neighbour, distance);
		k(0) = p_idx;
		k(1) = P_1[neighbour[0]];
		K_near.push_back(k);
	}
	return K_near;
}

// ============================================================================
// COLLECT MUTUAL NEAREST NEIGHBOURS
vector<Vector2i> mutualNN(vector<Vector2i> K_0, vector<Vector2i> K_1)
{
	vector<Vector2i> K_mutual;

	for (int i = 0; i < K_0.size(); i++)
	{
		Vector2i k_0 = K_0[i];
		for (int j = 0; j < K_1.size(); j++)
		{
			Vector2i k_1 = K_1[j];

			if ((k_0(0) == k_1(1)) && (k_0(1) == k_1(0)))
			{
				K_mutual.push_back(k_0);
			}
		}
	}
	return K_mutual;
}
// ============================================================================
// RUN TUPLE TEST
vector<Vector2i> tupleTest(vector<Vector2i> K_II, PointCloud model_0,
	PointCloud model_1)
{
	// Initialize values
	vector<Vector2i> K_III;
	double tau = 0.9;
	double tau_inv = 1.0 / tau;

	// Fill index vector
	vector<int> I(K_II.size());
	iota(I.begin(), I.end(), 0);

	// Do a random shuffle of I
	random_device rd;
	mt19937 g(rd());
	int count = 0;

	// Run through all possible correspondences
	for (size_t i = I.size() - 3; i < I.size() && count < K_II.size() * 1000; )
	{
		shuffle(I.begin(), I.end(), g);
		vector<int> idx = { I[i], I[i + 1], I[i + 2] };
		double r01, r02, r12;

		// Read the p and q points from the two models.
		Vector3d p_0 = model_0.points_[K_II[idx[0]](0)];
		Vector3d p_1 = model_0.points_[K_II[idx[1]](0)];
		Vector3d p_2 = model_0.points_[K_II[idx[2]](0)];
		Vector3d q_0 = model_1.points_[K_II[idx[0]](1)];
		Vector3d q_1 = model_1.points_[K_II[idx[1]](1)];
		Vector3d q_2 = model_1.points_[K_II[idx[2]](1)];


		// Compute ratios
		r01 = (p_0 - p_1).norm() / (q_0 - q_1).norm();
		r02 = (p_0 - p_2).norm() / (q_0 - q_2).norm();
		r12 = (p_1 - p_2).norm() / (q_1 - q_2).norm();

		// Save correspondences if ratios are valid
		if ((tau < r01 && r01 <= tau_inv) &&
			(tau < r02 && r02 <= tau_inv) &&
			(tau < r12 && r12 <= tau_inv))
		{
			K_III.push_back(K_II[idx[0]]);
			K_III.push_back(K_II[idx[1]]);
			K_III.push_back(K_II[idx[2]]);
			I.pop_back(); I.pop_back(); I.pop_back();
			i -= 3;
		}
		else count++;

	}
	return K_III;
}
