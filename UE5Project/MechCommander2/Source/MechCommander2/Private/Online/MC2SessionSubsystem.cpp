#include "Online/MC2SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"

const FName UMC2SessionSubsystem::SESSION_NAME    = FName("MC2Session");
const FName UMC2SessionSubsystem::SETTING_MAPNAME = FName("MapName");

void UMC2SessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
	if (!OSS) return;

	SessionInterface = OSS->GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	CreateSessionHandle  = SessionInterface->OnCreateSessionCompleteDelegates .AddUObject(this, &UMC2SessionSubsystem::HandleCreateSessionComplete);
	DestroySessionHandle = SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMC2SessionSubsystem::HandleDestroySessionComplete);
	FindSessionsHandle   = SessionInterface->OnFindSessionsCompleteDelegates  .AddUObject(this, &UMC2SessionSubsystem::HandleFindSessionsComplete);
	JoinSessionHandle    = SessionInterface->OnJoinSessionCompleteDelegates   .AddUObject(this, &UMC2SessionSubsystem::HandleJoinSessionComplete);
}

void UMC2SessionSubsystem::Deinitialize()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle (CreateSessionHandle);
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionHandle);
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle  (FindSessionsHandle);
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle   (JoinSessionHandle);
	}
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Host
// ---------------------------------------------------------------------------

void UMC2SessionSubsystem::HostSession(int32 MaxPlayers, bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;

	// Destroy any existing session first
	if (SessionInterface->GetNamedSession(SESSION_NAME))
	{
		SessionInterface->DestroySession(SESSION_NAME);
		// Session creation will happen in HandleDestroySessionComplete, but for
		// simplicity we create immediately after — OSS handles ordering.
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch          = bIsLAN;
	Settings.bUsesPresence        = !bIsLAN;
	Settings.NumPublicConnections = FMath::Clamp(MaxPlayers, 2, 8);
	Settings.bAllowJoinInProgress = false;
	Settings.bShouldAdvertise     = true;
	Settings.bUseLobbiesIfAvailable = true;

	if (UWorld* World = GetGameInstance()->GetWorld())
		Settings.Set(SETTING_MAPNAME, World->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer) return;

	SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), SESSION_NAME, Settings);
}

void UMC2SessionSubsystem::HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	OnSessionCreated.Broadcast(bWasSuccessful, InSessionName);

	if (bWasSuccessful)
	{
		// Travel to the listen server map; map name comes from the GameInstance or config
		if (UWorld* World = GetGameInstance()->GetWorld())
			World->ServerTravel(TEXT("?listen"), true);
	}
}

// ---------------------------------------------------------------------------
// Destroy
// ---------------------------------------------------------------------------

void UMC2SessionSubsystem::DestroySession()
{
	if (SessionInterface.IsValid())
		SessionInterface->DestroySession(SESSION_NAME);
}

void UMC2SessionSubsystem::HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
	OnSessionDestroyed.Broadcast(bWasSuccessful);
}

// ---------------------------------------------------------------------------
// Find
// ---------------------------------------------------------------------------

void UMC2SessionSubsystem::FindSessions(bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery         = bIsLAN;
	SessionSearch->MaxSearchResults    = 20;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer) return;

	SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void UMC2SessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	OnSessionsFound.Broadcast(bWasSuccessful && SessionSearch.IsValid());
}

int32 UMC2SessionSubsystem::GetFoundSessionCount() const
{
	return SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0;
}

FString UMC2SessionSubsystem::GetFoundSessionDisplayName(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
		return FString();
	FString MapName;
	SessionSearch->SearchResults[Index].Session.SessionSettings.Get(SETTING_MAPNAME, MapName);
	return MapName.IsEmpty() ? FString("Unknown Map") : MapName;
}

int32 UMC2SessionSubsystem::GetFoundSessionPingMs(int32 Index) const
{
	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(Index))
		return -1;
	return SessionSearch->SearchResults[Index].PingInMs;
}

// ---------------------------------------------------------------------------
// Join
// ---------------------------------------------------------------------------

void UMC2SessionSubsystem::JoinFoundSession(int32 Index)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid()) return;
	if (!SessionSearch->SearchResults.IsValidIndex(Index)) return;

	ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!LocalPlayer) return;

	SessionInterface->JoinSession(
		*LocalPlayer->GetPreferredUniqueNetId(),
		SESSION_NAME,
		SessionSearch->SearchResults[Index]
	);
}

void UMC2SessionSubsystem::HandleJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	FString TravelURL;

	if (bSuccess && SessionInterface.IsValid())
		SessionInterface->GetResolvedConnectString(InSessionName, TravelURL);

	OnSessionJoined.Broadcast(bSuccess, TravelURL);

	if (bSuccess && !TravelURL.IsEmpty())
	{
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
			PC->ClientTravel(TravelURL, TRAVEL_Absolute);
	}
}
