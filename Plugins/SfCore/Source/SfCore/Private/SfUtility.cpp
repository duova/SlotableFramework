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
	InternalValue = static_cast<uint16>(Value * 100.0);
}

bool FUint16_Quantize100::operator==(const FUint16_Quantize100& Other) const
{
	if (InternalValue == Other.InternalValue) return true;
	return false;
}

FInt16_Quantize10::FInt16_Quantize10(): InternalValue(0)
{
}

float FInt16_Quantize10::GetFloat() const
{
	return static_cast<float>(InternalValue) / 10.0;
}

void FInt16_Quantize10::SetFloat(const float Value)
{
	if (Value > 3276.7)
	{
		InternalValue = 32767;
		return;
	}
	if (Value < -3276.7)
	{
		InternalValue = -32767;
		return;
	}
	InternalValue = static_cast<int16>(Value * 10.0);
}

bool FInt16_Quantize10::operator==(const FInt16_Quantize10& Other) const
{
	if (InternalValue == Other.InternalValue) return true;
	return false;
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
