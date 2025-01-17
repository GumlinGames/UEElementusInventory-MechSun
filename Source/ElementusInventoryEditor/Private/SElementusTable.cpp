// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEElementusInventory

#include "SElementusTable.h"
#include <Management/ElementusInventoryFunctions.h>
#include <Management/ElementusInventoryData.h>
#include <Subsystems/AssetEditorSubsystem.h>
#include <Engine/AssetManager.h>

static const FName ColumnId_PrimaryIdLabel("PrimaryAssetId");
static const FName ColumnId_ItemIdLabel("Id");
static const FName ColumnId_NameLabel("Name");
static const FName ColumnId_TypeLabel("Type");
static const FName ColumnId_ObjectLabel("Object");
static const FName ColumnId_ClassLabel("Class");
static const FName ColumnId_ValueLabel("Value");
static const FName ColumnId_WeightLabel("Weight");

class SElementusItemTableRow final : public SMultiColumnTableRow<FElementusItemPtr>
{
public:
    SLATE_BEGIN_ARGS(SElementusItemTableRow) : _HightlightTextSource()
        {
        }

        SLATE_ARGUMENT(TSharedPtr<FText>, HightlightTextSource)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const FElementusItemPtr InEntryItem)
    {
        HighlightText = InArgs._HightlightTextSource;
        Item = InEntryItem;

        SMultiColumnTableRow<FElementusItemPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
    }

protected:
    virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
    {
        const FSlateFontInfo CellFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
        const FMargin CellMargin(4.f);

        const auto TextBlockCreator_Lambda = [this, &CellFont, &CellMargin](const FText& InText) -> TSharedRef<STextBlock>
            {
                return SNew(STextBlock)
                    .Text(InText)
                    .Font(CellFont)
                    .Margin(CellMargin)
                    .HighlightText(HighlightText.IsValid() ? *HighlightText.Get() : FText::GetEmpty());
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
            return TextBlockCreator_Lambda(FText::FromString(UElementusInventoryFunctions::ElementusItemEnumTypeToString(Item->Type)));
        }

        if (ColumnName == ColumnId_ObjectLabel)
        {
            return TextBlockCreator_Lambda(FText::FromString(Item->Object.ToString()));
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
    TSharedPtr<FText> HighlightText;
};

void SElementusTable::Construct([[maybe_unused]] const FArguments&)
{
    const TSharedPtr<SHeaderRow> HeaderRow = SNew(SHeaderRow);

    const auto HeaderColumnCreator_Lambda = [this](const FName& ColumnId, const FString& ColumnText, const float InWidth = 1.f) -> const SHeaderRow::FColumn::FArguments
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
    HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ObjectLabel, "Object"));
    HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ClassLabel, "Class"));
    HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_ValueLabel, "Value"));
    HeaderRow->AddColumn(HeaderColumnCreator_Lambda(ColumnId_WeightLabel, "Weight"));

    ChildSlot
        [
            ConstructContent(HeaderRow.ToSharedRef())
        ];

    UAssetManager::CallOrRegister_OnCompletedInitialScan(
        FSimpleMulticastDelegate::FDelegate::CreateLambda(
            [this]
            {
                UpdateItemList();
            }
        )
    );
}

TSharedRef<SWidget> SElementusTable::ConstructContent(const TSharedRef<SHeaderRow> HeaderRow)
{
    return SAssignNew(EdListView, SListView<FElementusItemPtr>)
        .ListItemsSource(&ItemArr)
        .SelectionMode(ESelectionMode::Multi)
        .IsFocusable(true)
        .OnGenerateRow(this, &SElementusTable::OnGenerateWidgetForList)
        .HeaderRow(HeaderRow)
        .OnMouseButtonDoubleClick(this, &SElementusTable::OnTableItemDoubleClicked);
}

TSharedRef<ITableRow> SElementusTable::OnGenerateWidgetForList(const FElementusItemPtr InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
    return SNew(SElementusItemTableRow, OwnerTable, InItem)
        .Visibility(this, &SElementusTable::GetIsVisible, InItem)
        .HightlightTextSource(SearchText);
}

