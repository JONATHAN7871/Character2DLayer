#include "Character2DActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Curves/CurveFloat.h"
#include "PaperFlipbook.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacter2DActor, Log, All);

ACharacter2DActor::ACharacter2DActor(const FObjectInitializer& ObjInit)
    : Super(ObjInit)
{
    PrimaryActorTick.bCanEverTick = true;

    // Корень и таймлайн-компоненты
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    BlinkTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BlinkTimeline"));
    TalkTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TalkTimeline"));

    // Кривые для анимаций
    BlinkCurve = CreateDefaultSubobject<UCurveFloat>(TEXT("BlinkCurve"));
    TalkCurve = CreateDefaultSubobject<UCurveFloat>(TEXT("TalkCurve"));

    // Спрайты и остальное
    SetupComponents();
}

void ACharacter2DActor::SetCharacterAsset(UCharacter2DAsset* NewAsset, bool bPreserveAnimationState)
{
    if (!IsValid(NewAsset) || NewAsset == CharacterAsset)
    {
        if (NewAsset == CharacterAsset)
        {
            UE_LOG(LogCharacter2DActor, Log, TEXT("SetCharacterAsset: Attempted to set the same asset. No changes were made."));
        }
        return;
    }

    const bool bRestoreBlink = bPreserveAnimationState && bBlinkingActive;
    const bool bRestoreTalk = bPreserveAnimationState && bTalkingActive;

    // Полностью останавливаем анимации и сбрасываем состояние, связанное со СТАРЫМ ассетом
    StopBlinking();
    StopTalking();
    StopAllAnimationsForRefresh();
    OriginalEyelidsSprite = nullptr;
    OriginalMouthSprite = nullptr;
    
    CharacterAsset = NewAsset;
    RefreshFromAsset();

    // Безопасно перезапускаем анимации, если нужно, используя данные из НОВОГО ассета
    if (bRestoreBlink)
    {
        EnableBlinking(true);
    }
    if (bRestoreTalk)
    {
        EnableTalking(true);
    }
}

void ACharacter2DActor::RestoreEyelidsAfterBlink()
{
    if (IsValid(SpriteEyelids) && CharacterAsset)
    {
        // Восстанавливаем либо сохраненный оригинальный спрайт, либо спрайт из ассета
        SpriteEyelids->SetSprite(OriginalEyelidsSprite ? OriginalEyelidsSprite : CharacterAsset->GetEyelidsSprite().Sprite);
        
        // Корректно устанавливаем видимость на основе данных ассета
        SpriteEyelids->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Eyelids) && bSpritesVisible);
    }
}

void ACharacter2DActor::StartTalking()
{
    if (!IsValid(SpriteMouth) || !CharacterAsset || !CharacterAsset->GetTalkSettings().TalkFlipbook) return;
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("StartTalking called"));
    
    bIsTalking = true;
    CurrentTalkFlipbook = CharacterAsset->GetTalkSettings().TalkFlipbook;
    OnTalkStarted.Broadcast();

    // Настраиваем естественную анимацию разговора
    HandleTalkTimelineUpdate(0.0f); // Устанавливаем первый кадр
    HandleTalkTimelineFinished(); // Планируем следующую смену
}

void ACharacter2DActor::StopTalking()
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("StopTalking called"));
    
    bIsTalking = false;
    GetWorldTimerManager().ClearTimer(TalkTimerHandle);
    
    if (TalkTimeline->IsPlaying())
    {
        TalkTimeline->Stop();
    }
    
    CurrentTalkFlipbook = nullptr;
    RestoreMouthAfterTalk();
    OnTalkStopped.Broadcast();
}

void ACharacter2DActor::HandleTalkTimelineUpdate(float Value)
{
    if (!bIsTalking || !CurrentTalkFlipbook || !IsValid(SpriteMouth)) 
    {
        return;
    }

    const auto& Settings = CharacterAsset->GetTalkSettings();
    const int32 NumFrames = CurrentTalkFlipbook->GetNumFrames();
    
    if (NumFrames > 0)
    {
        // Проверяем шанс повторения кадра для более естественной речи
        static int32 LastFrameIndex = -1;
        int32 NewFrameIndex = LastFrameIndex;
        
        if (FMath::FRand() > Settings.FrameRepeatChance || LastFrameIndex == -1)
        {
            // Выбираем новый кадр, отличный от предыдущего
            do {
                NewFrameIndex = FMath::RandRange(0, NumFrames - 1);
            } while (NewFrameIndex == LastFrameIndex && NumFrames > 1);
        }
        
        LastFrameIndex = NewFrameIndex;
        
        if (UPaperSprite* MouthSprite = CurrentTalkFlipbook->GetSpriteAtFrame(NewFrameIndex))
        {
            SpriteMouth->SetSprite(MouthSprite);
        }
    }
}

void ACharacter2DActor::HandleTalkTimelineFinished()
{
    if (!bIsTalking || !CharacterAsset) return;
    
    // Планируем следующую смену с вариативным интервалом
    const auto& Settings = CharacterAsset->GetTalkSettings();
    const float NextDelay = FMath::FRandRange(Settings.MouthChangeIntervalMin, Settings.MouthChangeIntervalMax);
    
    GetWorldTimerManager().SetTimer(TalkTimerHandle, [this]()
    {
        if (bIsTalking)
        {
            HandleTalkTimelineUpdate(0.0f);
            HandleTalkTimelineFinished();
        }
    }, NextDelay, false);
}

