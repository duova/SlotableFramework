// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FormQueryComponent.h"
#include "UObject/Object.h"
#include "TestQuery.generated.h"

UCLASS()
class SFTESTS_API UTestQuery : public USfQuery
{
	GENERATED_BODY()

public:

	//We declare two matching delegates. An input delegate that is added to a multicast delegate on the query.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTestQueryMulticastDelegate);
	DECLARE_DYNAMIC_DELEGATE(FTestQueryInputDelegate);

	//Multicast delegate is what should be called when the query succeeds.
	FTestQueryMulticastDelegate TestQueryDelegate;

	//This is the function used to bind the delegate of a query to a blueprint event.
	UFUNCTION(BlueprintCallable, Meta=(DefaultToSelf="Dependent"))
	static void BindTestQuery(const UConstituent* Dependent, const FTestQueryInputDelegate EventToBind)
	{
		static_cast<UTestQuery*>(Dependent->GetQuery(StaticClass()))->TestQueryDelegate.Add(EventToBind);
	};
	
	UTestQuery();

	//Overriden functions used to perform the query.
	virtual void PerformCheck(float DeltaTime, UFormCoreComponent* FormCore) override;

	virtual void Initialize() override;

	virtual void Deinitialize() override;
};
