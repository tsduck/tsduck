//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup cpp
//!  Unicode characters.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    class UString;

    //!
    //! UTF-16 character.
    //!
    typedef char16_t UChar;

    //!
    //! Case sensitivity used on string operations.
    //!
    enum CaseSensitivity {
        CASE_SENSITIVE,     //!< The operation is case-sensitive.
        CASE_INSENSITIVE    //!< The operation is not case-sensitive.
    };

    //!
    //! Characteristics of a character.
    //! Bitwise combinations are allowed.
    //!
    enum : uint32_t {
        CCHAR_LETTER   = 0x0001,  //!< The character is a letter.
        CCHAR_DIGIT    = 0x0002,  //!< The character is a digit.
        CCHAR_HEXA     = 0x0004,  //!< The character is an hexadecimal digit.
        CCHAR_LATIN    = 0x0008,  //!< The character is latin.
        CCHAR_GREEK    = 0x0010,  //!< The character is greek.
        CCHAR_HEBREW   = 0x0020,  //!< The character is hebrew.
        CCHAR_ARABIC   = 0x0040,  //!< The character is arabic.
        CCHAR_THAI     = 0x0080,  //!< The character is thai.
        CCHAR_CYRILLIC = 0x0100,  //!< The character is cyrillic.
        CCHAR_CDIACRIT = 0x0200,  //!< The character is combining diacritical.
        CCHAR_SPACE    = 0x0400,  //!< The character is space.
        CCHAR_PRINT    = 0x0800,  //!< The character is printable.
    };

    //!
    //! Get the characteristics of a character.
    //! @param [in] c A character.
    //! @return Bitmask of the characteristics of @a c.
    //!
    TSDUCKDLL uint32_t UCharacteristics(UChar c);

    //!
    //! Check if a character is a space.
    //! @param [in] c A character.
    //! @return True if @a c is a space, tab, new line character.
    //!
    TSDUCKDLL inline bool IsSpace(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_SPACE) != 0;
    }

    //!
    //! Check if a character is printable according to the current C locale.
    //! @param [in] c A character.
    //! @return True if @a c is a printable character.
    //!
    TSDUCKDLL inline bool IsPrintable(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_PRINT) != 0;
    }

    //!
    //! Check if a character is a letter.
    //! @param [in] c A character.
    //! @return True if @a c is a letter.
    //!
    TSDUCKDLL inline bool IsAlpha(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_LETTER) != 0;
    }

    //!
    //! Check if a character is a decimal digit.
    //! @param [in] c A character.
    //! @return True if @a c is a decimal digit.
    //!
    TSDUCKDLL inline bool IsDigit(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_DIGIT) != 0;
    }

    //!
    //! Check if a character is an hexadecimal digit.
    //! @param [in] c A character.
    //! @return True if @a c is an hexadecimal digit.
    //!
    TSDUCKDLL inline bool IsHexa(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_HEXA) != 0;
    }

    //!
    //! Convert a character representing a multi-base integer digit into the corresponding integer value.
    //!
    //! Characters '0'..'9' are converted to 0..9.
    //! Characters 'a'..'z' and 'A'..'Z' are converted to 10..35.
    //! This function can be used to convert decimal digits, hexadecimal and any other base up to base 36.
    //!
    //! @param [in] c A character to convert.
    //! @param [in] base The base of the integer representation, must be in the range 2 to 36.
    //! @param [in] defaultValue The value to return on invalid character.
    //! @return The corresponding integer value or the default value in case of error.
    //!
    TSDUCKDLL int ToDigit(UChar c, int base = 10, int defaultValue = -1);

    //!
    //! Check if a character is a lower case letter.
    //! @param [in] c A character.
    //! @return True if @a c is a lower case letter.
    //!
    TSDUCKDLL bool IsLower(UChar c);

    //!
    //! Check if a character is an upper case letter.
    //! @param [in] c A character.
    //! @return True if @a c is an upper case letter.
    //!
    TSDUCKDLL bool IsUpper(UChar c);

    //!
    //! Convert a character to lowercase.
    //! @param [in] c A character to convert to lowercase.
    //! @return @a c converted to lowercase.
    //!
    TSDUCKDLL UChar ToLower(UChar c);

    //!
    //! Convert a character to uppercase.
    //! @param [in] c A character to convert to uppercase.
    //! @return @a c converted to uppercase.
    //!
    TSDUCKDLL UChar ToUpper(UChar c);

    //!
    //! Check two characters match, case sensitive or insensitive.
    //! @param [in] c1 First character.
    //! @param [in] c2 Second character.
    //! @param [in] cs Case sensitivity of the comparision.
    //! @return True if the two characters match.
    //!
    TSDUCKDLL bool Match(UChar c1, UChar c2, CaseSensitivity cs);

    //!
    //! Check if a character contains an accent.
    //! @param [in] c A character.
    //! @return True if @a c contains an accent.
    //!
    TSDUCKDLL bool IsAccented(UChar c);

    //!
    //! Remove all forms of accent or composition from a character.
    //! @param [in] c A character.
    //! @return A string containing @a c without accent. This is a string and not a char
    //! since composed characters can be translated as two characters.
    //!
    TSDUCKDLL UString RemoveAccent(UChar c);

    //!
    //! Check if a character is a combining diacritical character.
    //! Such a character, when printed, is combined with the preceding character.
    //! @param [in] c A character.
    //! @return True if @a c is a combining diacritical character.
    //!
    TSDUCKDLL inline bool IsCombiningDiacritical(UChar c)
    {
        return (UCharacteristics(c) & CCHAR_CDIACRIT) != 0;
    }

    //!
    //! Check if a character is a "leading surrogate" value.
    //!
    //! In the most general form, a Unicode character needs 21 bits to be represented.
    //! However, most characters can be represented using 16 bits and are implemented
    //! as only one 16-bit value in UTF-16 or one single UChar. Any Unicode character
    //! which cannot be represented within 16 bits needs two consecutive UChar values
    //! in an UTF-16 string. These two values are called a "surrogate pair". The first
    //! ("leading") and second ("trailing") value of a surrogate pair are specially
    //! coded and can be identified as such.
    //!
    //! @param [in] c A character.
    //! @return True if @a c is a "leading surrogate" value.
    //! @see IsTrailingSurrogate()
    //!
    TSDUCKDLL inline bool IsLeadingSurrogate(UChar c)
    {
        return (int(c) & 0xFC00) == 0xD800;
    }

    //!
    //! Check if a character is a "trailing surrogate" value.
    //! @param [in] c A character.
    //! @return True if @a c is a "trailing surrogate" value.
    //! @see IsLeadingSurrogate()
    //!
    TSDUCKDLL inline bool IsTrailingSurrogate(UChar c)
    {
        return (int(c) & 0xFC00) == 0xDC00;
    }

    //!
    //! Check if a 32-bit Unicode code point needs a surrogate pair in UTF-16 representation.
    //! @param [in] cp A 32-bit Unicode code point.
    //! @return True if @a cp needs a surrogate pair.
    //!
    TSDUCKDLL inline bool NeedSurrogate(char32_t cp)
    {
        return cp >= 0x10000;
    }

    //!
    //! Compute the first part of the surrogate pair of a 32-bit Unicode code point (which needs a surrogate pair).
    //! @param [in] cp A 32-bit Unicode code point.
    //! @return The first part of its surrogate pair.
    //! @see NeedSurrogate()
    //!
    TSDUCKDLL inline UChar LeadingSurrogate(char32_t cp)
    {
        return 0xD800 | UChar(((cp - 0x10000) >> 10) & 0x03FF);
    }

    //!
    //! Compute the second part of the surrogate pair of a 32-bit Unicode code point (which needs a surrogate pair).
    //! @param [in] cp A 32-bit Unicode code point.
    //! @return The second part of its surrogate pair.
    //! @see NeedSurrogate()
    //!
    TSDUCKDLL inline UChar TrailingSurrogate(char32_t cp)
    {
        return 0xDC00 | UChar((cp - 0x10000) & 0x03FF);
    }

    //!
    //! Build a 32-bit Unicode code point from a surrogate pair.
    //! @param [in] lead First part of the surrogate pair.
    //! @param [in] trail Second part of the surrogate pair.
    //! @return A 32-bit Unicode code point.
    //!
    TSDUCKDLL inline char32_t FromSurrogatePair(UChar lead, UChar trail)
    {
        return 0x10000 + (((uint32_t(lead) & 0x03FF) << 10) | (uint32_t(trail) & 0x03FF));
    }

    //!
    //! Convert a character into its corresponding HTML sequence.
    //! @param [in] c A character.
    //! @return A string containing the html sequence for @a c.
    //!
    TSDUCKDLL UString ToHTML(UChar c);

    //!
    //! Convert the body on an HTML entity into a character.
    //! @param [in] entity The body on an HTML entity (e.g. "amp" for sequence "\&amp;").
    //! @return The corresponding character or CHAR_NULL if not found.
    //!
    TSDUCKDLL UChar FromHTML(const UString& entity);

    //!
    //! Build a precombined character from its base letter and non-spacing diacritical mark.
    //! @param [in] letter The base letter.
    //! @param [in] mark The non-spacing diacritical mark.
    //! @return The precombined character or CHAR_NULL if the sequence does not have a precombined equivalent.
    //!
    TSDUCKDLL UChar Precombined(UChar letter, UChar mark);

    //!
    //! Decompose a precombined character into its base letter and non-spacing diacritical mark.
    //! @param [in] c A character.
    //! @param [out] letter The base letter for @a c.
    //! @param [out] mark The non-spacing diacritical mark for @a c.
    //! @return True if @a c was successfully decomposed into @a letter and @a diac,
    //! false if @a c is not a precombined character.
    //!
    TSDUCKDLL bool DecomposePrecombined(UChar c, UChar& letter, UChar& mark);

    //
    // The following constants define all characters which can be
    // represented in ISO 8859 character sets.
    // See http://www.unicode.org/Public/MAPPINGS/ISO8859
    //
    static constexpr UChar CHAR_NULL                                   = UChar(0x0000); //!< ISO-8859 Unicode character.
    static constexpr UChar START_OF_HEADING                            = UChar(0x0001); //!< ISO-8859 Unicode character.
    static constexpr UChar START_OF_TEXT                               = UChar(0x0002); //!< ISO-8859 Unicode character.
    static constexpr UChar END_OF_TEXT                                 = UChar(0x0003); //!< ISO-8859 Unicode character.
    static constexpr UChar END_OF_TRANSMISSION                         = UChar(0x0004); //!< ISO-8859 Unicode character.
    static constexpr UChar ENQUIRY                                     = UChar(0x0005); //!< ISO-8859 Unicode character.
    static constexpr UChar ACKNOWLEDGE                                 = UChar(0x0006); //!< ISO-8859 Unicode character.
    static constexpr UChar BELL                                        = UChar(0x0007); //!< ISO-8859 Unicode character.
    static constexpr UChar BACKSPACE                                   = UChar(0x0008); //!< ISO-8859 Unicode character.
    static constexpr UChar HORIZONTAL_TABULATION                       = UChar(0x0009); //!< ISO-8859 Unicode character.
    static constexpr UChar LINE_FEED                                   = UChar(0x000A); //!< ISO-8859 Unicode character.
    static constexpr UChar VERTICAL_TABULATION                         = UChar(0x000B); //!< ISO-8859 Unicode character.
    static constexpr UChar FORM_FEED                                   = UChar(0x000C); //!< ISO-8859 Unicode character.
    static constexpr UChar CARRIAGE_RETURN                             = UChar(0x000D); //!< ISO-8859 Unicode character.
    static constexpr UChar SHIFT_OUT                                   = UChar(0x000E); //!< ISO-8859 Unicode character.
    static constexpr UChar SHIFT_IN                                    = UChar(0x000F); //!< ISO-8859 Unicode character.
    static constexpr UChar DATA_LINK_ESCAPE                            = UChar(0x0010); //!< ISO-8859 Unicode character.
    static constexpr UChar DEVICE_CONTROL_ONE                          = UChar(0x0011); //!< ISO-8859 Unicode character.
    static constexpr UChar DEVICE_CONTROL_TWO                          = UChar(0x0012); //!< ISO-8859 Unicode character.
    static constexpr UChar DEVICE_CONTROL_THREE                        = UChar(0x0013); //!< ISO-8859 Unicode character.
    static constexpr UChar DEVICE_CONTROL_FOUR                         = UChar(0x0014); //!< ISO-8859 Unicode character.
    static constexpr UChar NEGATIVE_ACKNOWLEDGE                        = UChar(0x0015); //!< ISO-8859 Unicode character.
    static constexpr UChar SYNCHRONOUS_IDLE                            = UChar(0x0016); //!< ISO-8859 Unicode character.
    static constexpr UChar END_OF_TRANSMISSION_BLOCK                   = UChar(0x0017); //!< ISO-8859 Unicode character.
    static constexpr UChar CANCEL                                      = UChar(0x0018); //!< ISO-8859 Unicode character.
    static constexpr UChar END_OF_MEDIUM                               = UChar(0x0019); //!< ISO-8859 Unicode character.
    static constexpr UChar SUBSTITUTE                                  = UChar(0x001A); //!< ISO-8859 Unicode character.
    static constexpr UChar ESCAPE                                      = UChar(0x001B); //!< ISO-8859 Unicode character.
    static constexpr UChar FILE_SEPARATOR                              = UChar(0x001C); //!< ISO-8859 Unicode character.
    static constexpr UChar GROUP_SEPARATOR                             = UChar(0x001D); //!< ISO-8859 Unicode character.
    static constexpr UChar RECORD_SEPARATOR                            = UChar(0x001E); //!< ISO-8859 Unicode character.
    static constexpr UChar UNIT_SEPARATOR                              = UChar(0x001F); //!< ISO-8859 Unicode character.
    static constexpr UChar SPACE                                       = UChar(0x0020); //!< ISO-8859 Unicode character.
    static constexpr UChar EXCLAMATION_MARK                            = UChar(0x0021); //!< ISO-8859 Unicode character.
    static constexpr UChar QUOTATION_MARK                              = UChar(0x0022); //!< ISO-8859 Unicode character.
    static constexpr UChar NUMBER_SIGN                                 = UChar(0x0023); //!< ISO-8859 Unicode character.
    static constexpr UChar DOLLAR_SIGN                                 = UChar(0x0024); //!< ISO-8859 Unicode character.
    static constexpr UChar PERCENT_SIGN                                = UChar(0x0025); //!< ISO-8859 Unicode character.
    static constexpr UChar AMPERSAND                                   = UChar(0x0026); //!< ISO-8859 Unicode character.
    static constexpr UChar APOSTROPHE                                  = UChar(0x0027); //!< ISO-8859 Unicode character.
    static constexpr UChar LEFT_PARENTHESIS                            = UChar(0x0028); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_PARENTHESIS                           = UChar(0x0029); //!< ISO-8859 Unicode character.
    static constexpr UChar ASTERISK                                    = UChar(0x002A); //!< ISO-8859 Unicode character.
    static constexpr UChar PLUS_SIGN                                   = UChar(0x002B); //!< ISO-8859 Unicode character.
    static constexpr UChar COMMA                                       = UChar(0x002C); //!< ISO-8859 Unicode character.
    static constexpr UChar HYPHEN_MINUS                                = UChar(0x002D); //!< ISO-8859 Unicode character.
    static constexpr UChar FULL_STOP                                   = UChar(0x002E); //!< ISO-8859 Unicode character.
    static constexpr UChar SOLIDUS                                     = UChar(0x002F); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_ZERO                                  = UChar(0x0030); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_ONE                                   = UChar(0x0031); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_TWO                                   = UChar(0x0032); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_THREE                                 = UChar(0x0033); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_FOUR                                  = UChar(0x0034); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_FIVE                                  = UChar(0x0035); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_SIX                                   = UChar(0x0036); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_SEVEN                                 = UChar(0x0037); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_EIGHT                                 = UChar(0x0038); //!< ISO-8859 Unicode character.
    static constexpr UChar DIGIT_NINE                                  = UChar(0x0039); //!< ISO-8859 Unicode character.
    static constexpr UChar COLON                                       = UChar(0x003A); //!< ISO-8859 Unicode character.
    static constexpr UChar SEMICOLON                                   = UChar(0x003B); //!< ISO-8859 Unicode character.
    static constexpr UChar LESS_THAN_SIGN                              = UChar(0x003C); //!< ISO-8859 Unicode character.
    static constexpr UChar EQUALS_SIGN                                 = UChar(0x003D); //!< ISO-8859 Unicode character.
    static constexpr UChar GREATER_THAN_SIGN                           = UChar(0x003E); //!< ISO-8859 Unicode character.
    static constexpr UChar QUESTION_MARK                               = UChar(0x003F); //!< ISO-8859 Unicode character.
    static constexpr UChar COMMERCIAL_AT                               = UChar(0x0040); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A                      = UChar(0x0041); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_B                      = UChar(0x0042); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C                      = UChar(0x0043); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_D                      = UChar(0x0044); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E                      = UChar(0x0045); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_F                      = UChar(0x0046); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_G                      = UChar(0x0047); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_H                      = UChar(0x0048); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I                      = UChar(0x0049); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_J                      = UChar(0x004A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_K                      = UChar(0x004B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_L                      = UChar(0x004C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_M                      = UChar(0x004D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_N                      = UChar(0x004E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O                      = UChar(0x004F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_P                      = UChar(0x0050); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Q                      = UChar(0x0051); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_R                      = UChar(0x0052); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S                      = UChar(0x0053); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T                      = UChar(0x0054); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U                      = UChar(0x0055); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_V                      = UChar(0x0056); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_W                      = UChar(0x0057); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_X                      = UChar(0x0058); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Y                      = UChar(0x0059); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Z                      = UChar(0x005A); //!< ISO-8859 Unicode character.
    static constexpr UChar LEFT_SQUARE_BRACKET                         = UChar(0x005B); //!< ISO-8859 Unicode character.
    static constexpr UChar REVERSE_SOLIDUS                             = UChar(0x005C); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_SQUARE_BRACKET                        = UChar(0x005D); //!< ISO-8859 Unicode character.
    static constexpr UChar CIRCUMFLEX_ACCENT                           = UChar(0x005E); //!< ISO-8859 Unicode character.
    static constexpr UChar LOW_LINE                                    = UChar(0x005F); //!< ISO-8859 Unicode character.
    static constexpr UChar GRAVE_ACCENT                                = UChar(0x0060); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A                        = UChar(0x0061); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_B                        = UChar(0x0062); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C                        = UChar(0x0063); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_D                        = UChar(0x0064); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E                        = UChar(0x0065); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_F                        = UChar(0x0066); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_G                        = UChar(0x0067); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_H                        = UChar(0x0068); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I                        = UChar(0x0069); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_J                        = UChar(0x006A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_K                        = UChar(0x006B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_L                        = UChar(0x006C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_M                        = UChar(0x006D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_N                        = UChar(0x006E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O                        = UChar(0x006F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_P                        = UChar(0x0070); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Q                        = UChar(0x0071); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_R                        = UChar(0x0072); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S                        = UChar(0x0073); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T                        = UChar(0x0074); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U                        = UChar(0x0075); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_V                        = UChar(0x0076); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_W                        = UChar(0x0077); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_X                        = UChar(0x0078); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Y                        = UChar(0x0079); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Z                        = UChar(0x007A); //!< ISO-8859 Unicode character.
    static constexpr UChar LEFT_CURLY_BRACKET                          = UChar(0x007B); //!< ISO-8859 Unicode character.
    static constexpr UChar VERTICAL_LINE                               = UChar(0x007C); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_CURLY_BRACKET                         = UChar(0x007D); //!< ISO-8859 Unicode character.
    static constexpr UChar TILDE                                       = UChar(0x007E); //!< ISO-8859 Unicode character.
    static constexpr UChar CHAR_DELETE                                 = UChar(0x007F); //!< ISO-8859 Unicode character.
    static constexpr UChar NO_BREAK_SPACE                              = UChar(0x00A0); //!< ISO-8859 Unicode character.
    static constexpr UChar INVERTED_EXCLAMATION_MARK                   = UChar(0x00A1); //!< ISO-8859 Unicode character.
    static constexpr UChar CENT_SIGN                                   = UChar(0x00A2); //!< ISO-8859 Unicode character.
    static constexpr UChar POUND_SIGN                                  = UChar(0x00A3); //!< ISO-8859 Unicode character.
    static constexpr UChar CURRENCY_SIGN                               = UChar(0x00A4); //!< ISO-8859 Unicode character.
    static constexpr UChar YEN_SIGN                                    = UChar(0x00A5); //!< ISO-8859 Unicode character.
    static constexpr UChar BROKEN_BAR                                  = UChar(0x00A6); //!< ISO-8859 Unicode character.
    static constexpr UChar SECTION_SIGN                                = UChar(0x00A7); //!< ISO-8859 Unicode character.
    static constexpr UChar DIAERESIS                                   = UChar(0x00A8); //!< ISO-8859 Unicode character.
    static constexpr UChar COPYRIGHT_SIGN                              = UChar(0x00A9); //!< ISO-8859 Unicode character.
    static constexpr UChar FEMININE_ORDINAL_INDICATOR                  = UChar(0x00AA); //!< ISO-8859 Unicode character.
    static constexpr UChar LEFT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK   = UChar(0x00AB); //!< ISO-8859 Unicode character.
    static constexpr UChar NOT_SIGN                                    = UChar(0x00AC); //!< ISO-8859 Unicode character.
    static constexpr UChar SOFT_HYPHEN                                 = UChar(0x00AD); //!< ISO-8859 Unicode character.
    static constexpr UChar REGISTERED_SIGN                             = UChar(0x00AE); //!< ISO-8859 Unicode character.
    static constexpr UChar MACRON                                      = UChar(0x00AF); //!< ISO-8859 Unicode character.
    static constexpr UChar DEGREE_SIGN                                 = UChar(0x00B0); //!< ISO-8859 Unicode character.
    static constexpr UChar PLUS_MINUS_SIGN                             = UChar(0x00B1); //!< ISO-8859 Unicode character.
    static constexpr UChar SUPERSCRIPT_TWO                             = UChar(0x00B2); //!< ISO-8859 Unicode character.
    static constexpr UChar SUPERSCRIPT_THREE                           = UChar(0x00B3); //!< ISO-8859 Unicode character.
    static constexpr UChar ACUTE_ACCENT                                = UChar(0x00B4); //!< ISO-8859 Unicode character.
    static constexpr UChar MICRO_SIGN                                  = UChar(0x00B5); //!< ISO-8859 Unicode character.
    static constexpr UChar PILCROW_SIGN                                = UChar(0x00B6); //!< ISO-8859 Unicode character.
    static constexpr UChar MIDDLE_DOT                                  = UChar(0x00B7); //!< ISO-8859 Unicode character.
    static constexpr UChar CEDILLA                                     = UChar(0x00B8); //!< ISO-8859 Unicode character.
    static constexpr UChar SUPERSCRIPT_ONE                             = UChar(0x00B9); //!< ISO-8859 Unicode character.
    static constexpr UChar MASCULINE_ORDINAL_INDICATOR                 = UChar(0x00BA); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK  = UChar(0x00BB); //!< ISO-8859 Unicode character.
    static constexpr UChar VULGAR_FRACTION_ONE_QUARTER                 = UChar(0x00BC); //!< ISO-8859 Unicode character.
    static constexpr UChar VULGAR_FRACTION_ONE_HALF                    = UChar(0x00BD); //!< ISO-8859 Unicode character.
    static constexpr UChar VULGAR_FRACTION_THREE_QUARTERS              = UChar(0x00BE); //!< ISO-8859 Unicode character.
    static constexpr UChar INVERTED_QUESTION_MARK                      = UChar(0x00BF); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_GRAVE           = UChar(0x00C0); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_ACUTE           = UChar(0x00C1); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX      = UChar(0x00C2); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_TILDE           = UChar(0x00C3); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS       = UChar(0x00C4); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE      = UChar(0x00C5); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_AE                     = UChar(0x00C6); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C_WITH_CEDILLA         = UChar(0x00C7); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_GRAVE           = UChar(0x00C8); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_ACUTE           = UChar(0x00C9); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX      = UChar(0x00CA); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS       = UChar(0x00CB); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_GRAVE           = UChar(0x00CC); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_ACUTE           = UChar(0x00CD); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX      = UChar(0x00CE); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS       = UChar(0x00CF); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_ETH                    = UChar(0x00D0); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_N_WITH_TILDE           = UChar(0x00D1); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_GRAVE           = UChar(0x00D2); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_ACUTE           = UChar(0x00D3); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX      = UChar(0x00D4); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_TILDE           = UChar(0x00D5); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS       = UChar(0x00D6); //!< ISO-8859 Unicode character.
    static constexpr UChar MULTIPLICATION_SIGN                         = UChar(0x00D7); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_STROKE          = UChar(0x00D8); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_GRAVE           = UChar(0x00D9); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_ACUTE           = UChar(0x00DA); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX      = UChar(0x00DB); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS       = UChar(0x00DC); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Y_WITH_ACUTE           = UChar(0x00DD); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_THORN                  = UChar(0x00DE); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_SHARP_S                  = UChar(0x00DF); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_GRAVE             = UChar(0x00E0); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_ACUTE             = UChar(0x00E1); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX        = UChar(0x00E2); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_TILDE             = UChar(0x00E3); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_DIAERESIS         = UChar(0x00E4); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_RING_ABOVE        = UChar(0x00E5); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_AE                       = UChar(0x00E6); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C_WITH_CEDILLA           = UChar(0x00E7); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_GRAVE             = UChar(0x00E8); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_ACUTE             = UChar(0x00E9); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX        = UChar(0x00EA); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_DIAERESIS         = UChar(0x00EB); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_GRAVE             = UChar(0x00EC); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_ACUTE             = UChar(0x00ED); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX        = UChar(0x00EE); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_DIAERESIS         = UChar(0x00EF); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_ETH                      = UChar(0x00F0); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_N_WITH_TILDE             = UChar(0x00F1); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_GRAVE             = UChar(0x00F2); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_ACUTE             = UChar(0x00F3); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX        = UChar(0x00F4); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_TILDE             = UChar(0x00F5); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_DIAERESIS         = UChar(0x00F6); //!< ISO-8859 Unicode character.
    static constexpr UChar DIVISION_SIGN                               = UChar(0x00F7); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_STROKE            = UChar(0x00F8); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_GRAVE             = UChar(0x00F9); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_ACUTE             = UChar(0x00FA); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX        = UChar(0x00FB); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_DIAERESIS         = UChar(0x00FC); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Y_WITH_ACUTE             = UChar(0x00FD); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_THORN                    = UChar(0x00FE); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Y_WITH_DIAERESIS         = UChar(0x00FF); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_MACRON          = UChar(0x0100); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_MACRON            = UChar(0x0101); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_BREVE           = UChar(0x0102); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_BREVE             = UChar(0x0103); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_A_WITH_OGONEK          = UChar(0x0104); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_A_WITH_OGONEK            = UChar(0x0105); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C_WITH_ACUTE           = UChar(0x0106); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C_WITH_ACUTE             = UChar(0x0107); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX      = UChar(0x0108); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C_WITH_CIRCUMFLEX        = UChar(0x0109); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C_WITH_DOT_ABOVE       = UChar(0x010A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C_WITH_DOT_ABOVE         = UChar(0x010B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_C_WITH_CARON           = UChar(0x010C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_C_WITH_CARON             = UChar(0x010D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_D_WITH_CARON           = UChar(0x010E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_D_WITH_CARON             = UChar(0x010F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_D_WITH_STROKE          = UChar(0x0110); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_D_WITH_STROKE            = UChar(0x0111); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_MACRON          = UChar(0x0112); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_MACRON            = UChar(0x0113); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_DOT_ABOVE       = UChar(0x0116); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_DOT_ABOVE         = UChar(0x0117); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_OGONEK          = UChar(0x0118); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_OGONEK            = UChar(0x0119); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_E_WITH_CARON           = UChar(0x011A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_E_WITH_CARON             = UChar(0x011B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_G_WITH_CIRCUMFLEX      = UChar(0x011C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_G_WITH_CIRCUMFLEX        = UChar(0x011D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_G_WITH_BREVE           = UChar(0x011E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_G_WITH_BREVE             = UChar(0x011F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_G_WITH_DOT_ABOVE       = UChar(0x0120); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_G_WITH_DOT_ABOVE         = UChar(0x0121); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_G_WITH_CEDILLA         = UChar(0x0122); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_G_WITH_CEDILLA           = UChar(0x0123); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_H_WITH_CIRCUMFLEX      = UChar(0x0124); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_H_WITH_CIRCUMFLEX        = UChar(0x0125); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_H_WITH_STROKE          = UChar(0x0126); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_H_WITH_STROKE            = UChar(0x0127); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_TILDE           = UChar(0x0128); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_TILDE             = UChar(0x0129); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_MACRON          = UChar(0x012A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_MACRON            = UChar(0x012B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_OGONEK          = UChar(0x012E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_I_WITH_OGONEK            = UChar(0x012F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE       = UChar(0x0130); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_DOTLESS_I                = UChar(0x0131); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_J_WITH_CIRCUMFLEX      = UChar(0x0134); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_J_WITH_CIRCUMFLEX        = UChar(0x0135); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_K_WITH_CEDILLA         = UChar(0x0136); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_K_WITH_CEDILLA           = UChar(0x0137); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_KRA                      = UChar(0x0138); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_L_WITH_ACUTE           = UChar(0x0139); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_L_WITH_ACUTE             = UChar(0x013A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_L_WITH_CEDILLA         = UChar(0x013B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_L_WITH_CEDILLA           = UChar(0x013C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_L_WITH_CARON           = UChar(0x013D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_L_WITH_CARON             = UChar(0x013E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_L_WITH_STROKE          = UChar(0x0141); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_L_WITH_STROKE            = UChar(0x0142); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_N_WITH_ACUTE           = UChar(0x0143); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_N_WITH_ACUTE             = UChar(0x0144); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_N_WITH_CEDILLA         = UChar(0x0145); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_N_WITH_CEDILLA           = UChar(0x0146); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_N_WITH_CARON           = UChar(0x0147); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_N_WITH_CARON             = UChar(0x0148); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_ENG                    = UChar(0x014A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_ENG                      = UChar(0x014B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_MACRON          = UChar(0x014C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_MACRON            = UChar(0x014D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_O_WITH_DOUBLE_ACUTE    = UChar(0x0150); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_O_WITH_DOUBLE_ACUTE      = UChar(0x0151); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LIGATURE_OE                   = UChar(0x0152); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LIGATURE_OE                     = UChar(0x0153); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_R_WITH_ACUTE           = UChar(0x0154); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_R_WITH_ACUTE             = UChar(0x0155); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_R_WITH_CEDILLA         = UChar(0x0156); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_R_WITH_CEDILLA           = UChar(0x0157); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_R_WITH_CARON           = UChar(0x0158); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_R_WITH_CARON             = UChar(0x0159); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_ACUTE           = UChar(0x015A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_ACUTE             = UChar(0x015B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_CIRCUMFLEX      = UChar(0x015C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX        = UChar(0x015D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_CEDILLA         = UChar(0x015E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_CEDILLA           = UChar(0x015F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_CARON           = UChar(0x0160); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_CARON             = UChar(0x0161); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T_WITH_CEDILLA         = UChar(0x0162); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T_WITH_CEDILLA           = UChar(0x0163); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T_WITH_CARON           = UChar(0x0164); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T_WITH_CARON             = UChar(0x0165); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T_WITH_STROKE          = UChar(0x0166); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T_WITH_STROKE            = UChar(0x0167); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_TILDE           = UChar(0x0168); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_TILDE             = UChar(0x0169); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_MACRON          = UChar(0x016A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_MACRON            = UChar(0x016B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_BREVE           = UChar(0x016C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_BREVE             = UChar(0x016D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_RING_ABOVE      = UChar(0x016E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_RING_ABOVE        = UChar(0x016F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_DOUBLE_ACUTE    = UChar(0x0170); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_DOUBLE_ACUTE      = UChar(0x0171); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_U_WITH_OGONEK          = UChar(0x0172); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_U_WITH_OGONEK            = UChar(0x0173); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX      = UChar(0x0174); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX        = UChar(0x0175); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX      = UChar(0x0176); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX        = UChar(0x0177); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS       = UChar(0x0178); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Z_WITH_ACUTE           = UChar(0x0179); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Z_WITH_ACUTE             = UChar(0x017A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE       = UChar(0x017B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE         = UChar(0x017C); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Z_WITH_CARON           = UChar(0x017D); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Z_WITH_CARON             = UChar(0x017E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_F_WITH_HOOK                     = UChar(0x0192); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_COMMA_BELOW     = UChar(0x0218); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_COMMA_BELOW       = UChar(0x0219); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T_WITH_COMMA_BELOW     = UChar(0x021A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T_WITH_COMMA_BELOW       = UChar(0x021B); //!< ISO-8859 Unicode character.
    static constexpr UChar MODIFIER_LETTER_CIRCUMFLEX_ACCENT           = UChar(0x02C6); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CARON                                       = UChar(0x02C7); //!< ISO-8859 Unicode character.
    static constexpr UChar BREVE                                       = UChar(0x02D8); //!< ISO-8859 Unicode character.
    static constexpr UChar DOT_ABOVE                                   = UChar(0x02D9); //!< ISO-8859 Unicode character.
    static constexpr UChar OGONEK                                      = UChar(0x02DB); //!< ISO-8859 Unicode character.
    static constexpr UChar SMALL_TILDE                                 = UChar(0x02DC); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOUBLE_ACUTE_ACCENT                         = UChar(0x02DD); //!< ISO-8859 Unicode character.
    static constexpr UChar COMBINING_GRAVE_ACCENT                      = UChar(0x0300); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ACUTE_ACCENT                      = UChar(0x0301); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CIRCUMFLEX_ACCENT                 = UChar(0x0302); //!< Combining diacritical character.
    static constexpr UChar COMBINING_TILDE                             = UChar(0x0303); //!< Combining diacritical character.
    static constexpr UChar COMBINING_MACRON                            = UChar(0x0304); //!< Combining diacritical character.
    static constexpr UChar COMBINING_OVERLINE                          = UChar(0x0305); //!< Combining diacritical character.
    static constexpr UChar COMBINING_BREVE                             = UChar(0x0306); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOT_ABOVE                         = UChar(0x0307); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DIAERESIS                         = UChar(0x0308); //!< Combining diacritical character.
    static constexpr UChar COMBINING_HOOK_ABOVE                        = UChar(0x0309); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RING_ABOVE                        = UChar(0x030A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_ACUTE_ACCENT               = UChar(0x030B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CARON                             = UChar(0x030C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_VERTICAL_LINE_ABOVE               = UChar(0x030D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_VERTICAL_LINE_ABOVE        = UChar(0x030E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_GRAVE_ACCENT               = UChar(0x030F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CANDRABINDU                       = UChar(0x0310); //!< Combining diacritical character.
    static constexpr UChar COMBINING_INVERTED_BREVE                    = UChar(0x0311); //!< Combining diacritical character.
    static constexpr UChar COMBINING_TURNED_COMMA_ABOVE                = UChar(0x0312); //!< Combining diacritical character.
    static constexpr UChar COMBINING_COMMA_ABOVE                       = UChar(0x0313); //!< Combining diacritical character.
    static constexpr UChar COMBINING_REVERSED_COMMA_ABOVE              = UChar(0x0314); //!< Combining diacritical character.
    static constexpr UChar COMBINING_COMMA_ABOVE_RIGHT                 = UChar(0x0315); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GRAVE_ACCENT_BELOW                = UChar(0x0316); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ACUTE_ACCENT_BELOW                = UChar(0x0317); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_TACK_BELOW                   = UChar(0x0318); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_TACK_BELOW                  = UChar(0x0319); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_ANGLE_ABOVE                  = UChar(0x031A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_HORN                              = UChar(0x031B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_HALF_RING_BELOW              = UChar(0x031C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_UP_TACK_BELOW                     = UChar(0x031D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOWN_TACK_BELOW                   = UChar(0x031E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_PLUS_SIGN_BELOW                   = UChar(0x031F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_MINUS_SIGN_BELOW                  = UChar(0x0320); //!< Combining diacritical character.
    static constexpr UChar COMBINING_PALATALIZED_HOOK_BELOW            = UChar(0x0321); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RETROFLEX_HOOK_BELOW              = UChar(0x0322); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOT_BELOW                         = UChar(0x0323); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DIAERESIS_BELOW                   = UChar(0x0324); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RING_BELOW                        = UChar(0x0325); //!< Combining diacritical character.
    static constexpr UChar COMBINING_COMMA_BELOW                       = UChar(0x0326); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CEDILLA                           = UChar(0x0327); //!< Combining diacritical character.
    static constexpr UChar COMBINING_OGONEK                            = UChar(0x0328); //!< Combining diacritical character.
    static constexpr UChar COMBINING_VERTICAL_LINE_BELOW               = UChar(0x0329); //!< Combining diacritical character.
    static constexpr UChar COMBINING_BRIDGE_BELOW                      = UChar(0x032A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_INVERTED_DOUBLE_ARCH_BELOW        = UChar(0x032B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CARON_BELOW                       = UChar(0x032C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_CIRCUMFLEX_ACCENT_BELOW           = UChar(0x032D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_BREVE_BELOW                       = UChar(0x032E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_INVERTED_BREVE_BELOW              = UChar(0x032F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_TILDE_BELOW                       = UChar(0x0330); //!< Combining diacritical character.
    static constexpr UChar COMBINING_MACRON_BELOW                      = UChar(0x0331); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LOW_LINE                          = UChar(0x0332); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_LOW_LINE                   = UChar(0x0333); //!< Combining diacritical character.
    static constexpr UChar COMBINING_TILDE_OVERLAY                     = UChar(0x0334); //!< Combining diacritical character.
    static constexpr UChar COMBINING_SHORT_STROKE_OVERLAY              = UChar(0x0335); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LONG_STROKE_OVERLAY               = UChar(0x0336); //!< Combining diacritical character.
    static constexpr UChar COMBINING_SHORT_SOLIDUS_OVERLAY             = UChar(0x0337); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LONG_SOLIDUS_OVERLAY              = UChar(0x0338); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_HALF_RING_BELOW             = UChar(0x0339); //!< Combining diacritical character.
    static constexpr UChar COMBINING_INVERTED_BRIDGE_BELOW             = UChar(0x033A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_SQUARE_BELOW                      = UChar(0x033B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_SEAGULL_BELOW                     = UChar(0x033C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_X_ABOVE                           = UChar(0x033D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_VERTICAL_TILDE                    = UChar(0x033E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_OVERLINE                   = UChar(0x033F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GRAVE_TONE_MARK                   = UChar(0x0340); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ACUTE_TONE_MARK                   = UChar(0x0341); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GREEK_PERISPOMENI                 = UChar(0x0342); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GREEK_KORONIS                     = UChar(0x0343); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GREEK_DIALYTIKA_TONOS             = UChar(0x0344); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GREEK_YPOGEGRAMMENI               = UChar(0x0345); //!< Combining diacritical character.
    static constexpr UChar COMBINING_BRIDGE_ABOVE                      = UChar(0x0346); //!< Combining diacritical character.
    static constexpr UChar COMBINING_EQUALS_SIGN_BELOW                 = UChar(0x0347); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_VERTICAL_LINE_BELOW        = UChar(0x0348); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_ANGLE_BELOW                  = UChar(0x0349); //!< Combining diacritical character.
    static constexpr UChar COMBINING_NOT_TILDE_ABOVE                   = UChar(0x034A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_HOMOTHETIC_ABOVE                  = UChar(0x034B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ALMOST_EQUAL_TO_ABOVE             = UChar(0x034C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_RIGHT_ARROW_BELOW            = UChar(0x034D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_UPWARDS_ARROW_BELOW               = UChar(0x034E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_GRAPHEME_JOINER                   = UChar(0x034F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_ARROWHEAD_ABOVE             = UChar(0x0350); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_HALF_RING_ABOVE              = UChar(0x0351); //!< Combining diacritical character.
    static constexpr UChar COMBINING_FERMATA                           = UChar(0x0352); //!< Combining diacritical character.
    static constexpr UChar COMBINING_X_BELOW                           = UChar(0x0353); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LEFT_ARROWHEAD_BELOW              = UChar(0x0354); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_ARROWHEAD_BELOW             = UChar(0x0355); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_ARROWHEAD_AND_UP_ARROWHEAD_BELOW = UChar(0x0356); //!< Combining diacritical character.
    static constexpr UChar COMBINING_RIGHT_HALF_RING_ABOVE             = UChar(0x0357); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOT_ABOVE_RIGHT                   = UChar(0x0358); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ASTERISK_BELOW                    = UChar(0x0359); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_RING_BELOW                 = UChar(0x035A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_ZIGZAG_ABOVE                      = UChar(0x035B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_BREVE_BELOW                = UChar(0x035C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_BREVE                      = UChar(0x035D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_MACRON                     = UChar(0x035E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_MACRON_BELOW               = UChar(0x035F); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_TILDE                      = UChar(0x0360); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_INVERTED_BREVE             = UChar(0x0361); //!< Combining diacritical character.
    static constexpr UChar COMBINING_DOUBLE_RIGHTWARDS_ARROW_BELOW     = UChar(0x0362); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_A              = UChar(0x0363); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_E              = UChar(0x0364); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_I              = UChar(0x0365); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_O              = UChar(0x0366); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_U              = UChar(0x0367); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_C              = UChar(0x0368); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_D              = UChar(0x0369); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_H              = UChar(0x036A); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_M              = UChar(0x036B); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_R              = UChar(0x036C); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_T              = UChar(0x036D); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_V              = UChar(0x036E); //!< Combining diacritical character.
    static constexpr UChar COMBINING_LATIN_SMALL_LETTER_X              = UChar(0x036F); //!< Combining diacritical character.
    static constexpr UChar GREEK_YPOGEGRAMMENI                         = UChar(0x037A); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_TONOS                                 = UChar(0x0384); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_DIALYTIKA_TONOS                       = UChar(0x0385); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS       = UChar(0x0386); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_EPSILON_WITH_TONOS     = UChar(0x0388); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_ETA_WITH_TONOS         = UChar(0x0389); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS        = UChar(0x038A); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_OMICRON_WITH_TONOS     = UChar(0x038C); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_UPSILON_WITH_TONOS     = UChar(0x038E); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_OMEGA_WITH_TONOS       = UChar(0x038F); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA_AND_TONOS = UChar(0x0390); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_ALPHA                  = UChar(0x0391); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_BETA                   = UChar(0x0392); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_GAMMA                  = UChar(0x0393); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_DELTA                  = UChar(0x0394); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_EPSILON                = UChar(0x0395); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_ZETA                   = UChar(0x0396); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_ETA                    = UChar(0x0397); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_THETA                  = UChar(0x0398); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_IOTA                   = UChar(0x0399); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_KAPPA                  = UChar(0x039A); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_LAMDA                  = UChar(0x039B); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_MU                     = UChar(0x039C); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_NU                     = UChar(0x039D); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_XI                     = UChar(0x039E); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_OMICRON                = UChar(0x039F); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_PI                     = UChar(0x03A0); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_RHO                    = UChar(0x03A1); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_SIGMA                  = UChar(0x03A3); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_TAU                    = UChar(0x03A4); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_UPSILON                = UChar(0x03A5); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_PHI                    = UChar(0x03A6); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_CHI                    = UChar(0x03A7); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_PSI                    = UChar(0x03A8); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_OMEGA                  = UChar(0x03A9); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_IOTA_WITH_DIALYTIKA    = UChar(0x03AA); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_CAPITAL_LETTER_UPSILON_WITH_DIALYTIKA = UChar(0x03AB); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_ALPHA_WITH_TONOS         = UChar(0x03AC); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_EPSILON_WITH_TONOS       = UChar(0x03AD); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_ETA_WITH_TONOS           = UChar(0x03AE); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_IOTA_WITH_TONOS          = UChar(0x03AF); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA_AND_TONOS = UChar(0x03B0); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_ALPHA                    = UChar(0x03B1); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_BETA                     = UChar(0x03B2); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_GAMMA                    = UChar(0x03B3); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_DELTA                    = UChar(0x03B4); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_EPSILON                  = UChar(0x03B5); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_ZETA                     = UChar(0x03B6); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_ETA                      = UChar(0x03B7); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_THETA                    = UChar(0x03B8); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_IOTA                     = UChar(0x03B9); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_KAPPA                    = UChar(0x03BA); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_LAMDA                    = UChar(0x03BB); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_MU                       = UChar(0x03BC); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_NU                       = UChar(0x03BD); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_XI                       = UChar(0x03BE); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_OMICRON                  = UChar(0x03BF); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_PI                       = UChar(0x03C0); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_RHO                      = UChar(0x03C1); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_FINAL_SIGMA              = UChar(0x03C2); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_SIGMA                    = UChar(0x03C3); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_TAU                      = UChar(0x03C4); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_UPSILON                  = UChar(0x03C5); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_PHI                      = UChar(0x03C6); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_CHI                      = UChar(0x03C7); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_PSI                      = UChar(0x03C8); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_OMEGA                    = UChar(0x03C9); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA      = UChar(0x03CA); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA   = UChar(0x03CB); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_OMICRON_WITH_TONOS       = UChar(0x03CC); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_UPSILON_WITH_TONOS       = UChar(0x03CD); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_OMEGA_WITH_TONOS         = UChar(0x03CE); //!< ISO-8859 Unicode character.
    static constexpr UChar GREEK_SMALL_LETTER_THETA_SYMBOL             = UChar(0x03D1); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar GREEK_UPSILON_WITH_HOOK_SYMBOL              = UChar(0x03D2); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar GREEK_PI_SYMBOL                             = UChar(0x03D6); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_IO                  = UChar(0x0401); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_DJE                 = UChar(0x0402); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_GJE                 = UChar(0x0403); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_UKRAINIAN_IE        = UChar(0x0404); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_DZE                 = UChar(0x0405); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_BYELORUSSIAN_UKRAINIAN_I = UChar(0x0406); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_YI                  = UChar(0x0407); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_JE                  = UChar(0x0408); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_LJE                 = UChar(0x0409); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_NJE                 = UChar(0x040A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_TSHE                = UChar(0x040B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_KJE                 = UChar(0x040C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_SHORT_U             = UChar(0x040E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_DZHE                = UChar(0x040F); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_A                   = UChar(0x0410); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_BE                  = UChar(0x0411); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_VE                  = UChar(0x0412); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_GHE                 = UChar(0x0413); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_DE                  = UChar(0x0414); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_IE                  = UChar(0x0415); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_ZHE                 = UChar(0x0416); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_ZE                  = UChar(0x0417); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_I                   = UChar(0x0418); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_SHORT_I             = UChar(0x0419); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_KA                  = UChar(0x041A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_EL                  = UChar(0x041B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_EM                  = UChar(0x041C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_EN                  = UChar(0x041D); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_O                   = UChar(0x041E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_PE                  = UChar(0x041F); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_ER                  = UChar(0x0420); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_ES                  = UChar(0x0421); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_TE                  = UChar(0x0422); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_U                   = UChar(0x0423); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_EF                  = UChar(0x0424); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_HA                  = UChar(0x0425); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_TSE                 = UChar(0x0426); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_CHE                 = UChar(0x0427); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_SHA                 = UChar(0x0428); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_SHCHA               = UChar(0x0429); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_HARD_SIGN           = UChar(0x042A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_YERU                = UChar(0x042B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_SOFT_SIGN           = UChar(0x042C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_E                   = UChar(0x042D); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_YU                  = UChar(0x042E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_CAPITAL_LETTER_YA                  = UChar(0x042F); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_A                     = UChar(0x0430); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_BE                    = UChar(0x0431); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_VE                    = UChar(0x0432); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_GHE                   = UChar(0x0433); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_DE                    = UChar(0x0434); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_IE                    = UChar(0x0435); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_ZHE                   = UChar(0x0436); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_ZE                    = UChar(0x0437); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_I                     = UChar(0x0438); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_SHORT_I               = UChar(0x0439); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_KA                    = UChar(0x043A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_EL                    = UChar(0x043B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_EM                    = UChar(0x043C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_EN                    = UChar(0x043D); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_O                     = UChar(0x043E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_PE                    = UChar(0x043F); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_ER                    = UChar(0x0440); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_ES                    = UChar(0x0441); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_TE                    = UChar(0x0442); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_U                     = UChar(0x0443); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_EF                    = UChar(0x0444); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_HA                    = UChar(0x0445); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_TSE                   = UChar(0x0446); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_CHE                   = UChar(0x0447); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_SHA                   = UChar(0x0448); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_SHCHA                 = UChar(0x0449); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_HARD_SIGN             = UChar(0x044A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_YERU                  = UChar(0x044B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_SOFT_SIGN             = UChar(0x044C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_E                     = UChar(0x044D); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_YU                    = UChar(0x044E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_YA                    = UChar(0x044F); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_IO                    = UChar(0x0451); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_DJE                   = UChar(0x0452); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_GJE                   = UChar(0x0453); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_UKRAINIAN_IE          = UChar(0x0454); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_DZE                   = UChar(0x0455); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_BYELORUSSIAN_UKRAINIAN_I = UChar(0x0456); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_YI                    = UChar(0x0457); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_JE                    = UChar(0x0458); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_LJE                   = UChar(0x0459); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_NJE                   = UChar(0x045A); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_TSHE                  = UChar(0x045B); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_KJE                   = UChar(0x045C); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_SHORT_U               = UChar(0x045E); //!< ISO-8859 Unicode character.
    static constexpr UChar CYRILLIC_SMALL_LETTER_DZHE                  = UChar(0x045F); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_ALEF                          = UChar(0x05D0); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_BET                           = UChar(0x05D1); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_GIMEL                         = UChar(0x05D2); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_DALET                         = UChar(0x05D3); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_HE                            = UChar(0x05D4); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_VAV                           = UChar(0x05D5); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_ZAYIN                         = UChar(0x05D6); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_HET                           = UChar(0x05D7); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_TET                           = UChar(0x05D8); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_YOD                           = UChar(0x05D9); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_FINAL_KAF                     = UChar(0x05DA); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_KAF                           = UChar(0x05DB); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_LAMED                         = UChar(0x05DC); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_FINAL_MEM                     = UChar(0x05DD); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_MEM                           = UChar(0x05DE); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_FINAL_NUN                     = UChar(0x05DF); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_NUN                           = UChar(0x05E0); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_SAMEKH                        = UChar(0x05E1); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_AYIN                          = UChar(0x05E2); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_FINAL_PE                      = UChar(0x05E3); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_PE                            = UChar(0x05E4); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_FINAL_TSADI                   = UChar(0x05E5); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_TSADI                         = UChar(0x05E6); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_QOF                           = UChar(0x05E7); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_RESH                          = UChar(0x05E8); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_SHIN                          = UChar(0x05E9); //!< ISO-8859 Unicode character.
    static constexpr UChar HEBREW_LETTER_TAV                           = UChar(0x05EA); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_COMMA                                = UChar(0x060C); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_SEMICOLON                            = UChar(0x061B); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_QUESTION_MARK                        = UChar(0x061F); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_HAMZA                         = UChar(0x0621); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ALEF_WITH_MADDA_ABOVE         = UChar(0x0622); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ALEF_WITH_HAMZA_ABOVE         = UChar(0x0623); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_WAW_WITH_HAMZA_ABOVE          = UChar(0x0624); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ALEF_WITH_HAMZA_BELOW         = UChar(0x0625); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_YEH_WITH_HAMZA_ABOVE          = UChar(0x0626); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ALEF                          = UChar(0x0627); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_BEH                           = UChar(0x0628); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_TEH_MARBUTA                   = UChar(0x0629); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_TEH                           = UChar(0x062A); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_THEH                          = UChar(0x062B); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_JEEM                          = UChar(0x062C); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_HAH                           = UChar(0x062D); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_KHAH                          = UChar(0x062E); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_DAL                           = UChar(0x062F); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_THAL                          = UChar(0x0630); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_REH                           = UChar(0x0631); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ZAIN                          = UChar(0x0632); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_SEEN                          = UChar(0x0633); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_SHEEN                         = UChar(0x0634); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_SAD                           = UChar(0x0635); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_DAD                           = UChar(0x0636); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_TAH                           = UChar(0x0637); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ZAH                           = UChar(0x0638); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_AIN                           = UChar(0x0639); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_GHAIN                         = UChar(0x063A); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_TATWEEL                              = UChar(0x0640); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_FEH                           = UChar(0x0641); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_QAF                           = UChar(0x0642); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_KAF                           = UChar(0x0643); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_LAM                           = UChar(0x0644); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_MEEM                          = UChar(0x0645); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_NOON                          = UChar(0x0646); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_HEH                           = UChar(0x0647); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_WAW                           = UChar(0x0648); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_ALEF_MAKSURA                  = UChar(0x0649); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_LETTER_YEH                           = UChar(0x064A); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_FATHATAN                             = UChar(0x064B); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_DAMMATAN                             = UChar(0x064C); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_KASRATAN                             = UChar(0x064D); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_FATHA                                = UChar(0x064E); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_DAMMA                                = UChar(0x064F); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_KASRA                                = UChar(0x0650); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_SHADDA                               = UChar(0x0651); //!< ISO-8859 Unicode character.
    static constexpr UChar ARABIC_SUKUN                                = UChar(0x0652); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KO_KAI                       = UChar(0x0E01); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHO_KHAI                     = UChar(0x0E02); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHO_KHUAT                    = UChar(0x0E03); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHO_KHWAI                    = UChar(0x0E04); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHO_KHON                     = UChar(0x0E05); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHO_RAKHANG                  = UChar(0x0E06); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_NGO_NGU                      = UChar(0x0E07); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_CHO_CHAN                     = UChar(0x0E08); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_CHO_CHING                    = UChar(0x0E09); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_CHO_CHANG                    = UChar(0x0E0A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SO_SO                        = UChar(0x0E0B); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_CHO_CHOE                     = UChar(0x0E0C); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_YO_YING                      = UChar(0x0E0D); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_DO_CHADA                     = UChar(0x0E0E); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_TO_PATAK                     = UChar(0x0E0F); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_THAN                     = UChar(0x0E10); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_NANGMONTHO               = UChar(0x0E11); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_PHUTHAO                  = UChar(0x0E12); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_NO_NEN                       = UChar(0x0E13); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_DO_DEK                       = UChar(0x0E14); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_TO_TAO                       = UChar(0x0E15); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_THUNG                    = UChar(0x0E16); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_THAHAN                   = UChar(0x0E17); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THO_THONG                    = UChar(0x0E18); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_NO_NU                        = UChar(0x0E19); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_BO_BAIMAI                    = UChar(0x0E1A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PO_PLA                       = UChar(0x0E1B); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PHO_PHUNG                    = UChar(0x0E1C); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_FO_FA                        = UChar(0x0E1D); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PHO_PHAN                     = UChar(0x0E1E); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_FO_FAN                       = UChar(0x0E1F); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PHO_SAMPHAO                  = UChar(0x0E20); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MO_MA                        = UChar(0x0E21); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_YO_YAK                       = UChar(0x0E22); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_RO_RUA                       = UChar(0x0E23); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_RU                           = UChar(0x0E24); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_LO_LING                      = UChar(0x0E25); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_LU                           = UChar(0x0E26); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_WO_WAEN                      = UChar(0x0E27); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SO_SALA                      = UChar(0x0E28); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SO_RUSI                      = UChar(0x0E29); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SO_SUA                       = UChar(0x0E2A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_HO_HIP                       = UChar(0x0E2B); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_LO_CHULA                     = UChar(0x0E2C); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_O_ANG                        = UChar(0x0E2D); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_HO_NOKHUK                    = UChar(0x0E2E); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PAIYANNOI                    = UChar(0x0E2F); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_A                       = UChar(0x0E30); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAI_HAN_AKAT                 = UChar(0x0E31); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_AA                      = UChar(0x0E32); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_AM                      = UChar(0x0E33); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_I                       = UChar(0x0E34); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_II                      = UChar(0x0E35); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_UE                      = UChar(0x0E36); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_UEE                     = UChar(0x0E37); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_U                       = UChar(0x0E38); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_UU                      = UChar(0x0E39); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_PHINTHU                      = UChar(0x0E3A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CURRENCY_SYMBOL_BAHT                   = UChar(0x0E3F); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_E                       = UChar(0x0E40); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_AE                      = UChar(0x0E41); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_O                       = UChar(0x0E42); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_AI_MAIMUAN              = UChar(0x0E43); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_SARA_AI_MAIMALAI             = UChar(0x0E44); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_LAKKHANGYAO                  = UChar(0x0E45); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAIYAMOK                     = UChar(0x0E46); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAITAIKHU                    = UChar(0x0E47); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAI_EK                       = UChar(0x0E48); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAI_THO                      = UChar(0x0E49); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAI_TRI                      = UChar(0x0E4A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_MAI_CHATTAWA                 = UChar(0x0E4B); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_THANTHAKHAT                  = UChar(0x0E4C); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_NIKHAHIT                     = UChar(0x0E4D); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_YAMAKKAN                     = UChar(0x0E4E); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_FONGMAN                      = UChar(0x0E4F); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_ZERO                             = UChar(0x0E50); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_ONE                              = UChar(0x0E51); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_TWO                              = UChar(0x0E52); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_THREE                            = UChar(0x0E53); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_FOUR                             = UChar(0x0E54); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_FIVE                             = UChar(0x0E55); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_SIX                              = UChar(0x0E56); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_SEVEN                            = UChar(0x0E57); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_EIGHT                            = UChar(0x0E58); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_DIGIT_NINE                             = UChar(0x0E59); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_ANGKHANKHU                   = UChar(0x0E5A); //!< ISO-8859 Unicode character.
    static constexpr UChar THAI_CHARACTER_KHOMUT                       = UChar(0x0E5B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_B_WITH_DOT_ABOVE       = UChar(0x1E02); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_B_WITH_DOT_ABOVE         = UChar(0x1E03); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_D_WITH_DOT_ABOVE       = UChar(0x1E0A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_D_WITH_DOT_ABOVE         = UChar(0x1E0B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_F_WITH_DOT_ABOVE       = UChar(0x1E1E); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_F_WITH_DOT_ABOVE         = UChar(0x1E1F); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_M_WITH_DOT_ABOVE       = UChar(0x1E40); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_M_WITH_DOT_ABOVE         = UChar(0x1E41); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_P_WITH_DOT_ABOVE       = UChar(0x1E56); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_P_WITH_DOT_ABOVE         = UChar(0x1E57); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_S_WITH_DOT_ABOVE       = UChar(0x1E60); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_S_WITH_DOT_ABOVE         = UChar(0x1E61); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_T_WITH_DOT_ABOVE       = UChar(0x1E6A); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_T_WITH_DOT_ABOVE         = UChar(0x1E6B); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_W_WITH_GRAVE           = UChar(0x1E80); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_W_WITH_GRAVE             = UChar(0x1E81); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_W_WITH_ACUTE           = UChar(0x1E82); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_W_WITH_ACUTE             = UChar(0x1E83); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_W_WITH_DIAERESIS       = UChar(0x1E84); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_W_WITH_DIAERESIS         = UChar(0x1E85); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_CAPITAL_LETTER_Y_WITH_GRAVE           = UChar(0x1EF2); //!< ISO-8859 Unicode character.
    static constexpr UChar LATIN_SMALL_LETTER_Y_WITH_GRAVE             = UChar(0x1EF3); //!< ISO-8859 Unicode character.
    static constexpr UChar EN_SPACE                                    = UChar(0x2002); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar EM_SPACE                                    = UChar(0x2003); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar THIN_SPACE                                  = UChar(0x2009); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ZERO_WIDTH_SPACE                            = UChar(0x200B); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ZERO_WIDTH_NON_JOINER                       = UChar(0x200C); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ZERO_WIDTH_JOINER                           = UChar(0x200D); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_TO_RIGHT_MARK                          = UChar(0x200E); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_TO_LEFT_MARK                          = UChar(0x200F); //!< ISO-8859 Unicode character.
    static constexpr UChar EN_DASH                                     = UChar(0x2013); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar EM_DASH                                     = UChar(0x2014); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar HORIZONTAL_BAR                              = UChar(0x2015); //!< ISO-8859 Unicode character.
    static constexpr UChar DOUBLE_LOW_LINE                             = UChar(0x2017); //!< ISO-8859 Unicode character.
    static constexpr UChar LEFT_SINGLE_QUOTATION_MARK                  = UChar(0x2018); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_SINGLE_QUOTATION_MARK                 = UChar(0x2019); //!< ISO-8859 Unicode character.
    static constexpr UChar SINGLE_LOW_9_QUOTATION_MARK                 = UChar(0x201A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_DOUBLE_QUOTATION_MARK                  = UChar(0x201C); //!< ISO-8859 Unicode character.
    static constexpr UChar RIGHT_DOUBLE_QUOTATION_MARK                 = UChar(0x201D); //!< ISO-8859 Unicode character.
    static constexpr UChar DOUBLE_LOW_9_QUOTATION_MARK                 = UChar(0x201E); //!< ISO-8859 Unicode character.
    static constexpr UChar DAGGER                                      = UChar(0x2020); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOUBLE_DAGGER                               = UChar(0x2021); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BULLET                                      = UChar(0x2022); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar HORIZONTAL_ELLIPSIS                         = UChar(0x2026); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar PER_MILLE_SIGN                              = UChar(0x2030); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar PRIME                                       = UChar(0x2032); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOUBLE_PRIME                                = UChar(0x2033); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK   = UChar(0x2039); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK  = UChar(0x203A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar OVERLINE                                    = UChar(0x203E); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar FRACTION_SLASH                              = UChar(0x2044); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar WORD_JOINER                                 = UChar(0x2060); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar EURO_SIGN                                   = UChar(0x20AC); //!< ISO-8859 Unicode character.
    static constexpr UChar DRACHMA_SIGN                                = UChar(0x20AF); //!< ISO-8859 Unicode character.
    static constexpr UChar BLACKLETTER_CAPITAL_I                       = UChar(0x2111); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar NUMERO_SIGN                                 = UChar(0x2116); //!< ISO-8859 Unicode character.
    static constexpr UChar SCRIPT_CAPITAL_P                            = UChar(0x2118); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BLACKLETTER_CAPITAL_R                       = UChar(0x211C); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar TRADE_MARK_SIGN                             = UChar(0x2122); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ALEF_SYMBOL                                 = UChar(0x2135); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFTWARDS_ARROW                             = UChar(0x2190); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar UPWARDS_ARROW                               = UChar(0x2191); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar RIGHTWARDS_ARROW                            = UChar(0x2192); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOWNWARDS_ARROW                             = UChar(0x2193); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_RIGHT_ARROW                            = UChar(0x2194); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOWNWARDS_ARROW_WITH_CORNER_LEFTWARDS       = UChar(0x21B5); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFTWARDS_DOUBLE_ARROW                      = UChar(0x21D0); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar UPWARDS_DOUBLE_ARROW                        = UChar(0x21D1); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar RIGHTWARDS_DOUBLE_ARROW                     = UChar(0x21D2); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOWNWARDS_DOUBLE_ARROW                      = UChar(0x21D3); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_RIGHT_DOUBLE_ARROW                     = UChar(0x21D4); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar FOR_ALL                                     = UChar(0x2200); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar PARTIAL_DIFFERENTIAL                        = UChar(0x2202); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar THERE_EXISTS                                = UChar(0x2203); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar EMPTY_SET                                   = UChar(0x2205); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar NABLA                                       = UChar(0x2207); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ELEMENT_OF                                  = UChar(0x2208); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar NOT_AN_ELEMENT_OF                           = UChar(0x2209); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CONTAINS_AS_MEMBER                          = UChar(0x220B); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar N_ARY_PRODUCT                               = UChar(0x220F); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar N_ARY_SUMATION                              = UChar(0x2211); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar MINUS_SIGN                                  = UChar(0x2212); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ASTERISK_OPERATOR                           = UChar(0x2217); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SQUARE_ROOT                                 = UChar(0x221A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar PROPORTIONAL_TO                             = UChar(0x221D); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CHAR_INFINITY                               = UChar(0x221E); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ANGLE                                       = UChar(0x2220); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LOGICAL_AND                                 = UChar(0x2227); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LOGICAL_OR                                  = UChar(0x2228); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar INTERSECTION                                = UChar(0x2229); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar UNION                                       = UChar(0x222A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar INTEGRAL                                    = UChar(0x222B); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar THEREFORE                                   = UChar(0x2234); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar TILDE_OPERATOR                              = UChar(0x223C); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar APPROXIMATELY_EQUAL_TO                      = UChar(0x2245); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar ALMOST_EQUAL_TO                             = UChar(0x2248); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar NOT_EQUAL_TO                                = UChar(0x2260); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar IDENTICAL_TO                                = UChar(0x2261); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LESS_THAN_OR_EQUAL_TO                       = UChar(0x2264); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar GREATER_THAN_OR_EQUAL_TO                    = UChar(0x2265); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SUBSET_OF                                   = UChar(0x2282); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SUPERSET_OF                                 = UChar(0x2283); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar NOT_A_SUBSET_OF                             = UChar(0x2284); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SUBSET_OF_OR_EQUAL_TO                       = UChar(0x2286); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar SUPERSET_OF_OR_EQUAL_TO                     = UChar(0x2287); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CIRCLED_PLUS                                = UChar(0x2295); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar CIRCLED_TIMES                               = UChar(0x2297); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar UP_TACK                                     = UChar(0x22A5); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar DOT_OPERATOR                                = UChar(0x22C5); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_CEILING                                = UChar(0x2308); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar RIGHT_CEILING                               = UChar(0x2309); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_FLOOR                                  = UChar(0x230A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar RIGHT_FLOOR                                 = UChar(0x230B); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LEFT_POINTING_ANGLE_BRACKET                 = UChar(0x2329); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar RIGHT_POINTING_ANGLE_BRACKET                = UChar(0x232A); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar LOZENGE                                     = UChar(0x25CA); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BLACK_SPADE_SUIT                            = UChar(0x2660); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BLACK_CLUB_SUIT                             = UChar(0x2663); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BLACK_HEART_SUIT                            = UChar(0x2665); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar BLACK_DIAMOND_SUIT                          = UChar(0x2666); //!< Non-ISO-8859 Unicode character.
    static constexpr UChar IDEOGRAPHIC_SPACE                           = UChar(0x3000); //!< CJK character for space.
    static constexpr UChar BYTE_ORDER_MARK                             = UChar(0xFEFF); //!< UTF-16 BOM, aka "ZERO WIDTH NO-BREAK SPACE".
    static constexpr UChar SWAPPED_BYTE_ORDER_MARK                     = UChar(0xFFFE); //!< UTF-16 BOM in wrong byte order, not a character.
}