void ACharacter2DActor::RestoreMouthAfterTalk()
{
    if (IsValid(SpriteMouth))
    {
        if (OriginalMouthSprite)
        {
            SpriteMouth->SetSprite(OriginalMouthSprite);
        }
        else if (CharacterAsset)
        {
            SpriteMouth->SetSprite(CharacterAsset->GetMouthSprite().Sprite);
        }
        
        if (CharacterAsset)
        {
            SpriteMouth->SetVisibility(CharacterAsset->SpriteStructure.Head.GetFinalChildVisibility(CharacterAsset->SpriteStructure.Head.Mouth) && bSpritesVisible);
        }
    }
}

void ACharacter2DActor::SetSpritesVisible(bool bVisible)
{
    bSpritesVisible = bVisible;
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents(true, true)) 
    { 
        if(Component) Component->SetVisibility(bVisible && Component->GetSprite() != nullptr); 
    }
    if(CharacterAsset) SetupHeadHierarchy();
}

void ACharacter2DActor::SetSkeletalVisible(bool bVisible)
{
    bSkeletalVisible = bVisible;
    for (USkeletalMeshComponent* Component : GetAllSkeletalComponents()) 
    { 
        if(Component) Component->SetVisibility(bVisible); 
    }
}

void ACharacter2DActor::SetBothVisible(bool bSprites, bool bSkeletal) 
{ 
    SetSpritesVisible(bSprites); 
    SetSkeletalVisible(bSkeletal); 
}

void ACharacter2DActor::RefreshFromAsset()
{
    if (!CharacterAsset) 
    { 
        UE_LOG(LogCharacter2DActor, Warning, TEXT("RefreshFromAsset: CharacterAsset is null")); 
        return; 
    }
    
    const FTransform SavedTransform = GetActorTransform();
    const bool bWasHidden = IsHidden();
    const bool bWasBlinkingActive = bBlinkingActive;
    const bool bWasTalkingActive = bTalkingActive;
    
    StopAllAnimationsForRefresh();
    OnConstruction(SavedTransform);
    
    SetActorTransform(SavedTransform);
    SetActorHiddenInGame(bWasHidden);
    
    bBlinkingActive = bWasBlinkingActive;
    bTalkingActive = bWasTalkingActive;

    if (GetWorld() && GetWorld()->IsGameWorld())
    {
        EnableBlinking(bBlinkingActive);
        EnableTalking(bTalkingActive);
    }
    else
    {
        if (bBlinkingActive && !bIsBlinking) StartBlinking();
        if (bTalkingActive && !bIsTalking) StartTalking();
    }
}

void ACharacter2DActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    StopAllAnimationsForRefresh();
}

void ACharacter2DActor::StopAllAnimationsForRefresh()
{
    GetWorldTimerManager().ClearAllTimersForObject(this);

    if (BlinkTimeline && BlinkTimeline->IsPlaying()) BlinkTimeline->Stop();
    if (TalkTimeline && TalkTimeline->IsPlaying()) TalkTimeline->Stop();
    
    bIsBlinking = false;
    bIsTalking = false;
}

void ACharacter2DActor::UpdateFromAssetPreserveState()
{
    if (!CharacterAsset) return;
    TMap<TObjectPtr<UPaperSpriteComponent>, TObjectPtr<UPaperSprite>> SavedSprites;
    for(auto* Comp : GetAllSpriteComponents(true, true)) 
    { 
        if(Comp) SavedSprites.Add(Comp, Comp->GetSprite()); 
    }
    RefreshFromAsset();
    for (const auto& Pair : SavedSprites) 
    { 
        if (Pair.Key && Pair.Value) Pair.Key->SetSprite(Pair.Value); 
    }
}

void ACharacter2DActor::StoreOriginalValues()
{
    OriginalActorLocation = GetActorLocation();
    OriginalActorScale = GetActorScale3D();
    OriginalSpriteColors.Empty();
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents(true, true)) 
    { 
        if(Component) OriginalSpriteColors.Add(Component, Component->GetSpriteColor()); 
    }
}

TArray<UPaperSpriteComponent*> ACharacter2DActor::GetAllSpriteComponents(bool bIncludeEffects, bool bIncludeShadow) const
{
    TArray<UPaperSpriteComponent*> Components = {SpriteBody, SpriteArms, SpriteHead, SpriteEyebrow, SpriteEyes, SpriteEyelids, SpriteMouth};
    if(bIncludeEffects) Components.Append({SpriteEffect1, SpriteEffect2, SpriteEffect3});
    if(bIncludeShadow) Components.Add(SpriteShadow);
    return Components;
}

TArray<USkeletalMeshComponent*> ACharacter2DActor::GetAllSkeletalComponents() const 
{ 
    return {BodyComponent, ArmsComponent, HeadComponent}; 
}

