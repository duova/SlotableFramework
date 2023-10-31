// Fill out your copyright notice in the Description page of Project Settings.


#include "SfUtility.h"


FUint16_Quantize100::FUint16_Quantize100(): InternalValue(0)
{
}

float FUint16_Quantize100::GetFloat() const
{
	return static_cast<float>(InternalValue) / 100.0;
}

void FUint16_Quantize100::SetFloat(const float Value)
{
	if (Value > 655.35)
	{
		InternalValue = 65535;
		return;
	}
	if (Value < 0)
	{
		InternalValue = 0;
		return;
	}
	InternalValue = static_cast<uint8>(Value * 100.0);
}

bool FUint16_Quantize100::operator==(const FUint16_Quantize100& Other) const
{
	if (InternalValue == Other.InternalValue) return true;
	return false;
}

FSfTickFunctionHelper::FSfTickFunctionHelper(): Interval(0), TickCurrentTime(0), TickStartingTime(0),
                                                TickFunction(nullptr)
{
}

void FSfTickFunctionHelper::DriveTick(const float DeltaTime)
{
	TickCurrentTime += DeltaTime;
	if (TickCurrentTime >= Interval)
	{
		TickFunction.ExecuteIfBound(TickCurrentTime - TickStartingTime);
		//We reduce the time by the interval instead of setting it to 0 to ensure that over time the tick rate averages
		//at the target instead of being slightly slower.
		TickCurrentTime -= Interval;
		//We save the time skipped on the new tick (ie. extra time on the last tick) in order to still be able to
		//calculate the correct delta time of each tick.
		TickStartingTime = TickCurrentTime;
	}
}
