// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "PlainTextLayoutMarshaller.h"

#if WITH_FANCY_TEXT

TSharedRef< FPlainTextLayoutMarshaller > FPlainTextLayoutMarshaller::Create(FTextBlockStyle InDefaultTextStyle)
{
	return MakeShareable(new FPlainTextLayoutMarshaller(MoveTemp(InDefaultTextStyle)));
}

FPlainTextLayoutMarshaller::~FPlainTextLayoutMarshaller()
{
}

void FPlainTextLayoutMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	TArray<FTextRange> LineRanges;
	FTextRange::CalculateLineRangesFromString(SourceString, LineRanges);

	for(const FTextRange& LineRange : LineRanges)
	{
		TSharedRef<FString> LineText = MakeShareable(new FString(SourceString.Mid(LineRange.BeginIndex, LineRange.Len())));

		TArray<TSharedRef<IRun>> Runs;
		Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, DefaultTextStyle));

		TargetTextLayout.AddLine(LineText, Runs);
	}
}

void FPlainTextLayoutMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
{
	SourceTextLayout.GetAsText(TargetString);
}

void FPlainTextLayoutMarshaller::SetDefaultTextStyle(FTextBlockStyle InDefaultTextStyle)
{
	DefaultTextStyle = MoveTemp(InDefaultTextStyle);
	MakeDirty();
}

const FTextBlockStyle& FPlainTextLayoutMarshaller::GetDefaultTextStyle() const
{
	return DefaultTextStyle;
}

FPlainTextLayoutMarshaller::FPlainTextLayoutMarshaller(FTextBlockStyle InDefaultTextStyle)
	: DefaultTextStyle(MoveTemp(InDefaultTextStyle))
{
}

#endif //WITH_FANCY_TEXT