USkeletalMeshComponent* ACharacter2DActor::GetSkeletalComponentByTarget(ECharacter2DAttachmentTarget Target) const
{
    switch (Target) 
    { 
        case ECharacter2DAttachmentTarget::Body: return BodyComponent; 
        case ECharacter2DAttachmentTarget::Arms: return ArmsComponent; 
        case ECharacter2DAttachmentTarget::Head: return HeadComponent; 
        default: return nullptr; 
    }
}

USkeletalMeshComponent* ACharacter2DActor::GetSkeletalComponentByAttachmentTarget(ECharacter2DSkeletalAttachmentTarget Target) const
{
    switch (Target) 
    { 
        case ECharacter2DSkeletalAttachmentTarget::Body: return BodyComponent; 
        default: return nullptr; 
    }
}

bool ACharacter2DActor::HasValidSprites() const 
{ 
    return CharacterAsset && CharacterAsset->HasValidSpriteConfiguration(); 
}

bool ACharacter2DActor::HasValidSkeletalMeshes() const 
{ 
    return CharacterAsset && CharacterAsset->HasValidSkeletalConfiguration(); 
}

void ACharacter2DActor::AttachSkeletalComponentToSocket(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part)
{
    if (!Component || !CharacterAsset || Part.AttachmentTarget == ECharacter2DSkeletalAttachmentTarget::None) 
    {
        return;
    }
    
    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByAttachmentTarget(Part.AttachmentTarget);
    if (AttachComponentToSocket(Component, TargetComponent, Part.SocketName, Part.bUseSocketTransform))
    {
        UE_LOG(LogCharacter2DActor, Log, TEXT("Attached skeletal component %s to socket %s"), 
               *Component->GetName(), *Part.SocketName.ToString());
    }
    else
    {
        UE_LOG(LogCharacter2DActor, Warning, TEXT("Failed to attach skeletal component %s to socket %s"), 
               *Component->GetName(), *Part.SocketName.ToString());
    }
}

void ACharacter2DActor::SetupSkeletalComponent(USkeletalMeshComponent* Component, const FCharacter2DSkeletalPart& Part)
{
    if (!Component || !CharacterAsset) return;
    
    Component->SetSkeletalMesh(Part.Mesh);
    Component->SetAnimInstanceClass(Part.AnimInstance);
    
    for (const auto& Material : Part.Materials) 
    { 
        Component->SetMaterial(Material.SlotIndex, Material.Material); 
    }
    
    // Если компонент не аттачится к сокету, применяем offset и scale
    if (Part.AttachmentTarget == ECharacter2DSkeletalAttachmentTarget::None)
    {
        Component->SetRelativeLocation(Part.Offset + CharacterAsset->SkeletalGlobalOffset);
        Component->SetRelativeScale3D(FVector(Part.Scale * CharacterAsset->GlobalScale));
    }
    else
    {
        // Если аттачится к сокету, offset и scale будут применены после аттачмента
        Component->SetRelativeLocation(Part.Offset);
        Component->SetRelativeScale3D(FVector(Part.Scale));
    }
    
    Component->SetVisibility(Part.Mesh != nullptr && bSkeletalVisible);
}

void ACharacter2DActor::SetupSpriteComponent(UPaperSpriteComponent* Component, TObjectPtr<UPaperSprite> Sprite, const FVector& Offset, float Scale, bool bIsVisible, const FLinearColor& Color, float Opacity)
{
    if (!Component || !CharacterAsset) return;
    const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + Offset;
    const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * Scale;
    Component->SetSprite(Sprite);
    Component->SetRelativeLocation(FinalOffset);
    Component->SetRelativeScale3D(FVector(FinalScale));
    Component->SetVisibility(bIsVisible && bSpritesVisible);
    
    FLinearColor FinalColor = Color;
    FinalColor.A = Opacity;
    Component->SetSpriteColor(FinalColor);
}

void ACharacter2DActor::AttachSpriteToSocket(UPaperSpriteComponent* SpriteComp, ECharacter2DAttachmentTarget TargetType, FName SocketName, bool bUseSocketTransform, const FVector& Offset, float Scale)
{
    if (!SpriteComp || !CharacterAsset || TargetType == ECharacter2DAttachmentTarget::None) return;
    USkeletalMeshComponent* TargetComponent = GetSkeletalComponentByTarget(TargetType);
    if (AttachComponentToSocket(SpriteComp, TargetComponent, SocketName, bUseSocketTransform))
    {
        if(!bUseSocketTransform)
        {
            SetupSpriteComponent(SpriteComp, SpriteComp->GetSprite(), Offset, Scale, SpriteComp->IsVisible());
        }
    }
}

bool ACharacter2DActor::AttachComponentToSocket(USceneComponent* Component, USkeletalMeshComponent* TargetMesh, FName SocketName, bool bUseSocketTransform)
{
    if (!Component || !TargetMesh || SocketName == NAME_None || !TargetMesh->DoesSocketExist(SocketName)) return false;
    Component->AttachToComponent(TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
    if(bUseSocketTransform) 
    { 
        Component->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator); 
    }
    return true;
}

