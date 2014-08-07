// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_FANCY_TEXT

#include "RichTextLayoutMarshaller.h"

/**
 * A rich static text widget. 
 * Through the use of markup and text decorators, text with different styles, embedded image and widgets can be achieved.
 */
class SLATE_API SRichTextBlock : public SWidget
{
public:

	SLATE_BEGIN_ARGS( SRichTextBlock )
		: _Text(  )
		, _WrapTextAt( 0.0f )
		, _AutoWrapText(false)
		, _DecoratorStyleSet( &FCoreStyle::Get() )
		, _TextStyle( &FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>( "NormalText" ) )
		, _Margin( FMargin() )
		, _LineHeightPercentage( 1.0f )
		, _Justification( ETextJustify::Left )
		, _Decorators()
		, _Parser()
	{}
		/** The text displayed in this text block */
		SLATE_ATTRIBUTE( FText, Text )

		/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
		SLATE_ATTRIBUTE( float, WrapTextAt )

		/** Whether to wrap text automatically based on the widget's computed horizontal space.  IMPORTANT: Using automatic wrapping can result
		    in visual artifacts, as the the wrapped size will computed be at least one frame late!  Consider using WrapTextAt instead.  The initial 
			desired size will not be clamped.  This works best in cases where the text block's size is not affecting other widget's layout. */
		SLATE_ATTRIBUTE( bool, AutoWrapText )

		/** The style set used for looking up styles used by decorators*/
		SLATE_ARGUMENT( const ISlateStyle*, DecoratorStyleSet )

		/** The style of the text block, which dictates the default font, color, and shadow options. */
		SLATE_STYLE_ARGUMENT( FTextBlockStyle, TextStyle )

		/** The amount of blank space left around the edges of text area. */
		SLATE_ATTRIBUTE( FMargin, Margin )

		/** The amount to scale each lines height by. */
		SLATE_ATTRIBUTE( float, LineHeightPercentage )

		/** How the text should be aligned with the margin. */
		SLATE_ATTRIBUTE( ETextJustify::Type, Justification )

		/** Any decorators that should be used while parsing the text. */
		SLATE_ARGUMENT( TArray< TSharedRef< class ITextDecorator > >, Decorators )

		/** The parser used to resolve any markup used in the provided string. */
		SLATE_ARGUMENT( TSharedPtr< class IRichTextMarkupParser >, Parser )

		/** Additional decorators can be append to the widget inline. Inline decorators get precedence over decorators not specified inline. */
		TArray< TSharedRef< ITextDecorator > > InlineDecorators;
		SRichTextBlock::FArguments& operator + (const TSharedRef< ITextDecorator >& DecoratorToAdd)
		{
			InlineDecorators.Add( DecoratorToAdd );
			return *this;
		}

	SLATE_END_ARGS()

	static TSharedRef< ITextDecorator > Decorator( const TSharedRef< ITextDecorator >& Decorator )
	{
		return Decorator;
	}

	static TSharedRef< ITextDecorator > WidgetDecorator( const FString RunName, const FWidgetDecorator::FCreateWidget& FactoryDelegate )
	{
		return FWidgetDecorator::Create( RunName, FactoryDelegate );
	}

	template< class UserClass >
	static TSharedRef< ITextDecorator >  WidgetDecorator( const FString RunName, UserClass* InUserObjectPtr, typename FWidgetDecorator::FCreateWidget::TSPMethodDelegate_Const< UserClass >::FMethodPtr InFunc )
	{
		return FWidgetDecorator::Create( RunName, FWidgetDecorator::FCreateWidget::CreateSP( InUserObjectPtr, InFunc ) );
	}

	static TSharedRef< ITextDecorator > ImageDecorator( FString RunName = TEXT("img"), const ISlateStyle* const OverrideStyle = NULL )
	{
		return FImageDecorator::Create( RunName, OverrideStyle );
	}

	static TSharedRef< ITextDecorator > HyperlinkDecorator( const FString Id, const FSlateHyperlinkRun::FOnClick& NavigateDelegate )
	{
		return FHyperlinkDecorator::Create( Id, NavigateDelegate );
	}

	template< class UserClass >
	static TSharedRef< ITextDecorator > HyperlinkDecorator( const FString Id, UserClass* InUserObjectPtr, typename FSlateHyperlinkRun::FOnClick::TSPMethodDelegate< UserClass >::FMethodPtr NavigateFunc )
	{
		return FHyperlinkDecorator::Create( Id, FSlateHyperlinkRun::FOnClick::CreateSP( InUserObjectPtr, NavigateFunc ) );
	}

	// Begin SWidget Interface
	void Construct( const FArguments& InArgs );

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	virtual void CacheDesiredSize() override;
	virtual FVector2D ComputeDesiredSize() const override;

	virtual FChildren* GetChildren() override;

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	// End SWidget Interface

	/**
	 * Gets the text assigned to this text block
	 */
	const FText& GetText() const
	{
		return BoundText.Get(FText::GetEmpty());
	}

	/**
	 * Sets the text for this text block
	 */
	void SetText( const TAttribute<FText>& InTextAttr );

	void SetHighlightText( const FText& InHighlightText );


private:

	/** The text displayed in this text block */
	TAttribute< FText > BoundText;

	/** The state of BoundText last Tick() (only used when BoundText is bound to a delegate providing the source text) */
	FText BoundTextLastTick;

	/** The marshaller used to set the Text text in the text layout. */
	TSharedPtr< FRichTextLayoutMarshaller > Marshaller;

	TSharedPtr< FSlateTextLayout > TextLayout;

	/** Default style used by the TextLayout */
	FTextBlockStyle TextStyle;

	FText HighlightText;
	TSharedPtr< ISlateRunRenderer> TextHighlighter;

	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	TAttribute<float> WrapTextAt;
	
	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	TAttribute<bool> AutoWrapText;

	/** The last known width of the control from the previous OnPaint, used to recalculate wrapping. */
	mutable float CachedAutoWrapTextWidth;

	TAttribute< FMargin > Margin;
	TAttribute< ETextJustify::Type > Justification; 
	TAttribute< float > LineHeightPercentage;
};

#endif //WITH_FANCY_TEXT