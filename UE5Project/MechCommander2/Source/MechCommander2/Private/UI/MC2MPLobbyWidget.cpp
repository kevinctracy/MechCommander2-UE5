#include "UI/MC2MPLobbyWidget.h"
#include "Online/MC2SessionSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ListViewBase.h"
#include "Components/CheckBox.h"
#include "Components/SpinBox.h"

void UMC2MPLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostButton)   HostButton->OnClicked.AddDynamic(this, &UMC2MPLobbyWidget::HandleHost);
	if (FindButton)   FindButton->OnClicked.AddDynamic(this, &UMC2MPLobbyWidget::HandleFind);
	if (JoinButton)   JoinButton->OnClicked.AddDynamic(this, &UMC2MPLobbyWidget::HandleJoin);
	if (LaunchButton) LaunchButton->OnClicked.AddDynamic(this, &UMC2MPLobbyWidget::HandleLaunch);
	if (BackButton)   BackButton->OnClicked.AddDynamic(this, &UMC2MPLobbyWidget::HandleBack);

	// Bind session subsystem delegates
	if (UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>())
	{
		SS->OnSessionCreated.AddDynamic(this, &UMC2MPLobbyWidget::OnSessionCreatedInternal);
		SS->OnSessionsFound .AddDynamic(this, &UMC2MPLobbyWidget::OnSessionsFoundInternal);
		SS->OnSessionJoined .AddDynamic(this, &UMC2MPLobbyWidget::OnJoinedInternal);
	}
}

void UMC2MPLobbyWidget::HostSession()
{
	UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>();
	if (!SS) return;

	int32 MaxPlayers = MaxPlayersSpinner ? FMath::RoundToInt(MaxPlayersSpinner->GetValue()) : 4;
	bool  bLAN       = LANCheck ? LANCheck->IsChecked() : false;
	SS->HostSession(MaxPlayers, bLAN);

	if (StatusText) StatusText->SetText(FText::FromString(TEXT("Creating session...")));
}

void UMC2MPLobbyWidget::FindSessions()
{
	UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>();
	if (!SS) return;

	bool bLAN = LANCheck ? LANCheck->IsChecked() : false;
	SS->FindSessions(bLAN);

	if (StatusText) StatusText->SetText(FText::FromString(TEXT("Searching...")));
}

void UMC2MPLobbyWidget::JoinSelectedSession()
{
	if (SelectedSessionIndex < 0) return;
	UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>();
	if (SS) SS->JoinFoundSession(SelectedSessionIndex);
}

void UMC2MPLobbyWidget::HandleHost()   { HostSession(); }
void UMC2MPLobbyWidget::HandleFind()   { FindSessions(); }
void UMC2MPLobbyWidget::HandleJoin()   { JoinSelectedSession(); }
void UMC2MPLobbyWidget::HandleLaunch()
{
	// Host travels — map configured in BP
	if (APlayerController* PC = GetOwningPlayer())
		PC->GetWorld()->ServerTravel(TEXT("L_MC2_01?listen"), true);
}
void UMC2MPLobbyWidget::HandleBack()
{
	if (UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>())
		SS->DestroySession();
	RemoveFromParent();
}

void UMC2MPLobbyWidget::OnSessionCreatedInternal(bool bSuccess, FName SessionName)
{
	OnSessionCreated(bSuccess);
	if (StatusText)
		StatusText->SetText(bSuccess
			? FText::FromString(TEXT("Session ready — waiting for players"))
			: FText::FromString(TEXT("Failed to create session")));
}

void UMC2MPLobbyWidget::OnSessionsFoundInternal(bool bSuccess)
{
	int32 NumFound = 0;
	if (UMC2SessionSubsystem* SS = GetGameInstance()->GetSubsystem<UMC2SessionSubsystem>())
		NumFound = SS->GetFoundSessionCount();

	OnSessionsFound(bSuccess, NumFound);
	if (StatusText)
		StatusText->SetText(FText::Format(
			FText::FromString(TEXT("Found {0} session(s)")), FText::AsNumber(NumFound)));
}

void UMC2MPLobbyWidget::OnJoinedInternal(bool bSuccess, FString TravelURL)
{
	OnJoinedSession(bSuccess);
	if (StatusText)
		StatusText->SetText(bSuccess
			? FText::FromString(TEXT("Joined — waiting for host"))
			: FText::FromString(TEXT("Failed to join session")));
}