void ACharacter2DActor::SetupComponents()
{
	BodyComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyComponent"));
	ArmsComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmsComponent"));
	HeadComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HeadComponent"));
	BodyComponent->SetupAttachment(RootComponent);
	ArmsComponent->SetupAttachment(RootComponent);
	HeadComponent->SetupAttachment(RootComponent);

	SpriteBody = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteBody"));
	SpriteArms = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteArms"));
	SpriteHead = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteHead"));
	SpriteEyebrow = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyebrow"));
	SpriteEyes = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyes"));
	SpriteEyelids = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEyelids"));
	SpriteMouth = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteMouth"));
	SpriteEffect1 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect1"));
	SpriteEffect2 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect2"));
	SpriteEffect3 = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteEffect3"));
	SpriteShadow = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("SpriteShadow"));

	SpriteBody->SetupAttachment(RootComponent);
	SpriteArms->SetupAttachment(RootComponent);
	SpriteHead->SetupAttachment(RootComponent);
	SpriteEyebrow->SetupAttachment(SpriteHead);
	SpriteEyes->SetupAttachment(SpriteHead);
	SpriteEyelids->SetupAttachment(SpriteHead);
	SpriteMouth->SetupAttachment(SpriteHead);
	SpriteEffect1->SetupAttachment(SpriteHead);
	SpriteEffect2->SetupAttachment(SpriteHead);
	SpriteEffect3->SetupAttachment(SpriteHead);
	// Shadow теперь отдельный слой, аттачим к root
	SpriteShadow->SetupAttachment(RootComponent);

	for (UPaperSpriteComponent* Component : GetAllSpriteComponents(true, true))
	{
		if (Component)
		{
			Component->SetCastShadow(false);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ACharacter2DActor::BeginPlay()
{
    Super::BeginPlay();

    // Привязка функций к таймлайну моргания
    if (BlinkTimeline)
    {
        FOnTimelineFloat BlinkUpdateCallback;
        BlinkUpdateCallback.BindUFunction(this, FName("HandleBlinkTimelineUpdate"));
        BlinkTimeline->AddInterpFloat(BlinkCurve, BlinkUpdateCallback);
        BlinkTimeline->SetLooping(false);
        BlinkTimeline->SetTimelineLength(1.0f);

        FOnTimelineEvent BlinkFinishedCallback;
        BlinkFinishedCallback.BindUFunction(this, FName("HandleBlinkTimelineFinished"));
        BlinkTimeline->SetTimelineFinishedFunc(BlinkFinishedCallback);
    }

    // Привязка функций к таймлайну разговора
    if (TalkTimeline)
    {
        FOnTimelineFloat TalkUpdateCallback;
        TalkUpdateCallback.BindUFunction(this, FName("HandleTalkTimelineUpdate"));
        TalkTimeline->AddInterpFloat(TalkCurve, TalkUpdateCallback);
        TalkTimeline->SetLooping(false);

        FOnTimelineEvent TalkFinishedCallback;
        TalkFinishedCallback.BindUFunction(this, FName("HandleTalkTimelineFinished"));
        TalkTimeline->SetTimelineFinishedFunc(TalkFinishedCallback);
    }
	
    StoreOriginalValues();
	
    if (CharacterAsset)
    {
        UpdateOriginalSprites();
		
        if (GetWorld() && GetWorld()->IsGameWorld())
        {
            EnableBlinking(CharacterAsset->bAutoBlink);
            EnableTalking(CharacterAsset->bAutoTalk);
        }
    }
}

void ACharacter2DActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (!CharacterAsset) return;

    // Настройка skeletal компонентов
    SetupSkeletalComponent(BodyComponent, CharacterAsset->Body);
    SetupSkeletalComponent(ArmsComponent, CharacterAsset->Arms);
    SetupSkeletalComponent(HeadComponent, CharacterAsset->Head);
    
    // Аттач skeletal компонентов к сокетам (если настроено)
    AttachSkeletalComponentToSocket(ArmsComponent, CharacterAsset->Arms);
    AttachSkeletalComponentToSocket(HeadComponent, CharacterAsset->Head);

    const auto& BodySpriteData = CharacterAsset->GetBodySprite();
    const auto& ArmsSpriteData = CharacterAsset->GetArmsSprite();
    
    SetupSpriteComponent(SpriteBody, BodySpriteData.Sprite, BodySpriteData.Offset, BodySpriteData.Scale, BodySpriteData.bVisible, BodySpriteData.Color, BodySpriteData.Opacity);
    SetupSpriteComponent(SpriteArms, ArmsSpriteData.Sprite, ArmsSpriteData.Offset, ArmsSpriteData.Scale, ArmsSpriteData.bVisible, ArmsSpriteData.Color, ArmsSpriteData.Opacity);
    SetupHeadHierarchy();
    
    AttachSpriteToSocket(SpriteBody, BodySpriteData.AttachmentTarget, BodySpriteData.SocketName, BodySpriteData.bUseSocketTransform, BodySpriteData.Offset, BodySpriteData.Scale);
    AttachSpriteToSocket(SpriteArms, ArmsSpriteData.AttachmentTarget, ArmsSpriteData.SocketName, ArmsSpriteData.bUseSocketTransform, ArmsSpriteData.Offset, ArmsSpriteData.Scale);
    AttachHeadToSocket();

    SetupEffectLayers();
    SetupShadowLayer();
    AttachShadowToSocket();
    SetSpritesVisible(HasValidSprites());
    SetSkeletalVisible(HasValidSkeletalMeshes());
	
    UpdateOriginalSprites();
}

void ACharacter2DActor::UpdateOriginalSprites()
{
    if (!CharacterAsset) return;
    
    if (SpriteEyelids && CharacterAsset->GetEyelidsSprite().Sprite)
    {
        OriginalEyelidsSprite = CharacterAsset->GetEyelidsSprite().Sprite;
    }
    
    if (SpriteMouth && CharacterAsset->GetMouthSprite().Sprite)
    {
        OriginalMouthSprite = CharacterAsset->GetMouthSprite().Sprite;
    }
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("Updated original sprites: Eyelids=%s, Mouth=%s"), 
           OriginalEyelidsSprite ? *OriginalEyelidsSprite->GetName() : TEXT("None"),
           OriginalMouthSprite ? *OriginalMouthSprite->GetName() : TEXT("None"));
}

