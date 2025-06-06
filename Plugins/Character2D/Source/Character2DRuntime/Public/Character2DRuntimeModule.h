#pragma once
#include "Modules/ModuleManager.h"

class CHARACTER2DRUNTIME_API FCharacter2DRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override {}
};