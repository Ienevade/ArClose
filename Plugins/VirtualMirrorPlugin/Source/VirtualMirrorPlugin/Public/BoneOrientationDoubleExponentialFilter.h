#pragma once

#include <array>
#include <map>
#include "TransformSmoothParameters.h"

/// Implementation of a Holt Double Exponential Smoothing filter for orientation. The double 
/// exponential smooths the curve and predicts. There is also noise jitter removal. And maximum
/// prediction bounds.  The parameters are commented in the Init function.
	
	
class BoneOrientationDoubleExponentialFilter
	{
		
	private:
		/// Historical Filter Data.  
		struct FilterDoubleExponentialData
		{
			/// Gets or sets Historical Position.  
			FQuat RawBoneOrientation;

			/// Gets or sets Historical Filtered Position.  
			FQuat FilteredBoneOrientation;

			/// Gets or sets Historical Trend.  
			FQuat Trend;

			/// Gets or sets Historical FrameCount.  
			unsigned int FrameCount;
		};
		
		/// The previous filtered orientation data.
		std::array<FilterDoubleExponentialData, JointType_Count> history;

		/// The transform smoothing parameters for this filter.
		TransformSmoothParameters smoothParameters;

		/// True when the filter parameters are initialized.
		bool init;
		
		

		
	public:
		/// Initializes a new instance of the class.
		BoneOrientationDoubleExponentialFilter();
		
		/// Initialize the filter with a default set of TransformSmoothParameters.
		void Init();
	
		/// Initialize the filter with a set of manually specified TransformSmoothParameters.

		/// <param name="smoothingValue">Smoothing = [0..1], lower values is closer to the raw data and more noisy.</param>
		/// <param name="correctionValue">Correction = [0..1], higher values correct faster and feel more responsive.</param>
		/// <param name="predictionValue">Prediction = [0..n], how many frames into the future we want to predict.</param>
		/// <param name="jitterRadiusValue">JitterRadius = The deviation angle in radians that defines jitter.</param>
		/// <param name="maxDeviationRadiusValue">MaxDeviation = The maximum angle in radians that filtered positions are allowed to deviate from raw data.</param>
		void Init(float smoothingValue, float correctionValue, float predictionValue, float jitterRadiusValue, float maxDeviationRadiusValue);
		

		/// Initialize the filter with a set of TransformSmoothParameters.
		/// <param name="smoothingParameters">The smoothing parameters to filter with.</param>
		void Init(TransformSmoothParameters smoothingParameters);
		
		/// Resets the filter to default values.
		void Reset();
		
		/// DoubleExponentialJointOrientationFilter - Implements a double exponential smoothing filter on the skeleton bone orientation quaternions.
		/// <param name="body">The Body to filter.</param>
		
		void UpdateFilter(IBody* body, JointOrientation(&orientations)[JointType_Count], std::map<uint64, Vector4> faceOrientations);

		
		
		
	protected:

		/// Update the filter for one joint.  
		
		/// <param name="body">The Body to filter.</param>
		/// <param name="jt">The Skeleton Joint index to filter.</param>
		/// <param name="smoothingParameters">The Smoothing parameters to apply.</param>
		void FilterJoint(IBody* body, JointOrientation(&orientations)[JointType_Count],  JointType jt, TransformSmoothParameters smoothingParameters, Vector4 faceOrientation);

		
};