void ACharacter2DActor::SetupHeadHierarchy()
{
    if (!CharacterAsset) return;

    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    
    SetupSpriteComponent(SpriteHead, HeadStructure.Head.Sprite, HeadStructure.Head.Offset, HeadStructure.Head.Scale, HeadStructure.Head.bVisible, HeadStructure.Head.Color, HeadStructure.Head.Opacity);
    
    auto SetupChildSprite = [this, &HeadStructure](
        UPaperSpriteComponent* Component, 
        const FCharacter2DHeadChildSprite& ChildData)
    {
        if (!Component) return;
        Component->SetSprite(ChildData.Sprite);
        Component->SetRelativeLocation(ChildData.LocalOffset);
        Component->SetRelativeScale3D(FVector(ChildData.LocalScale));
        Component->SetVisibility(HeadStructure.GetFinalChildVisibility(ChildData) && bSpritesVisible);
        
        FLinearColor FinalColor = ChildData.Color;
        FinalColor.A = ChildData.Opacity;
        Component->SetSpriteColor(FinalColor);
    };

    SetupChildSprite(SpriteEyebrow, HeadStructure.Eyebrows);
    SetupChildSprite(SpriteEyes, HeadStructure.Eyes);
    SetupChildSprite(SpriteEyelids, HeadStructure.Eyelids);
    SetupChildSprite(SpriteMouth, HeadStructure.Mouth);
}

void ACharacter2DActor::SetupEffectLayers()
{
    if (!CharacterAsset) return;
    const auto& HeadStructure = CharacterAsset->SpriteStructure.Head;
    
    auto SetupLayer = [this](UPaperSpriteComponent* Component, const FCharacter2DEffectLayer& Layer)
    {
        if (!Component) return;
        Component->SetSprite(Layer.Sprite);
        Component->SetRelativeLocation(Layer.LocalOffset);
        Component->SetRelativeScale3D(FVector(Layer.LocalScale));
        Component->SetVisibility(Layer.bVisible && bSpritesVisible);
        FLinearColor Color = Layer.Color;
        Color.A = Layer.Opacity;
        Component->SetSpriteColor(Color);
    };
    
    SetupLayer(SpriteEffect1, HeadStructure.EffectLayer1);
    SetupLayer(SpriteEffect2, HeadStructure.EffectLayer2);
    SetupLayer(SpriteEffect3, HeadStructure.EffectLayer3);
}

void ACharacter2DActor::SetupShadowLayer()
{
    if (!CharacterAsset || !SpriteShadow) return;
    
    const auto& ShadowData = CharacterAsset->GetShadowLayer();
    
    // Применяем GlobalOffset и GlobalScale как для других спрайтов
    const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + ShadowData.Offset;
    const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * ShadowData.Scale;
    
    SpriteShadow->SetSprite(ShadowData.Sprite);
    SpriteShadow->SetRelativeLocation(FinalOffset);
    SpriteShadow->SetRelativeScale3D(FVector(FinalScale));
    SpriteShadow->SetVisibility(ShadowData.bVisible && bSpritesVisible);
    
    FLinearColor ShadowColor = ShadowData.Color;
    ShadowColor.A = ShadowData.Opacity;
    SpriteShadow->SetSpriteColor(ShadowColor);
}

void ACharacter2DActor::AttachShadowToSocket()
{
    if (!CharacterAsset) return;
    const auto& ShadowData = CharacterAsset->GetShadowLayer();
    
    // Передаем правильные offset и scale с учетом Global параметров
    const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + ShadowData.Offset;
    const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * ShadowData.Scale;
    
    AttachSpriteToSocket(SpriteShadow, ShadowData.AttachmentTarget, ShadowData.SocketName, ShadowData.bUseSocketTransform, FinalOffset, FinalScale);
}

void ACharacter2DActor::AttachHeadToSocket()
{
    if (!CharacterAsset) return;
    const auto& HeadRoot = CharacterAsset->SpriteStructure.Head.Head;
    AttachSpriteToSocket(SpriteHead, HeadRoot.AttachmentTarget, HeadRoot.SocketName, HeadRoot.bUseSocketTransform, HeadRoot.Offset, HeadRoot.Scale);
}

