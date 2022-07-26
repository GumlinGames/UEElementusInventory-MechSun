// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEElementusInventory

#include "SElementusTable.h"

static const FName& ColumnId_PrimaryIdLabel = TEXT("PrimaryAssetId");
static const FName& ColumnId_ItemIdLabel = TEXT("Id");
static const FName& ColumnId_NameLabel = TEXT("Name");
static const FName& ColumnId_TypeLabel = TEXT("Type");
static const FName& ColumnId_ClassLabel = TEXT("Class");
static const FName& ColumnId_ValueLabel = TEXT("Value");
static const FName& ColumnId_WeightLabel = TEXT("Weight");

class SElementusItemTableRow final : public SMultiColumnTableRow<FElementusItemPtr>
{
public:
	SLATE_BEGIN_ARGS(SElementusItemTableRow) :
			_HightlightTextSource()
		{
		}

		SLATE_ARGUMENT(const FText*, HightlightTextSource)
	SLATE_END_ARGS()

	const FText* HighlightText;

	void Construct(const FArguments& InArgs,
	               const TSharedRef<STableViewBase>& InOwnerTableView,
	               const FElementusItemPtr InEntryItem)
	{
		HighlightText = InArgs._HightlightTextSource;

		Item = InEntryItem;
		SMultiColumnTableRow<FElementusItemPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		const FSlateFontInfo& CellFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
		const FMargin& CellMargin = FMargin(4.f);

		const auto& TextBlockCreator_Lambda =
			[this, &CellFont, &CellMargin](const FText& InText) -> TSharedRef<STextBlock>
		{
			return SNew(STextBlock)
					.Text(InText)
					.Font(CellFont)
					.Margin(CellMargin)
					.HighlightText(*HighlightText);
		};

		if (ColumnName == ColumnId_PrimaryIdLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(Item->PrimaryAssetId.PrimaryAssetName.ToString()));
		}
		if (ColumnName == ColumnId_ItemIdLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(FString::FromInt(Item->Id)));
		}
		if (ColumnName == ColumnId_NameLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(Item->Name.ToString()));
		}
		if (ColumnName == ColumnId_TypeLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(
				ElementusEdHelper::EnumToString(TEXT("EElementusItemType"), static_cast<uint8>(Item->Type))));
		}
		if (ColumnName == ColumnId_ClassLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(Item->Class.ToString()));
		}
		if (ColumnName == ColumnId_ValueLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(FString::SanitizeFloat(Item->Value)));
		}
		if (ColumnName == ColumnId_WeightLabel)
		{
			return TextBlockCreator_Lambda(FText::FromString(FString::SanitizeFloat(Item->Weight)));
		}

		return SNullWidget::NullWidget;
	}

private:
	FElementusItemPtr Item;
};

void SElementusTable::Construct([[maybe_unused]] const FArguments& InArgs)
{
	const TSharedPtr<SHeaderRow> HeaderRow = SNew(SHeaderRow);

	const auto& HeaderColumnCreator_Lambda =
		[&](const FName& ColumnId, const FString& ColumnText,
		    const float InWidth = 0.5f) -> const SHeaderRow::FColumn::FArguments
	{
		return SHeaderRow::Column(ColumnId)
		       .DefaultLabel(FText::FromString(ColumnText))
		       .FillWidth(InWidth)
		       .SortMode(this, &SElementusTable::GetColumnSort, ColumnId)
		       .OnSort(this, &SElementusTable::OnColumnSort)
		       .HeaderComboVisibility(EHeaderComboVisibility::OnHover);
	};

	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_PrimaryIdLabel, "Primary Asset Id", 0.75f));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ItemIdLabel, "Id"));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_NameLabel, "Name"));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_TypeLabel, "Type"));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ClassLabel, "Class"));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ValueLabel, "Value"));
	HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_WeightLabel, "Weight"));

	EdListView = SNew(SListView<FElementusItemPtr>)
					.ListItemsSource(&ItemArr)
					.SelectionMode(ESelectionMode::Multi)
					.IsFocusable(true)
					.OnGenerateRow(this, &SElementusTable::OnGenerateWidgetForList)
					.HeaderRow(HeaderRow);

	ChildSlot
	[
		SNew(SBorder)
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
		[
			EdListView.ToSharedRef()
		]
	];
}

TSharedRef<ITableRow> SElementusTable::OnGenerateWidgetForList(const FElementusItemPtr InItem,
                                                               const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(SElementusItemTableRow, OwnerTable, InItem)
				.Visibility(this, &SElementusTable::GetIsVisible, InItem)
				.HightlightTextSource(&SearchText);
}

