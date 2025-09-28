#pragma once

/**
 * Filter types available for each band
 * Band 1 and 5 have restricted filter types
 */
enum class FilterType
{
    // Available for all bands
    Peak,           // Parametric peak/notch
    
    // Low band (Band 1) specific
    LowCut,         // High-pass filter
    LowShelf,       // Low frequency shelf
    
    // High band (Band 5) specific  
    HighCut,        // Low-pass filter
    HighShelf,      // High frequency shelf
    
    // Mid bands (2, 3, 4) can use Peak + both shelves
    Disabled        // Band bypassed
};

/**
 * Band restrictions based on position
 */
enum class BandPosition
{
    Low = 0,        // Band 1: Only LowCut, LowShelf
    LowMid = 1,     // Band 2: Full parametric
    Mid = 2,        // Band 3: Full parametric
    HighMid = 3,    // Band 4: Full parametric
    High = 4        // Band 5: Only HighCut, HighShelf
};

/**
 * Get available filter types for a specific band
 */
class FilterTypeRestrictions
{
public:
    static std::vector<FilterType> getAvailableTypes(BandPosition position)
    {
        switch (position)
        {
            case BandPosition::Low:     // Band 1
                return { FilterType::LowCut, FilterType::LowShelf, FilterType::Disabled };
                
            case BandPosition::High:    // Band 5  
                return { FilterType::HighCut, FilterType::HighShelf, FilterType::Disabled };
                
            case BandPosition::LowMid:  // Band 2
            case BandPosition::Mid:     // Band 3
            case BandPosition::HighMid: // Band 4
                return { 
                    FilterType::Peak, 
                    FilterType::LowShelf, 
                    FilterType::HighShelf, 
                    FilterType::Disabled 
                };
        }
        return { FilterType::Disabled };
    }
    
    static bool isValidTypeForBand(FilterType type, BandPosition position)
    {
        auto availableTypes = getAvailableTypes(position);
        return std::find(availableTypes.begin(), availableTypes.end(), type) != availableTypes.end();
    }
};