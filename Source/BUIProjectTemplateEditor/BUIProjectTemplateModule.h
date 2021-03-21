#pragma once

#include "UnrealEd.h"
#include "Engine.h"
#include "IAssetTypeActions.h"
#include "IAssetTools.h"
//#include "ParticleDefinitions.h"
//#include "SoundDefinitions.h"
//#include "Net/UnrealNetwork.h"


class FTitanEditor : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	void AddMenuEntry( FMenuBuilder& MenuBuilder );

	void ShowTitanWindow();
	//void FillSubmenu( FMenuBuilder& MenuBuilder );
	//TSharedRef<IDetailsView> DetailsView;

	TSharedPtr<FStructOnScope> StructToDisplay;

	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}

	//TSharedPtr<class IStructureDetailsView> StructureDetailsView;
	//TSharedRef<IDetailsView> PropertyView;
};
