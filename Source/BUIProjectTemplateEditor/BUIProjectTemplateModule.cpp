#include "TitanEditor.h"
#include "BYGTitanEditorCommands.h"
#include "PropertyEditorModule.h"
#include "LevelEditor.h"
#include "DataEditorWindow/BYGDataEditorWindow.h"
#include "AssetTypeActions/BYGAssetTypeActions_BYGDataTable.h"
#include "DetailsCustomization/BYGDistributionCustomization.h"
#include "DetailsCustomization/BYGTextStylesheetCustomization.h"
#include "DetailsCustomization/BYGTextStyleCustomization.h"


void FTitanEditor::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();

	// If this turns into a HUGE mess, look at the editor's DetailCustomizations.cpp for how to do it nicer

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked< FPropertyEditorModule >( "PropertyEditor" );
	//PropertyModule.RegisterCustomPropertyTypeLayout( "BYGRichTextStyle", FOnGetPropertyTypeCustomizationInstance::CreateStatic( &FBYGRichTextStyleCustomization::MakeInstance ) );

	// Register Object customizations
	PropertyModule.RegisterCustomClassLayout( "BYGTextStylesheet", FOnGetDetailCustomizationInstance::CreateStatic( &FBYGTextStylesheetCustomization::MakeInstance ) );
	PropertyModule.RegisterCustomClassLayout( "BYGTextStyle", FOnGetDetailCustomizationInstance::CreateStatic( &FBYGTextStyleCustomization::MakeInstance ) );
	//PropertyModule.RegisterCustomPropertyTypeLayout( "BYGTextStyle", FOnGetPropertyTypeCustomizationInstance::CreateStatic( &FBYGTextStyleCustomization::MakeInstance ) );

	PropertyModule.RegisterCustomClassLayout( "BYGRandomDistribution", FOnGetDetailCustomizationInstance::CreateStatic( &FBYGDistributionCustomization::MakeInstance ) );
	PropertyModule.RegisterCustomClassLayout( "BYGRandomDistributionObject", FOnGetDetailCustomizationInstance::CreateStatic( &FBYGDistributionCustomization::MakeInstance ) );

	FBYGTitanEditorCommands::Register();
	PluginCommands = MakeShareable( new FUICommandList );
	// This is how you map Commands to Functions
	PluginCommands->MapAction(
		FBYGTitanEditorCommands::Get().ShowTitanWindow,
		FExecuteAction::CreateRaw( this, &FTitanEditor::ShowTitanWindow )
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>( "LevelEditor" );
	{
		TSharedPtr<FExtender> NewMenuExtender = MakeShareable( new FExtender );
		NewMenuExtender->AddMenuExtension( "LevelEditor",
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw( this, &FTitanEditor::AddMenuEntry ) );
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender( NewMenuExtender );
	}

#if 0
	auto RegisterAssetTypeAction = [this](const TSharedRef<IAssetTypeActions>& InAssetTypeAction)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisteredAssetTypeActions.Add(InAssetTypeAction);
		AssetTools.RegisterAssetTypeActions(InAssetTypeAction);
	};
#endif

	//IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>( "AssetTools" ).Get();
	//RegisterAssetTypeAction( AssetTools, MakeShareable( new FBYGAssetTypeActions_BYGDataTable() ) );



	// Find file related to class name
	// Loop through all entries in the XML
	// Set up the properties of the instance
	// Save the instance
	//for ( TFieldIterator<UProperty> PropIt( Dummy.StaticStruct() ); PropIt; ++PropIt )
	//{
		//UE_LOG( LogTemp, Warning, TEXT( "Name: %s" ), *PropIt->GetNameCPP() );
	//}
	//for ( TFieldIterator<UProperty> PropIt( UBYGTestObject::StaticClass() ); PropIt; ++PropIt )
	//{
		//UE_LOG( LogTemp, Warning, TEXT( "Name: %s" ), *PropIt->GetNameCPP() );
	//}


	PropertyModule.NotifyCustomizationModuleChanged();
}


void FTitanEditor::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();

	FBYGTitanEditorCommands::Unregister();
}


