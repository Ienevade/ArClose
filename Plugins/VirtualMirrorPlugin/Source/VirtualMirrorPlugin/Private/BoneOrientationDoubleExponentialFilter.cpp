#include "BoneOrientationDoubleExponentialFilter.h"
#include "VirtualMirrorFunctionLibrary.h"
#include "VirtualMirrorPluginPrivatePCH.h"




BoneOrientationDoubleExponentialFilter::BoneOrientationDoubleExponentialFilter(){
	this->init = false;
}


void BoneOrientationDoubleExponentialFilter::Init(){
	// Set some reasonable defaults
	this->Init(0.5f, 0.8f, 0.75f, 0.1f, 0.1f);  //  - Normal
	//this->Init(0.9f, 0.8f, 0.75f, 0.1f, 0.1f); // - Very slow 
}

void BoneOrientationDoubleExponentialFilter::Init(float smoothingValue, float correctionValue, float predictionValue, float jitterRadiusValue, float maxDeviationRadiusValue){
	
	this->smoothParameters.MaxDeviationRadius = maxDeviationRadiusValue; // Size of the max prediction radius Can snap back to noisy data when too high
	this->smoothParameters.Smoothing = smoothingValue;                   // How much soothing will occur.  Will lag when too high
	this->smoothParameters.Correction = correctionValue;                 // How much to correct back from prediction.  Can make things springy
	this->smoothParameters.Prediction = predictionValue;                 // Amount of prediction into the future to use. Can over shoot when too high
	this->smoothParameters.JitterRadius = jitterRadiusValue;             // Size of the radius where jitter is removed. Can do too much smoothing when too high
	this->Reset();
	this->init = true;
}


void BoneOrientationDoubleExponentialFilter::Init(TransformSmoothParameters smoothingParameters){
	this->smoothParameters = smoothingParameters;
this->Reset();
this->init = true;
}


void BoneOrientationDoubleExponentialFilter::Reset(){
	std::array<FilterDoubleExponentialData, JointType_Count> historyEmpty;
	//Zero props
	for (int i = 0; i < JointType_Count; i++){
		historyEmpty[i].RawBoneOrientation = FQuat::Identity;
		historyEmpty[i].FilteredBoneOrientation = FQuat::Identity;
		historyEmpty[i].Trend = FQuat::Identity;
		historyEmpty[i].FrameCount = 0;
	}


	this->history = historyEmpty;
}

void BoneOrientationDoubleExponentialFilter::UpdateFilter(IBody* body, JointOrientation(&orientations)[JointType_Count], std::map<uint64, Vector4> faceOrientations){
	if (!body){
		return;
	}

	BOOLEAN bTracked = false;
	HRESULT hr = body->get_IsTracked(&bTracked);
	if (!SUCCEEDED(hr) || !bTracked){
		return;
	}

	if (this->init == false)
	{
		this->Init(); // initialize with default parameters                
	}

	TransformSmoothParameters tempSmoothingParams;

	// Check for divide by zero. Use an epsilon of a 10th of a millimeter
	this->smoothParameters.JitterRadius = FMath::Max(0.0001f, this->smoothParameters.JitterRadius);

	tempSmoothingParams.Smoothing = this->smoothParameters.Smoothing;
	tempSmoothingParams.Correction = this->smoothParameters.Correction;
	tempSmoothingParams.Prediction = this->smoothParameters.Prediction;
	tempSmoothingParams.JitterRadius = this->smoothParameters.JitterRadius;
	tempSmoothingParams.MaxDeviationRadius = this->smoothParameters.MaxDeviationRadius;

	Joint joints[JointType_Count];
	body->GetJoints(JointType_Count, joints);

	for (int jt = 0; jt < JointType_Count; jt++){

		// If not tracked, we smooth a bit more by using a bigger jitter radius
		// Always filter feet highly as they are so noisy
		if (joints[jt].TrackingState != TrackingState_Tracked || jt == JointType_AnkleLeft || jt == JointType_AnkleRight || jt == JointType_WristLeft || jt == JointType_WristRight || jt == JointType_Head){
			tempSmoothingParams.JitterRadius *= 3.0f;
			tempSmoothingParams.MaxDeviationRadius *= 3.0f;
			
		}
		else{
			tempSmoothingParams.JitterRadius = this->smoothParameters.JitterRadius;
			tempSmoothingParams.MaxDeviationRadius = this->smoothParameters.MaxDeviationRadius;
		}

		//Get corresponding face orientation by trackingID;
		uint64 trackingId;
		body->get_TrackingId(&trackingId);
		Vector4 faceOrientation = faceOrientations.find(trackingId)->second;


		if (!(joints[jt].TrackingState == TrackingState_Inferred)){
			this->FilterJoint(body, orientations, static_cast<JointType>(jt), tempSmoothingParams,faceOrientation);
		}
	}
}


