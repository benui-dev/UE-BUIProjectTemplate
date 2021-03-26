#pragma once

#include "../BUIAutomationTestBase.h"


class FBUISaveLoadAutomationTestBase : public FBUIAutomationTestBase
{
public:
	FBUISaveLoadAutomationTestBase( const FString& InName, bool bIsComplex );

	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override;
	virtual bool RunTest( const FString& Parameters ) override;

protected:
	struct FBUISaveLoadTestData
	{
		const FString Description;
		const FString Input;
	};

	TMap<FString, FBUISaveLoadTestData> TestData;
};