void SElementusTable::OnTableItemDoubleClicked(const FElementusItemPtr ElementusItemRowData) const
{
    if (const UAssetManager* const AssetManager = UAssetManager::GetIfValid())
    {
        UAssetEditorSubsystem* const AssetEditorSubsystem = NewObject<UAssetEditorSubsystem>();
        const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(ElementusItemRowData->PrimaryAssetId);

        AssetEditorSubsystem->OpenEditorForAsset(AssetPath.ToString());
    }
}

EVisibility SElementusTable::GetIsVisible(const FElementusItemPtr InItem) const
{
    EVisibility Output;

    if ([&InItem](const FString& InText) -> const bool
        {
            return InText.IsEmpty()
                || InItem->PrimaryAssetId.ToString().Contains(InText, ESearchCase::IgnoreCase)
                || FString::FromInt(InItem->Id).Contains(InText, ESearchCase::IgnoreCase)
                || InItem->Name.ToString().Contains(InText, ESearchCase::IgnoreCase)
                || UElementusInventoryFunctions::ElementusItemEnumTypeToString(InItem->Type).Contains(InText, ESearchCase::IgnoreCase)
                || InItem->Class.ToString().Contains(InText, ESearchCase::IgnoreCase)
                || FString::SanitizeFloat(InItem->Value).Contains(InText, ESearchCase::IgnoreCase)
                || FString::SanitizeFloat(InItem->Weight).Contains(InText, ESearchCase::IgnoreCase);
        }(SearchText.IsValid() ? SearchText->ToString() : FString())
                && (AllowedTypes.Contains(static_cast<uint8>(InItem->Type)) || UElementusInventoryFunctions::HasEmptyParam(AllowedTypes)))
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
    SearchText = MakeShared<FText>(InText);
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

    for (const FPrimaryAssetId& Iterator : UElementusInventoryFunctions::GetAllElementusItemIds())
    {
        ItemArr.Add(MakeShared<FElementusItemRowData>(Iterator));
    }

    EdListView->RequestListRefresh();

    if (const UAssetManager* const AssetManager = UAssetManager::GetIfValid(); IsValid(AssetManager) && AssetManager->HasInitialScanCompleted() && UElementusInventoryFunctions::HasEmptyParam(ItemArr))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Asset Manager could not find any Elementus Items. Please check your Asset Manager settings.")));
    }
}

TArray<FElementusItemPtr> SElementusTable::GetSelectedItems() const
{
    return EdListView->GetSelectedItems();
}

void SElementusTable::OnColumnSort([[maybe_unused]] const EColumnSortPriority::Type, const FName& ColumnName, const EColumnSortMode::Type SortMode)
{
    ColumnBeingSorted = ColumnName;
    CurrentSortMode = SortMode;

    const auto Compare_Lambda = [&SortMode](const auto& Val1, const auto& Val2) -> bool
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

    const auto Sort_Lambda = [&ColumnName, &Compare_Lambda](const FElementusItemPtr& Val1, const FElementusItemPtr& Val2) -> bool
        {
            if (ColumnName == ColumnId_PrimaryIdLabel)
            {
                return Compare_Lambda(Val1->PrimaryAssetId.ToString(), Val2->PrimaryAssetId.ToString());
            }

            if (ColumnName == ColumnId_ItemIdLabel)
            {
                return Compare_Lambda(Val1->Id, Val2->Id);
            }

            if (ColumnName == ColumnId_NameLabel)
            {
                return Compare_Lambda(Val1->Name.ToString(), Val2->Name.ToString());
            }

            if (ColumnName == ColumnId_TypeLabel)
            {
                const auto ItemTypeToString_Lambda = [&](const EElementusItemType& InType) -> FString
                    {
                        return *UElementusInventoryFunctions::ElementusItemEnumTypeToString(InType);
                    };

                return Compare_Lambda(ItemTypeToString_Lambda(Val1->Type), ItemTypeToString_Lambda(Val2->Type));
            }

            if (ColumnName == ColumnId_ClassLabel)
            {
                return Compare_Lambda(Val1->Class.ToString(), Val2->Class.ToString());
            }

            if (ColumnName == ColumnId_ValueLabel)
            {
                return Compare_Lambda(Val1->Value, Val2->Value);
            }

            if (ColumnName == ColumnId_WeightLabel)
            {
                return Compare_Lambda(Val1->Weight, Val2->Weight);
            }

            return false;
        };

    Algo::Sort(ItemArr, Sort_Lambda);
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