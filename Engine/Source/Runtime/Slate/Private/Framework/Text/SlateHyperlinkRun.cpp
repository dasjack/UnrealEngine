// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"

#if WITH_FANCY_TEXT

#include "SlateHyperlinkRun.h"

TSharedRef< FSlateHyperlinkRun > FSlateHyperlinkRun::Create( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, FOnClick NavigateDelegate )
{
	return MakeShareable( new FSlateHyperlinkRun( InRunInfo, InText, InStyle, NavigateDelegate ) );
}

TSharedRef< FSlateHyperlinkRun > FSlateHyperlinkRun::Create( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, FOnClick NavigateDelegate, const FTextRange& InRange )
{
	return MakeShareable( new FSlateHyperlinkRun( InRunInfo, InText, InStyle, NavigateDelegate, InRange ) );
}

FTextRange FSlateHyperlinkRun::GetTextRange() const 
{
	return Range;
}

void FSlateHyperlinkRun::SetTextRange( const FTextRange& Value )
{
	Range = Value;
}

int16 FSlateHyperlinkRun::GetBaseLine( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetBaseline( Style.TextStyle.Font, Scale );
}

int16 FSlateHyperlinkRun::GetMaxHeight( float Scale ) const 
{
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->GetMaxCharacterHeight( Style.TextStyle.Font, Scale );
}

FVector2D FSlateHyperlinkRun::Measure( int32 StartIndex, int32 EndIndex, float Scale ) const 
{
	if ( EndIndex - StartIndex == 0 )
	{
		return FVector2D( 0, GetMaxHeight( Scale ) );
	}

	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FontMeasure->Measure( **Text, StartIndex, EndIndex, Style.TextStyle.Font, true, Scale );
}

int8 FSlateHyperlinkRun::GetKerning( int32 CurrentIndex, float Scale ) const 
{
	return 0;
}

TSharedRef< ILayoutBlock > FSlateHyperlinkRun::CreateBlock( int32 StartIndex, int32 EndIndex, FVector2D Size, const TSharedPtr< IRunRenderer >& Renderer )
{
	TSharedRef< SWidget > Widget = SNew( SRichTextHyperlink, ViewModel )
		.Style( &Style )
		.Text( FText::FromString( FString( EndIndex - StartIndex, **Text + StartIndex ) ) )
		.OnNavigate( this, &FSlateHyperlinkRun::OnNavigate );

	Children.Add( Widget );

	return FWidgetLayoutBlock::Create( SharedThis( this ), Widget, FTextRange( StartIndex, EndIndex ), Size, Renderer );
}

void FSlateHyperlinkRun::OnNavigate()
{
	NavigateDelegate.Execute( RunInfo.MetaData );
}

int32 FSlateHyperlinkRun::OnPaint( const FPaintArgs& Args, const FTextLayout::FLineView& Line, const TSharedRef< ILayoutBlock >& Block, const FTextBlockStyle& DefaultStyle, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const 
{
	const TSharedRef< FWidgetLayoutBlock > WidgetBlock = StaticCastSharedRef< FWidgetLayoutBlock >( Block );

	FGeometry WidgetGeometry = AllottedGeometry.MakeChild( Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f );
	return WidgetBlock->GetWidget()->Paint( Args, WidgetGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );
}

const TArray< TSharedRef<SWidget> >& FSlateHyperlinkRun::GetChildren()
{
	return Children;
}

void FSlateHyperlinkRun::ArrangeChildren( const TSharedRef< ILayoutBlock >& Block, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const 
{
	const TSharedRef< FWidgetLayoutBlock > WidgetBlock = StaticCastSharedRef< FWidgetLayoutBlock >( Block );
	ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( WidgetBlock->GetWidget(), Block->GetLocationOffset() * ( 1 / AllottedGeometry.Scale ), Block->GetSize() * ( 1 / AllottedGeometry.Scale ), 1.0f ) );
}

int32 FSlateHyperlinkRun::GetTextIndexAt( const TSharedRef< ILayoutBlock >& Block, const FVector2D& Location, float Scale, ETextHitPoint* const OutHitPoint ) const
{
	const FVector2D& BlockOffset = Block->GetLocationOffset();
	const FVector2D& BlockSize = Block->GetSize();

	const float Left = BlockOffset.X;
	const float Top = BlockOffset.Y;
	const float Right = BlockOffset.X + BlockSize.X;
	const float Bottom = BlockOffset.Y + BlockSize.Y;

	const bool ContainsPoint = Location.X >= Left && Location.X < Right && Location.Y >= Top && Location.Y < Bottom;

	if ( !ContainsPoint )
	{
		return INDEX_NONE;
	}

	const FTextRange BlockRange = Block->GetTextRange();
	const TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	const int32 Index = FontMeasure->FindCharacterIndexAtOffset( Text.Get(), BlockRange.BeginIndex, BlockRange.EndIndex, Style.TextStyle.Font, Location.X - BlockOffset.X, true, Scale );
	if (OutHitPoint)
	{
		*OutHitPoint = (Index == BlockRange.EndIndex) ? ETextHitPoint::RightGutter : ETextHitPoint::WithinText;
	}

	return Index;
}

FVector2D FSlateHyperlinkRun::GetLocationAt( const TSharedRef< ILayoutBlock >& Block, int32 Offset, float Scale ) const
{
	return Block->GetLocationOffset();
}

void FSlateHyperlinkRun::Move(const TSharedRef<FString>& NewText, const FTextRange& NewRange)
{
	Text = NewText;
	Range = NewRange;
}

TSharedRef<IRun> FSlateHyperlinkRun::Clone() const
{
	return FSlateHyperlinkRun::Create(RunInfo, Text, Style, NavigateDelegate, Range);
}

void FSlateHyperlinkRun::AppendTextTo(FString& AppendToText) const
{
	AppendToText.Append(**Text + Range.BeginIndex, Range.Len());
}

void FSlateHyperlinkRun::AppendTextTo(FString& AppendToText, const FTextRange& PartialRange) const
{
	check(Range.BeginIndex <= PartialRange.BeginIndex);
	check(Range.EndIndex >= PartialRange.EndIndex);

	AppendToText.Append(**Text + PartialRange.BeginIndex, PartialRange.Len());
}

const FRunInfo& FSlateHyperlinkRun::GetRunInfo() const
{
	return RunInfo;
}

FSlateHyperlinkRun::FSlateHyperlinkRun( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, FOnClick InNavigateDelegate ) 
	: RunInfo( InRunInfo )
	, Text( InText )
	, Range( 0, Text->Len() )
	, Style( InStyle )
	, NavigateDelegate( InNavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}

FSlateHyperlinkRun::FSlateHyperlinkRun( const FRunInfo& InRunInfo, const TSharedRef< const FString >& InText, const FHyperlinkStyle& InStyle, FOnClick InNavigateDelegate, const FTextRange& InRange ) 
	: RunInfo( InRunInfo )
	, Text( InText )
	, Range( InRange )
	, Style( InStyle )
	, NavigateDelegate( InNavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}

FSlateHyperlinkRun::FSlateHyperlinkRun( const FSlateHyperlinkRun& Run ) 
	: RunInfo( Run.RunInfo )
	, Text( Run.Text )
	, Range( Run.Range )
	, Style( Run.Style )
	, NavigateDelegate( Run.NavigateDelegate )
	, ViewModel( MakeShareable( new FSlateHyperlinkRun::FWidgetViewModel() ) )
	, Children()
{

}



#endif //WITH_FANCY_TEXT