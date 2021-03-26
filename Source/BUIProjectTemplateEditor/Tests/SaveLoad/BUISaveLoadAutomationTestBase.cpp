#include "BUISaveLoadAutomationTestBase.h"

#include "CoreMinimal.h"

#include <Tests/AutomationEditorCommon.h>
#include <FunctionalTestBase.h>

FBUISaveLoadAutomationTestBase::FBUISaveLoadAutomationTestBase( const FString& InName, const bool bInComplexTask )
	: FBUIAutomationTestBase( InName, bInComplexTask )
{
}

void FBUISaveLoadAutomationTestBase::GetTests( TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands ) const
{
	for ( const auto& Pair : TestData )
	{
		OutBeautifiedNames.Add( Pair.Key );
		OutTestCommands.Add( Pair.Key );
	}
}

bool FBUISaveLoadAutomationTestBase::RunTest( const FString& Parameters )
{
	const auto& TestDatum = TestData[ Parameters ];

	#if 0
	// Better to create this here every time than store centrally and have issues where the
	// default properties cannot be found
	UBUIRichTextStylesheet* DefaultStylesheet = NewObject<UBUIRichTextStylesheet>();
	{
		UBUIRichTextStyle* Style = NewObject<UBUIRichTextStyle>();
		Style->SetID( "default" );
		DefaultStylesheet->AddStyle( Style );
		DefaultStylesheet->SetDefaultStyleName("default");
	}
	{
		UBUIRichTextStyle* Style = NewObject<UBUIRichTextStyle>();
		Style->SetID( "strong" );
		Style->SetDisplayType( EBUIStyleDisplayType::Inline );
		Style->SetShortcut( "*" );
		DefaultStylesheet->AddStyle( Style );
	}
	{
		UBUIRichTextStyle* Style = NewObject<UBUIRichTextStyle>();
		Style->SetID( "h1" );
		Style->SetDisplayType( EBUIStyleDisplayType::Block );
		Style->SetShortcut( "#" );
		DefaultStylesheet->AddStyle( Style );
	}
	{
		UBUIRichTextStyle* Style = NewObject<UBUIRichTextStyle>();
		Style->SetID( "h2" );
		Style->SetDisplayType( EBUIStyleDisplayType::Block );
		Style->SetShortcut( "##" );
		DefaultStylesheet->AddStyle( Style );
	}

	UBUISaveLoadAutomation* Block = NewObject<UBUISaveLoadAutomation>();
	Block->SetRichTextStylesheet( DefaultStylesheet );

	TSharedRef<FBUIRichTextMarkupParser> Parser = FBUIRichTextMarkupParser::Create( Block, "s" );
	TArray<FBUITextBlockInfo> TextBlocks = Parser.Get().SplitIntoBlocks( TestDatum.Input );
	FString None = "None";

	TestEqual( TestDatum.Input + " block count", TextBlocks.Num(), TestDatum.ExpectedTextBlockInfo.Num() );
	for ( int32 i = 0; i < FMath::Max<int32>( TextBlocks.Num(), TestDatum.ExpectedTextBlockInfo.Num() ); ++i )
	{
		const FString Output = TextBlocks.IsValidIndex( i ) ? TextBlocks[ i ].RawText : None;
		const FString Expected = TestDatum.ExpectedTextBlockInfo.IsValidIndex( i ) ? TestDatum.ExpectedTextBlockInfo[ i ].RawText : None;
		const int32 OutputCount = TextBlocks.IsValidIndex( i ) ? TextBlocks[ i ].StylesApplied.Num() : INDEX_NONE;
		const int32 ExpectedCount = TestDatum.ExpectedTextBlockInfo.IsValidIndex( i ) ? TestDatum.ExpectedTextBlockInfo[ i ].StylesApplied.Num() : INDEX_NONE;

		TestEqual( FString::Printf( TEXT( "%s, block #%d, raw text" ), *TestDatum.Input, i ), Output, Expected );
		TestEqual( FString::Printf( TEXT( "%s, block #%d, style count" ), *TestDatum.Input, i ), OutputCount, ExpectedCount );

		for ( int32 j = 0; j < FMath::Max<int32>( OutputCount, ExpectedCount ); ++j )
		{
			FString OutputStyle = TextBlocks.IsValidIndex( i ) && TextBlocks[ i ].StylesApplied.IsValidIndex( j ) ? TextBlocks[ i ].StylesApplied[ j ].ToString() : None;
			FString ExpectedStyle = TestDatum.ExpectedTextBlockInfo.IsValidIndex( i ) && TestDatum.ExpectedTextBlockInfo[ i ].StylesApplied.IsValidIndex( j ) ? TestDatum.ExpectedTextBlockInfo[ i ].StylesApplied[ j ].ToString() : None;

			TestEqual( FString::Printf( TEXT( "%s, block #%d, style #%d" ), *TestDatum.Input, i, j ),
				OutputStyle,
				ExpectedStyle );
		}
	}
	#endif

	return true;
}

static const int TestFlags = (
	EAutomationTestFlags::EditorContext
	| EAutomationTestFlags::CommandletContext
	| EAutomationTestFlags::ClientContext
	| EAutomationTestFlags::ProductFilter );

IMPLEMENT_CUSTOM_COMPLEX_AUTOMATION_TEST( FBUISaveLoadAutomationTest, FBUISaveLoadAutomationTestBase, "BUI.SaveLoad", TestFlags )
void FBUISaveLoadAutomationTest::GetTests( TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands ) const
{
	FBUISaveLoadAutomationTestBase::GetTests( OutBeautifiedNames, OutTestCommands );
}

bool FBUISaveLoadAutomationTest::RunTest( const FString& Parameters )
{
	return FBUISaveLoadAutomationTestBase::RunTest( Parameters );
}