// Shadow Layer Functions
void ACharacter2DActor::SetShadowLayer(UPaperSprite* Sprite, bool bVisible, const FLinearColor& Color, float Opacity)
{
    if (!SpriteShadow || !CharacterAsset) return;
    
    SpriteShadow->SetSprite(Sprite);
    SpriteShadow->SetVisibility(bVisible && bSpritesVisible);
    
    FLinearColor FinalColor = Color;
    FinalColor.A = Opacity;
    SpriteShadow->SetSpriteColor(FinalColor);
    
    // При ручной установке спрайта тени также учитываем Global параметры
    if (Sprite)
    {
        const auto& ShadowData = CharacterAsset->GetShadowLayer();
        const FVector FinalOffset = CharacterAsset->GetGlobalSpriteOffset() + ShadowData.Offset;
        const float FinalScale = CharacterAsset->GetGlobalSpriteScale() * ShadowData.Scale;
        
        SpriteShadow->SetRelativeLocation(FinalOffset);
        SpriteShadow->SetRelativeScale3D(FVector(FinalScale));
    }
}

void ACharacter2DActor::SetShadowOpacity(float Opacity)
{
    if (!SpriteShadow) return;
    
    FLinearColor CurrentColor = SpriteShadow->GetSpriteColor();
    CurrentColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
    SpriteShadow->SetSpriteColor(CurrentColor);
}

void ACharacter2DActor::SetShadowColor(const FLinearColor& Color)
{
    if (!SpriteShadow) return;
    
    FLinearColor NewColor = Color;
    NewColor.A = SpriteShadow->GetSpriteColor().A;
    SpriteShadow->SetSpriteColor(NewColor);
}

void ACharacter2DActor::SetShadowVisible(bool bVisible)
{
    if (!SpriteShadow) return;
    SpriteShadow->SetVisibility(bVisible && bSpritesVisible);
}

// Sprite Setters
void ACharacter2DActor::SetEyebrowSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteEyebrow) SpriteEyebrow->SetSprite(NewSprite); 
}

void ACharacter2DActor::SetEyesSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteEyes) SpriteEyes->SetSprite(NewSprite); 
}

void ACharacter2DActor::SetEyelidsSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteEyelids) 
    { 
        SpriteEyelids->SetSprite(NewSprite); 
        if (!bIsBlinking) 
        {
            OriginalEyelidsSprite = NewSprite;
            UE_LOG(LogCharacter2DActor, Log, TEXT("Updated OriginalEyelidsSprite to: %s"), 
                   NewSprite ? *NewSprite->GetName() : TEXT("None"));
        }
    }
}

void ACharacter2DActor::SetMouthSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteMouth) 
    { 
        SpriteMouth->SetSprite(NewSprite); 
        if (!bIsTalking) 
        {
            OriginalMouthSprite = NewSprite;
            UE_LOG(LogCharacter2DActor, Log, TEXT("Updated OriginalMouthSprite to: %s"), 
                   NewSprite ? *NewSprite->GetName() : TEXT("None"));
        }
    }
}

void ACharacter2DActor::SetHeadSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteHead) SpriteHead->SetSprite(NewSprite); 
}

void ACharacter2DActor::SetBodySprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteBody) SpriteBody->SetSprite(NewSprite); 
}

void ACharacter2DActor::SetArmsSprite(UPaperSprite* NewSprite) 
{ 
    if (SpriteArms) SpriteArms->SetSprite(NewSprite); 
}

UPaperSpriteComponent* ACharacter2DActor::GetEffectComponentByIndex(ECharacter2DEffectLayerIndex LayerIndex) const
{
    switch (LayerIndex)
    {
    case ECharacter2DEffectLayerIndex::Effect1: return SpriteEffect1;
    case ECharacter2DEffectLayerIndex::Effect2: return SpriteEffect2;
    case ECharacter2DEffectLayerIndex::Effect3: return SpriteEffect3;
    default: return nullptr;
    }
}

void ACharacter2DActor::ShowEffectLayer(ECharacter2DEffectLayerIndex LayerIndex, bool bShow)
{
    if (UPaperSpriteComponent* EffectComponent = GetEffectComponentByIndex(LayerIndex))
    {
        if (bShow)
        {
            EffectComponent->SetVisibility(true, true);
            OnShowEffect(LayerIndex, EffectComponent->GetSprite());
        }
        else
        {
            OnHideEffect(LayerIndex);
        }
    }
}

void ACharacter2DActor::ClearAllEffects()
{
    if (SpriteEffect1 && SpriteEffect1->IsVisible())
    {
        OnHideEffect(ECharacter2DEffectLayerIndex::Effect1);
    }
    if (SpriteEffect2 && SpriteEffect2->IsVisible())
    {
        OnHideEffect(ECharacter2DEffectLayerIndex::Effect2);
    }
    if (SpriteEffect3 && SpriteEffect3->IsVisible())
    {
        OnHideEffect(ECharacter2DEffectLayerIndex::Effect3);
    }
}

