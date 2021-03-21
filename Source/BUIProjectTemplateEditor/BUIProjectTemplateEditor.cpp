#include "BUIProjectTemplateEditor.h"


void FBUIProjectTemplateEditor::StartupModule()
{
	FDefaultGameModuleImpl::StartupModule();
}


void FBUIProjectTemplateEditor::ShutdownModule()
{
	FDefaultGameModuleImpl::ShutdownModule();
}


IMPLEMENT_GAME_MODULE( FBUIProjectTemplateEditor, BUIProjectTemplateEditor );

