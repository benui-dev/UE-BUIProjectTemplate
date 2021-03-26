#pragma once

#include "Misc/AutomationTest.h"

class FBUIAutomationTestBase : public FAutomationTestBase
{

public:
	FBUIAutomationTestBase( const FString& InName, bool bIsComplex )
		: FAutomationTestBase( InName, bIsComplex )
	{
		//bSuppressLogs = true;
	}

	virtual ~FBUIAutomationTestBase() override
	{
	}

	//virtual bool SuppressLogs() override
	//{
		//return true;
	//}
};
