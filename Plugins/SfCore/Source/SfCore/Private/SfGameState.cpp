// Fill out your copyright notice in the Description page of Project Settings.


#include "SfGameState.h"

float ASfGameState::GetRawReplicatedServerWorldTime() const
{
	return ReplicatedWorldTimeSecondsDouble;
}