void BoneOrientationDoubleExponentialFilter::FilterJoint(IBody* body, JointOrientation(&orientations)[JointType_Count], JointType jt, TransformSmoothParameters smoothingParameters, Vector4 faceOrientation){
	if (!body)
	{
		return;
	}

	int jointIndex = (int)jt;
	

	JointOrientation jointOrientations[JointType_Count];
	Joint joints[JointType_Count];

	body->GetJointOrientations(JointType_Count, jointOrientations);

	//Inject FACE rotation here

	jointOrientations[JointType_Head].Orientation = faceOrientation;


	///////////////////////////


	body->GetJoints(JointType_Count, joints);

	FQuat filteredOrientation;
	FQuat trend;

	FQuat rawOrientation = FQuat(jointOrientations[jt].Orientation.x, jointOrientations[jt].Orientation.y, jointOrientations[jt].Orientation.z, jointOrientations[jt].Orientation.w);
	FQuat prevFilteredOrientation = this->history[jointIndex].FilteredBoneOrientation;
	FQuat prevTrend = this->history[jointIndex].Trend;
	FVector rawPosition = FVector(joints[jt].Position.X, joints[jt].Position.Y, joints[jt].Position.Z);
	bool orientationIsValid = UKinectV1FunctionLibrary::JointPositionIsValid(rawPosition) && UKinectV1FunctionLibrary::IsTrackedOrInferred(body, jt) && UKinectV1FunctionLibrary::BoneOrientationIsValid(rawOrientation);

	
	//Elbow rotation check after frame 0 only
	Vector4 rawOrientationParentVec = jointOrientations[UKinectV1FunctionLibrary::GetParentBone(jt)].Orientation;
	FQuat rawOrientationParent = FQuat(rawOrientationParentVec.x, rawOrientationParentVec.y, rawOrientationParentVec.z, rawOrientationParentVec.w);

	FQuat rawRelative = rawOrientationParent.Inverse()*rawOrientation;
	FQuat rawRelativePrev = rawOrientationParent.Inverse()*prevFilteredOrientation;
	FRotator rawRelativeRot = rawRelative.Rotator();
	
	
	if (jt == JointType_WristLeft){
		if (rawRelative.Rotator().Pitch > 0){
			rawRelativeRot.Pitch = -rawRelativeRot.Pitch;
			//UE_LOG(KinectV2, Warning, TEXT("Left elbow fixed"));
		}
		
	}

	if (jt == JointType_WristRight){
		if (rawRelative.Rotator().Pitch < 0){
			rawRelativeRot.Pitch = -rawRelativeRot.Pitch;
			//UE_LOG(KinectV2, Warning, TEXT("Right elbow fixed"));
		}
	
		
	}
	

	if (jt == JointType_Head || jt == JointType_Neck){
		
		rawRelativeRot.Pitch = FMath::ClampAngle(rawRelativeRot.Pitch, -45, 45); //RIGHT - LEFT
		rawRelativeRot.Roll = -rawRelativeRot.Roll; //FEL - LE
		rawRelativeRot.Roll = FMath::ClampAngle(rawRelativeRot.Roll, 158, 217); //UP - DOWN
		rawRelativeRot.Yaw = FMath::ClampAngle(rawRelativeRot.Yaw, 150, 210);
		
		//Turn by 180 degrees
		if (rawRelativeRot != FRotator::ZeroRotator){
			rawRelativeRot=rawRelativeRot + FRotator(180, 0, 0);
		}
	}
	rawOrientation = rawOrientationParent * rawRelativeRot.Quaternion();
	/////////////////////////////
	

	if (!orientationIsValid)
	{
		if (this->history[jointIndex].FrameCount > 0)
		{
			rawOrientation = this->history[jointIndex].FilteredBoneOrientation;
			this->history[jointIndex].FrameCount = 0;
		}
	}

	// Initial start values or reset values
	if (this->history[jointIndex].FrameCount == 0)
	{
		// Use raw position and zero trend for first value
		filteredOrientation = rawOrientation;
		trend = FQuat::Identity;
	}
	else if (this->history[jointIndex].FrameCount == 1)
	{
		// Use average of two positions and calculate proper trend for end value
		FQuat prevRawOrientation = this->history[jointIndex].RawBoneOrientation;
		filteredOrientation = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(prevRawOrientation, rawOrientation, 0.5f);

		FQuat diffStarted = UKinectV1FunctionLibrary::RotationBetweenQuaternions(filteredOrientation, prevFilteredOrientation);
		trend = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(prevTrend, diffStarted, smoothingParameters.Correction);
	}
	else
	{
		// First apply a jitter filter
		FQuat diffJitter = UKinectV1FunctionLibrary::RotationBetweenQuaternions(rawOrientation, prevFilteredOrientation);
		float diffValJitter = (float)FMath::Abs(UKinectV1FunctionLibrary::QuaternionAngle(diffJitter));
		
		if (diffValJitter <= smoothingParameters.JitterRadius)
		{
			filteredOrientation = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(prevFilteredOrientation, rawOrientation, diffValJitter / smoothingParameters.JitterRadius);
		}
		else
		{
			filteredOrientation = rawOrientation;
		}
		

		// Now the double exponential smoothing filter
		
		filteredOrientation = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(filteredOrientation, (prevFilteredOrientation*prevTrend), smoothingParameters.Smoothing);
		
		diffJitter = UKinectV1FunctionLibrary::RotationBetweenQuaternions(filteredOrientation, prevFilteredOrientation);
		trend = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(prevTrend, diffJitter, smoothingParameters.Correction);
	}

	// Use the trend and predict into the future to reduce latency
	FQuat predictedOrientation = (filteredOrientation*UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(FQuat::Identity, trend, smoothingParameters.Prediction));

	// Check that we are not too far away from raw data
	FQuat diff = UKinectV1FunctionLibrary::RotationBetweenQuaternions(predictedOrientation, filteredOrientation);
	float diffVal = (float)FMath::Abs(UKinectV1FunctionLibrary::QuaternionAngle(diff));

	if (diffVal > smoothingParameters.MaxDeviationRadius)
	{
		predictedOrientation = UKinectV1FunctionLibrary::EnhancedQuaternionSlerp(filteredOrientation, predictedOrientation, smoothingParameters.MaxDeviationRadius / diffVal);
	}

	predictedOrientation.Normalize();
	filteredOrientation.Normalize();
	trend.Normalize();

	// Save the data from this frame
	this->history[jointIndex].RawBoneOrientation = rawOrientation;
	this->history[jointIndex].FilteredBoneOrientation = filteredOrientation;
	this->history[jointIndex].Trend = trend;
	this->history[jointIndex].FrameCount++;

	
	//Mirror rotation
	//predictedOrientation.Y = -predictedOrientation.Y;
	//predictedOrientation.Z = -predictedOrientation.Z;
	
	// Set the filtered and predicted data back into the bone orientation
	orientations[jt].Orientation = UKinectV1FunctionLibrary::UnrealQuaternionToVector4(predictedOrientation);
	
	
}