void ACharacter2DActor::ForceHideEffect(ECharacter2DEffectLayerIndex LayerIndex)
{
    if (UPaperSpriteComponent* EffectComponent = GetEffectComponentByIndex(LayerIndex))
    {
        EffectComponent->SetVisibility(false);
    }
}

void ACharacter2DActor::ForceHideAllEffects()
{
    ForceHideEffect(ECharacter2DEffectLayerIndex::Effect1);
    ForceHideEffect(ECharacter2DEffectLayerIndex::Effect2);
    ForceHideEffect(ECharacter2DEffectLayerIndex::Effect3);
}

void ACharacter2DActor::EnableBlinking(bool bEnable)
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("EnableBlinking called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    bBlinkingActive = bEnable;
    if (bSpritesVisible) 
    { 
        if (bEnable && !bIsBlinking) 
        {
            UpdateOriginalSprites();
            StartBlinking();
        }
        else if (!bEnable && bIsBlinking) 
        {
            StopBlinking();
        }
    }
}

void ACharacter2DActor::EnableTalking(bool bEnable)
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("EnableTalking called: %s"), bEnable ? TEXT("true") : TEXT("false"));
    
    bTalkingActive = bEnable;
    if (bSpritesVisible) 
    { 
        if (bEnable && !bIsTalking) 
        {
            UpdateOriginalSprites();
            StartTalking();
        }
        else if (!bEnable && bIsTalking) 
        {
            StopTalking();
        }
    }
}

void ACharacter2DActor::BlinkOnce()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset || BlinkTimeline->IsPlaying()) return;

    bBlinkScheduleNext = false;
    UpdateOriginalSprites();
    TriggerBlink();
}

void ACharacter2DActor::TriggerBlink()
{
    if (!bBlinkingActive && !bBlinkScheduleNext) return;
    if (!CharacterAsset || !SpriteEyelids || !GetWorld()) return;

    const auto& Settings = CharacterAsset->GetBlinkSettings();
    if (!Settings.ClosedEyelidsFlipbook || Settings.ClosedEyelidsFlipbook->GetNumFrames() == 0) return;
    
    UpdateOriginalSprites();

    if (BlinksLeftInSequence <= 0)
    {
        if (FMath::FRand() < Settings.DoubleBlinkChance)
        {
            BlinksLeftInSequence = 2;
        }
        else
        {
            BlinksLeftInSequence = 1;
        }
    }
    
    OnBlinkStarted.Broadcast();
    
    CurrentBlinkFlipbook = Settings.ClosedEyelidsFlipbook;
    const int32 NumFrames = CurrentBlinkFlipbook->GetNumFrames();
    const float ActualDuration = Settings.BlinkDuration + FMath::FRandRange(-Settings.BlinkDurationVariation, Settings.BlinkDurationVariation);
    const float FinalDuration = FMath::Max(ActualDuration, 0.01f);
    
    BlinkCurve->FloatCurve.Reset();
    BlinkCurve->FloatCurve.AddKey(0.0f, 0.0f);
    BlinkCurve->FloatCurve.AddKey(0.4f, static_cast<float>(NumFrames - 1));
    BlinkCurve->FloatCurve.AddKey(0.6f, static_cast<float>(NumFrames - 1));
    BlinkCurve->FloatCurve.AddKey(1.0f, 0.0f);

    BlinkTimeline->SetPlayRate(1.0f / FinalDuration);
    BlinkTimeline->PlayFromStart();
}

void ACharacter2DActor::StartBlinking()
{
    if (!IsValid(this) || !IsValid(SpriteEyelids) || !CharacterAsset) return;
    
    UE_LOG(LogCharacter2DActor, Log, TEXT("StartBlinking called"));
    
    bIsBlinking = true;
    bBlinkScheduleNext = true;

    const auto& Settings = CharacterAsset->GetBlinkSettings();
    const float Delay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
    GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::TriggerBlink, Delay, false);
}

void ACharacter2DActor::StopBlinking()
{
    UE_LOG(LogCharacter2DActor, Log, TEXT("StopBlinking called"));
    
    bIsBlinking = false;
    bBlinkScheduleNext = false;
    
    GetWorldTimerManager().ClearTimer(BlinkTimerHandle);
    GetWorldTimerManager().ClearTimer(InterBlinkTimerHandle);
    
    if (BlinkTimeline && BlinkTimeline->IsPlaying())
    {
        BlinkTimeline->Stop();
    }

    BlinksLeftInSequence = 0;
    RestoreEyelidsAfterBlink();
}

void ACharacter2DActor::HandleBlinkTimelineUpdate(float Value)
{
    if (!CurrentBlinkFlipbook) return;

    int32 FrameIndex = FMath::Clamp(FMath::FloorToInt(Value), 0, CurrentBlinkFlipbook->GetNumFrames() - 1);
    
    if (UPaperSprite* FrameSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(FrameIndex))
    {
        SpriteEyelids->SetSprite(FrameSprite);
    }
}