void FTitanEditor::AddMenuEntry( FMenuBuilder& MenuBuilder )
{
	MenuBuilder.BeginSection( "CustomMenu", TAttribute<FText>( FText::FromString( "Tooltip blah" ) ) );

	MenuBuilder.AddMenuEntry( FBYGTitanEditorCommands::Get().ShowTitanWindow );
	//MenuBuilder.AddSubMenu( FText::FromString( "My Submenu" ),
		//FText::FromString( "My submenu toolkit " ),
		//FNewMenuDelegate::CreateRaw( this, &FTitanEditor::FillSubmenu ) );

	MenuBuilder.EndSection();
}


void FTitanEditor::ShowTitanWindow()
{
#if 0
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );

	//FDetailsViewArgs DetailsViewArgs( false, false, true, FDetailsViewArgs::HideNameArea );
	//DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );

	//UBYGPatchNotesData* TestData = LoadPatchNotesJSON();
	//DetailsView->SetObject( GetMutableDefault<UBYGPatchNotesData>() );
	//DetailsView->SetObject( TestData );
	// create struct to display
	FStructureDetailsViewArgs StructureViewArgs;
	StructureViewArgs.bShowObjects = true;
	StructureViewArgs.bShowAssets = true;
	StructureViewArgs.bShowClasses = true;
	StructureViewArgs.bShowInterfaces = true;

	FDetailsViewArgs ViewArgs;
	ViewArgs.bAllowSearch = true;
	ViewArgs.bHideSelectionTip = false;
	ViewArgs.bShowActorLabel = false;
	//ViewArgs.NotifyHook = NotifyHook; // TODO
	StructToDisplay = MakeShareable( new FStructOnScope( FBYGPatchNotesEntryData::StaticStruct(), (uint8*)&PatchNotesEntryData ) );

	StructureDetailsView = PropertyEditorModule.CreateStructureDetailView( ViewArgs, StructureViewArgs, StructToDisplay, NSLOCTEXT("Blah", "Struct", "Struct View" ) );

	//StructureDetailsView->SetStructureData( StructToDisplay );

	//ControlRigEditor.Pin()->SetDetailStruct( MakeShareable( new FStructOnScope( FRigJoint::StaticStruct(), ( uint8* )&RigHierarchy->Joints[ JointIndex ] ) ) );



	//FDetailsViewArgs DetailsViewArgs( /*bUpdateFromSelection=*/ false, /*bLockable=*/ false, /*bAllowSearch=*/ true, NameAreaSettings, /*bHideSelectionTip=*/ true ); // /*InNotifyHook=*/ NotifyHook, /*InSearchInitialKeyFocus=*/ false, /*InViewIdentifier=*/ InArgs._ViewIdentifier );
	//PropertyView = EditModule.CreateDetailView( DetailsViewArgs );
	FString Name;

	TSharedRef<SWindow> Window = SNew( SWindow )
		.Title( FText::FromString( TEXT( "Titan Data" ) ) )
		.ClientSize( FVector2D( 800, 400 ) )
		.SupportsMaximize( false )
		.SupportsMinimize( false )
		[
			SNew( SVerticalBox )
			//+ SVerticalBox::Slot()
				//.HAlign( HAlign_Center )
				//.VAlign( VAlign_Center )
			//[
				//SNew( STextBlock )
				//.Text( FText::FromString( TEXT( "Hello from Slate" ) ) )
			//]
			+ SVerticalBox::Slot()
				.FillHeight( 1.0f )
				[
					StructureDetailsView->GetWidget().ToSharedRef()
				]
			//+ SVerticalBox::Slot()
				//.AutoHeight()
				//[
					//SNew(SButton)
					//.ContentPadding( FMargin( 20.0f, 2.0f ) )
					//.Text( NSLOCTEXT( "Boop", "OkButtonLabel", "Save" ) )
					//.OnClicked( this, &SBYGDataEditorWindow::OnSaveClicked, Name )
				//]
		];


	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild( Window, RootWindow.ToSharedRef() );
#endif
}



IMPLEMENT_GAME_MODULE( FTitanEditor, TitanEditor );

