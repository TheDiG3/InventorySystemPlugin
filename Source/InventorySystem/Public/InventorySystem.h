// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FInventorySystemModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
