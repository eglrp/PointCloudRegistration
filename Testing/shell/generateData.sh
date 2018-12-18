#!/bin/sh
echo "====================================================================="
echo "generateData.sh:                                                     "
echo "   This is the test generateData. This test will display the effect  "
echo "   of several settings for the data generation program.              "
echo "                                                                     "

DAT=../../data/
FIG=../../figures/GenerateData.exe
MAT=../../matlab

export INPUT_PATH=$DAT
export OUTPUT_PATH=./
# ==============================================================================
# Generate all the data needed

echo "Input and output paths defined by:                                   "
echo "Input : $INPUT_PATH                                                  "
echo "Output: $OUTPUT_PATH                                                 "
echo "                                                                     "

# Clean model
echo "Fetching clean model"
cp $DAT/bunny.ply bunnyClean.ply
MODEL="'bunnyClean', "

echo "====================================================================="
echo "Commencing tests:                                                    "

# Test the Gaussian noise
echo "   Gaussian noise.                                                   "
export NOISE_TYPE=gaussian
export NOISE_STRENGTH=1.5
./GenerateData.exe bunny.ply bunnyGaussian.ply

MODEL+="'bunnyGaussian', "

# Test the Outlier noise
echo "   Outlier addition.                                                 "
export NOISE_TYPE=outliers
export OUTLIER_AMOUNT=5.0
./GenerateData.exe bunny.ply bunnyOutliers.ply

MODEL+="'bunnyOutliers', "

# Test combination noise
echo "   Combined noise and outlier.                                       "
export NOISE_TYPE=both
export NOISE_STRENGTH=2.0
export OUTLIER_AMOUNT=5.0
./GenerateData.exe bunny.ply bunnyNoise.ply

MODEL+="'bunnyNoise', "

# Test transformation
echo "   Transformation.                                                   "
export NOISE_TYPE=none
export ROTATION="0.52,0.52,0.79" # degrees: 30, 30, 45
export TRANSLATION="0.0,0.0,0.0"
./GenerateData.exe bunny.ply bunnyTransform.ply

MODEL+="'bunnyTransform' "

if [ -s error.err ] ; then
	exit
fi
# ==============================================================================
# Export the figures using matlab
echo "Running matlab to complete visualisation.                            "
mkdir fig $FIG -p
matlab -wait -nodesktop -nosplash -r "addpath('$MAT');
	renderModel({$MODEL},'./','fig');
	exit;" 
mv -ft $FIG fig/*
rm -fr *.ply *.exe *.sh fig
echo "Results placed in folder:                                            "
echo $FIG
echo "====================================================================="
