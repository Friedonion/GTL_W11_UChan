#include "AnimTypes.h"
#include "Animation/AnimNotifyState.h"
#include "UObject/UObjectArray.h"

void FAnimNotifyEvent::SetAnimNotify(class UAnimNotify* InNotify)
{
    if (Notify)
    {
        GUObjectArray.MarkRemoveObject(Notify);
    }
    Notify = InNotify;
}

void FAnimNotifyEvent::SetAnimNotifyState(class UAnimNotifyState* InNotifyState)
{
    if (NotifyState)
    {
        GUObjectArray.MarkRemoveObject(NotifyState);
    }
    NotifyState = InNotifyState;
}
