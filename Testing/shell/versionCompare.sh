#!/bin/sh
#==============================================================================
# Define Preparation of data ect.

Prepare()
{
	# Setup folders needed
	TNAME=`basename -- "$0"`
	FIG=../../figures/${TNAME%.*}
	DAT=../../data
	MAT=../../matlab

	mkdir -p fig $FIG $FIG/data dat

	# Define input and output paths
	export INPUT_PATH="dat/"
	export OUTPUT_PATH="dat/"

	echo "Input and output paths defined by:"
	echo "   Input : $INPUT_PATH"
	echo "   Output: $OUTPUT_PATH"
	echo ' '

	# Fetch the data needed from the data folder
	cp $DAT/bunnyPartial1.ply dat/bunnyClean.ply
	cp $DAT/bunnyPartial2.ply dat/bunnyTransform.ply

	# Alter the moddels as needed
	ROTATION="0.52,0.52,0.79" \
	TRANSLATION="0.0,0.0,0.0" \
		./GenerateData.exe bunnyTransform.ply bunnyTransform.ply

	ROTATION="0.32,0.32,0.59" \
	TRANSLATION="0.0,0.2,0.01" \
		./GenerateData.exe bunnyclean.ply bunnyTransform2.ply

}

# End of Preparation
#==============================================================================
# Define the actual test part of the script 

Program()
{
	# Tests using the bunny data ----------------------------------------------
	export ALPHA="1.5"

	echo 'Testing local implementation'
	start=`date +%s.%N`

	FPFH_VERSION=local \
	FGR_VERSION=local \
	OUTPUT_NAME=local \
		./Registration.exe bunny

	end=`date +%s.%N`
	runtime=$(echo $end $start | awk '{ printf "%f", $1 - $2 }')
	echo ' '
	echo "Local: $runtime"
	echo ' '
	echo ---------------------------------------------------------------

	echo 'Testing FPFH open3d'
	start=`date +%s.%N`
	
	FPFH_VERSION=open3d \
	FGR_VERSION=local \
	OUTPUT_NAME=fpfh_open3d \
		./Registration.exe bunny

	end=`date +%s.%N`
	runtime=$(echo $end $start | awk '{ printf "%f", $1 - $2 }')
	echo ' '
	echo "FPFH: $runtime"
	echo ' '
	echo ---------------------------------------------------------------

	echo 'Testing FGR open3d'
	start=`date +%s.%N`

	FPFH_VERSION=open3d \
	FGR_VERSION=open3d \
	OUTPUT_NAME=fgr_open3d \
		./Registration.exe bunny

	end=`date +%s.%N`
	runtime=$(echo $end $start | awk '{ printf "%f", $1 - $2 }')
	echo ' '
	echo "FGR: $runtime"

	# -------------------------------------------------------------------------

}

# End of Program
#==============================================================================
# Define Visualize

Visualize()
{
	echo ' '
	echo Visualizing
	MATOPT="-wait -nodesktop -nosplash"
	MATTESTS="'local','fpfh_open3d','fgr_open3d'"
	matlab $MATOPT \
	-r "addpath('$MAT');displayRegistration({$MATTESTS},'dat/','fig/');exit"
}

# End of Visualize
#==============================================================================
# Define Finalize

Finalize()
{
	mv -ft $FIG fig/*
	mv -ft $FIG/data dat/*
	rm -fr *.exe *.sh fig dat

	echo '   Figures moved to $FIG.'
	echo '   Data used located in $FIG/data'
	echo 'Test concluded successfully.'
}

# End of Visualize
#==============================================================================
# Define Early Termination

Early()
{
	rm -fr *.ply *.exe *.sh fig dat
	echo ' '
	echo ' ================= WARNING: EARLY TERMINATION ================= '
	cat error.err 
	echo ' ===================== ERRORS SHOWN ABOVE ===================== '
	echo ' '
}

# End of Early Termination
#==============================================================================
# Call Functions
echo __________________________________________________________________________
echo 'Preparing data and folders'
echo ' '

Prepare

echo ' '
echo __________________________________________________________________________
echo 'Commencing tests:'
echo ' '
test_start=`date +%s.%N`

Program

test_end=`date +%s.%N`
runtime=$(echo $test_end $test_start | awk '{ printf "%f", $1 - $2 }')
echo "Computation time for test: $runtime seconds."
if [ -s error.err ] ; then
	Early
	exit
fi

echo ' '
echo __________________________________________________________________________

Visualize

echo ' '
echo __________________________________________________________________________
echo ' '
echo Finalizing

Finalize

echo ' '
echo __________________________________________________________________________
# ==============================   End of File   ==============================