#include "Input/MC2InputConfig.h"
#include "InputAction.h"

FText UMC2InputConfig::GetActionDisplayName(const UInputAction* Action)
{
	if (!Action)
	{
		return FText::GetEmpty();
	}

	// Strip the "IA_" prefix and insert spaces before capitals for readability.
	FString Raw = Action->GetName();
	if (Raw.StartsWith(TEXT("IA_")))
	{
		Raw.RemoveFromStart(TEXT("IA_"));
	}

	FString Result;
	Result.Reserve(Raw.Len() + 8);
	for (int32 i = 0; i < Raw.Len(); ++i)
	{
		if (i > 0 && FChar::IsUpper(Raw[i]) && !FChar::IsUpper(Raw[i - 1]))
		{
			Result.AppendChar(TEXT(' '));
		}
		Result.AppendChar(Raw[i]);
	}

	return FText::FromString(Result);
}