void ACharacter2DActor::HandleBlinkTimelineFinished()
{
    BlinksLeftInSequence--;
    
    if (bBlinkScheduleNext && BlinksLeftInSequence > 0 && CharacterAsset)
    {
        const auto& Settings = CharacterAsset->GetBlinkSettings();
        RestoreEyelidsPartiallyAfterBlink(Settings.SecondBlinkOpenAmount);
        GetWorldTimerManager().SetTimer(InterBlinkTimerHandle, this, &ACharacter2DActor::TriggerBlink, Settings.InterBlinkDelay, false);
    }
    else
    {
        BlinksLeftInSequence = 0;
        RestoreEyelidsAfterBlink();
        OnBlinkFinished.Broadcast();
        CurrentBlinkFlipbook = nullptr;

        if (bBlinkScheduleNext && bIsBlinking && CharacterAsset)
        {
            const auto& Settings = CharacterAsset->GetBlinkSettings();
            const float NextDelay = FMath::FRandRange(Settings.BlinkIntervalMin, Settings.BlinkIntervalMax);
            GetWorldTimerManager().SetTimer(BlinkTimerHandle, this, &ACharacter2DActor::TriggerBlink, NextDelay, false);
        }
    }
}

void ACharacter2DActor::SetAllSpritesVisible(bool bVisible)
{
    bSpritesVisible = bVisible;
    for (UPaperSpriteComponent* Component : GetAllSpriteComponents(true, false)) 
    { 
        if(Component) Component->SetVisibility(bVisible && Component->GetSprite() != nullptr); 
    }
    if(CharacterAsset) SetupHeadHierarchy();
}

void ACharacter2DActor::SetAllSkeletalVisible(bool bVisible)
{
    bSkeletalVisible = bVisible;
    for (USkeletalMeshComponent* Component : GetAllSkeletalComponents()) 
    { 
        if(Component) Component->SetVisibility(bVisible); 
    }
}

void ACharacter2DActor::RestoreEyelidsPartiallyAfterBlink(float OpenAmount)
{
    if (!SpriteEyelids || !CurrentBlinkFlipbook || !CharacterAsset)
    {
        RestoreEyelidsAfterBlink();
        return;
    }

    const int32 NumFrames = CurrentBlinkFlipbook->GetNumFrames();
    if (NumFrames > 1)
    {
        const int32 FrameIndex = FMath::Clamp(FMath::RoundToInt(static_cast<float>(NumFrames - 1) * (1.0f - OpenAmount)), 0, NumFrames - 1);
        if (UPaperSprite* PartialSprite = CurrentBlinkFlipbook->GetSpriteAtFrame(FrameIndex))
        {
             SpriteEyelids->SetSprite(PartialSprite);
        }
    }
    else
    {
        RestoreEyelidsAfterBlink();
    }
}

void ACharacter2DActor::Appear()
{
    OnAppear();
}

void ACharacter2DActor::Disappear()
{
    OnDisappear();
}

void ACharacter2DActor::GainFocus()
{
    if (!bIsFocused)
    {
        bIsFocused = true;
        OnFocusGained();
    }
}

void ACharacter2DActor::LoseFocus()
{
    if (bIsFocused)
    {
        bIsFocused = false;
        OnFocusLost();
    }
}

void ACharacter2DActor::SetEffectLayer(ECharacter2DEffectLayerIndex LayerIndex, UPaperSprite* Sprite, bool bVisible, const FLinearColor& Color, float Opacity)
{
    if (UPaperSpriteComponent* EffectComponent = GetEffectComponentByIndex(LayerIndex))
    {
        EffectComponent->SetSprite(Sprite);
        FLinearColor FinalColor = Color;
        FinalColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
        EffectComponent->SetSpriteColor(FinalColor);
        
        ShowEffectLayer(LayerIndex, bVisible);
    }
}

void ACharacter2DActor::SetEffectLayerOpacity(ECharacter2DEffectLayerIndex LayerIndex, float Opacity)
{
    if (UPaperSpriteComponent* EffectComponent = GetEffectComponentByIndex(LayerIndex))
    {
        FLinearColor CurrentColor = EffectComponent->GetSpriteColor();
        CurrentColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
        EffectComponent->SetSpriteColor(CurrentColor);
    }
}

void ACharacter2DActor::SetEffectLayerColor(ECharacter2DEffectLayerIndex LayerIndex, const FLinearColor& Color)
{
    if (UPaperSpriteComponent* EffectComponent = GetEffectComponentByIndex(LayerIndex))
    {
        FLinearColor NewColor = Color;
        NewColor.A = EffectComponent->GetSpriteColor().A;
        EffectComponent->SetSpriteColor(NewColor);
    }
}

void ACharacter2DActor::SetAllCharacterSpritesColor(const FLinearColor& NewColor, bool bIncludeEffects)
{
    for (UPaperSpriteComponent* SpriteComp : GetAllSpriteComponents(bIncludeEffects, false))
    {
        if (SpriteComp)
        {
            FLinearColor CurrentColor = SpriteComp->GetSpriteColor();
            CurrentColor.R = NewColor.R;
            CurrentColor.G = NewColor.G;
            CurrentColor.B = NewColor.B;
            SpriteComp->SetSpriteColor(CurrentColor);
        }
    }
}