EVisibility SElementusTable::GetIsVisible(const FElementusItemPtr InItem) const
{
	EVisibility Output;

	if ([&InItem](const FString& InText) -> const bool
		{
			return InText.IsEmpty()
				|| InItem->PrimaryAssetId.ToString().Contains(InText)
				|| FString::FromInt(InItem->Id).Contains(InText)
				|| InItem->Name.ToString().Contains(InText)
				|| ElementusEdHelper::EnumToString(TEXT("EElementusItemType"),
				                                   static_cast<uint8>(InItem->Type)).Contains(InText)
				|| InItem->Class.ToString().Contains(InText)
				|| FString::SanitizeFloat(InItem->Value).Contains(InText)
				|| FString::SanitizeFloat(InItem->Weight).Contains(InText);
		}(SearchText.ToString())
		&& (AllowedTypes.Contains(static_cast<uint8>(InItem->Type)) || AllowedTypes.IsEmpty()))
	{
		Output = EVisibility::Visible;
	}
	else
	{
		Output = EVisibility::Collapsed;
	}

	return Output;
}

void SElementusTable::OnSearchTextModified(const FText& InText)
{
	SearchText = InText;
	EdListView->RebuildList();
}

void SElementusTable::OnSearchTypeModified(const ECheckBoxState InState, const int32 InType)
{
	switch (InState)
	{
	case ECheckBoxState::Checked:
		AllowedTypes.Add(InType);
		break;
	case ECheckBoxState::Unchecked:
		AllowedTypes.Remove(InType);
		break;
	case ECheckBoxState::Undetermined:
		AllowedTypes.Remove(InType);
		break;
	}
	EdListView->RequestListRefresh();
}

void SElementusTable::UpdateItemList()
{
	ItemArr.Empty();

	for (const auto& Iterator : UElementusInventoryFunctions::GetElementusItemIds())
	{
		ItemArr.Add(MakeShareable<FElementusItemRowData>(new FElementusItemRowData(Iterator)));
	}

	EdListView->RequestListRefresh();
}

void SElementusTable::OnColumnSort([[maybe_unused]] const EColumnSortPriority::Type SortPriority,
                                   const FName& ColumnName,
                                   const EColumnSortMode::Type SortMode)
{
	ColumnBeingSorted = ColumnName;
	CurrentSortMode = SortMode;

	const auto& CompareLambda =
		[&](const auto& Val1, const auto& Val2) -> bool
	{
		switch (SortMode)
		{
		case EColumnSortMode::Ascending:
			return Val1 < Val2;
		case EColumnSortMode::Descending:
			return Val1 > Val2;
		case EColumnSortMode::None:
			return Val1 < Val2;
		default:
			return false;
		}
	};

	const auto& SortLambda =
		[&](const TSharedPtr<FElementusItemRowData>& Val1, const TSharedPtr<FElementusItemRowData>& Val2) -> bool
	{
		if (ColumnName == ColumnId_PrimaryIdLabel)
		{
			return CompareLambda(Val1->PrimaryAssetId.ToString(), Val2->PrimaryAssetId.ToString());
		}

		if (ColumnName == ColumnId_ItemIdLabel)
		{
			return CompareLambda(Val1->Id, Val2->Id);
		}

		if (ColumnName == ColumnId_NameLabel)
		{
			return CompareLambda(Val1->Name.ToString(), Val2->Name.ToString());
		}

		if (ColumnName == ColumnId_TypeLabel)
		{
			const auto& ItemTypeToString_Lambda = [&](const EElementusItemType& InType) -> FString
			{
				return *ElementusEdHelper::EnumToString(TEXT("EElementusItemType"), static_cast<uint8>(InType));
			};
			return CompareLambda(ItemTypeToString_Lambda(Val1->Type), ItemTypeToString_Lambda(Val2->Type));
		}

		if (ColumnName == ColumnId_ClassLabel)
		{
			return CompareLambda(Val1->Class.ToString(), Val2->Class.ToString());
		}

		if (ColumnName == ColumnId_ValueLabel)
		{
			return CompareLambda(Val1->Value, Val2->Value);
		}

		if (ColumnName == ColumnId_WeightLabel)
		{
			return CompareLambda(Val1->Weight, Val2->Weight);
		}

		return false;
	};

	Algo::Sort(ItemArr, SortLambda);
	EdListView->RequestListRefresh();
}

EColumnSortMode::Type SElementusTable::GetColumnSort(const FName ColumnId) const
{
	if (ColumnBeingSorted != ColumnId)
	{
		return EColumnSortMode::None;
	}

	return CurrentSortMode;
}