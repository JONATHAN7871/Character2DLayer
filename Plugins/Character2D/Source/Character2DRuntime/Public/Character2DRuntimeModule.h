#pragma once
#include "Modules/ModuleManager.h"

class FCharacter2DRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};