#pragma once

#include "UnrealEd.h"
#include "Engine.h"
//#include "IAssetTypeActions.h"
//#include "IAssetTools.h"


class FBUIProjectTemplateEditor : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
