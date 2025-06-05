#include "Character2DAssetEditorToolkit/Slate/SCharacter2DPresetPanel.h"
#include "Data/Character2DBasePreset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PaperSprite.h"
#include "Widgets/Layout/SScrollBox.h"
#include "EditorStyleSet.h"
#include "Character2DAssetEditorToolkit/SCharacter2DSpriteTile.h"
#include "Data/Character2DPosePreset.h"

/* ─────────────────────────  Construct  ───────────────────────── */
void SCharacter2DPresetPanel::Construct(const FArguments& InArgs)
{
	Asset  = InArgs._CharacterAsset;
	Mode   = InArgs._Mode;
	OnChosen = InArgs._OnChosen;

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SAssignNew(Container, SVerticalBox)   // Container хранит наши тайлы
		]
	];

	Refresh();
}

/* ─────────────────────────  Refresh  ─────────────────────────── */
void SCharacter2DPresetPanel::Refresh()
{
	if (!Container.IsValid()) return;

	Container->ClearChildren();

	TArray<UObject*> Found;
	CollectPresets(Found);

	for (UObject* Obj : Found)
	{
		Container->AddSlot().AutoHeight().Padding(2)
		[
			MakeTile(Obj)
		];
	}
}

/* ───────────────  Сбор пресетов (простая фильтрация) ─────────── */
void SCharacter2DPresetPanel::CollectPresets(TArray<UObject*>& Out)
{
	const FString BasePath = TEXT("/Game/Character2D/Presets");

	FARFilter Filter;
	Filter.PackagePaths.Add(*BasePath);
	Filter.bRecursivePaths = true;

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	ARM.Get().GetAssets(Filter, Assets);

	for (const FAssetData& AD : Assets)
	{
		UObject* Obj = AD.GetAsset();
		if (!Obj) continue;

		if (Mode == ECharacter2DEditMode::Pose && Obj->IsA<UCharacter2DPosePreset>())
			Out.Add(Obj);
		else if (Mode != ECharacter2DEditMode::Pose && Obj->IsA<UCharacter2DPartPreset>())
		{
			UCharacter2DPartPreset* P = Cast<UCharacter2DPartPreset>(Obj);
			if (P && P->Part == Mode) Out.Add(P);
		}
	}
}

/* ─────────────────────  Создаём плитку  ──────────────────────── */
TSharedRef<SWidget> SCharacter2DPresetPanel::MakeTile(UObject* Preset)
{
	UPaperSprite* PreviewSprite = nullptr;
	FText         Label        = FText::FromName(Preset->GetFName());

	if (UCharacter2DPartPreset* Part = Cast<UCharacter2DPartPreset>(Preset))
	{
		if (Part->Mesh && Part->Mesh->GetNumSourceModels() > 0)
			PreviewSprite = nullptr; // для SkeletalMesh обычно нет превью-спрайта
	}
	else if (Cast<UCharacter2DPosePreset>(Preset))
	{
		/* Пока не делаем превью позы — оставляем пусто */
	}

	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "HoverOnlyToolButton")   // ← замена
		.OnClicked_Lambda([this, Preset]()
		{
			if (OnChosen.IsBound()) OnChosen.Execute(Preset);
			return FReply::Handled();
		})
		[
			SNew(SCharacter2DSpriteTile)
			.Label(Label)
			.PreviewTexture(PreviewSprite ? PreviewSprite->GetBakedTexture() : nullptr)
		];
}
