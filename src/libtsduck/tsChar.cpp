//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  Unicode characters.
//
//----------------------------------------------------------------------------

#include "tsChar.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Lowercase / uppercase tables.
//----------------------------------------------------------------------------

namespace {
    //
    // The standard functions towlower and towupper fail with some upper/lower
    // pairs of characters. Define here a table with the missing pairs.
    // An equivalence between uppercase and lowercase characters:
    //
    struct UpperLower {
        ts::Char upper;
        ts::Char lower;
    };

    //
    // A pointer to member inside UpperLower.
    //
    typedef ts::Char UpperLower::*UpperLowerField;

    //
    // A better trick to compute number of elements in static arrays.
    //
    template <typename T, size_t S>
    inline size_t ElementCount(const T(&v)[S]) { return S; }

    //
    // A structure describing a table of UpperLower
    //
    struct UpperLowerTable {
        const UpperLower* table;  // Address of first element.
        size_t            size;   // Element count.
    };

    //
    // IMPORTANT: The following tables must remain sorted in ascending order
    // of BOTH upper and lower fields. This is checked by CharSelfTest().
    // When a range of upper/lower values cannot be inserted, another table
    // is created.
    //
    const UpperLower UpperLower1[] = {
        {ts::LATIN_CAPITAL_LETTER_A_WITH_GRAVE, ts::LATIN_SMALL_LETTER_A_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_ACUTE, ts::LATIN_SMALL_LETTER_A_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_TILDE, ts::LATIN_SMALL_LETTER_A_WITH_TILDE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_A_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE, ts::LATIN_SMALL_LETTER_A_WITH_RING_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_C_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_GRAVE, ts::LATIN_SMALL_LETTER_E_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_ACUTE, ts::LATIN_SMALL_LETTER_E_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_E_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_GRAVE, ts::LATIN_SMALL_LETTER_I_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_ACUTE, ts::LATIN_SMALL_LETTER_I_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_I_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_TILDE, ts::LATIN_SMALL_LETTER_N_WITH_TILDE},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_GRAVE, ts::LATIN_SMALL_LETTER_O_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_ACUTE, ts::LATIN_SMALL_LETTER_O_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_TILDE, ts::LATIN_SMALL_LETTER_O_WITH_TILDE},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_O_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_STROKE, ts::LATIN_SMALL_LETTER_O_WITH_STROKE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_GRAVE, ts::LATIN_SMALL_LETTER_U_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_ACUTE, ts::LATIN_SMALL_LETTER_U_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_ACUTE, ts::LATIN_SMALL_LETTER_Y_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_MACRON, ts::LATIN_SMALL_LETTER_A_WITH_MACRON},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_BREVE, ts::LATIN_SMALL_LETTER_A_WITH_BREVE},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_OGONEK, ts::LATIN_SMALL_LETTER_A_WITH_OGONEK},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_ACUTE, ts::LATIN_SMALL_LETTER_C_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_C_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_C_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CARON, ts::LATIN_SMALL_LETTER_C_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_CARON, ts::LATIN_SMALL_LETTER_D_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_STROKE, ts::LATIN_SMALL_LETTER_D_WITH_STROKE},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_MACRON, ts::LATIN_SMALL_LETTER_E_WITH_MACRON},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_E_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_OGONEK, ts::LATIN_SMALL_LETTER_E_WITH_OGONEK},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_CARON, ts::LATIN_SMALL_LETTER_E_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_G_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_BREVE, ts::LATIN_SMALL_LETTER_G_WITH_BREVE},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_G_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_G_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_H_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_H_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_H_WITH_STROKE, ts::LATIN_SMALL_LETTER_H_WITH_STROKE},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_TILDE, ts::LATIN_SMALL_LETTER_I_WITH_TILDE},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_MACRON, ts::LATIN_SMALL_LETTER_I_WITH_MACRON},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_OGONEK, ts::LATIN_SMALL_LETTER_I_WITH_OGONEK},
        {ts::LATIN_CAPITAL_LETTER_J_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_J_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_K_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_K_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_ACUTE, ts::LATIN_SMALL_LETTER_L_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_L_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_CARON, ts::LATIN_SMALL_LETTER_L_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_STROKE, ts::LATIN_SMALL_LETTER_L_WITH_STROKE},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_ACUTE, ts::LATIN_SMALL_LETTER_N_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_N_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_CARON, ts::LATIN_SMALL_LETTER_N_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_MACRON, ts::LATIN_SMALL_LETTER_O_WITH_MACRON},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_DOUBLE_ACUTE, ts::LATIN_SMALL_LETTER_O_WITH_DOUBLE_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_ACUTE, ts::LATIN_SMALL_LETTER_R_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_R_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_CARON, ts::LATIN_SMALL_LETTER_R_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_ACUTE, ts::LATIN_SMALL_LETTER_S_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_S_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CARON, ts::LATIN_SMALL_LETTER_S_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_CEDILLA, ts::LATIN_SMALL_LETTER_T_WITH_CEDILLA},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_CARON, ts::LATIN_SMALL_LETTER_T_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_STROKE, ts::LATIN_SMALL_LETTER_T_WITH_STROKE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_TILDE, ts::LATIN_SMALL_LETTER_U_WITH_TILDE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_MACRON, ts::LATIN_SMALL_LETTER_U_WITH_MACRON},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_BREVE, ts::LATIN_SMALL_LETTER_U_WITH_BREVE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_RING_ABOVE, ts::LATIN_SMALL_LETTER_U_WITH_RING_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_DOUBLE_ACUTE, ts::LATIN_SMALL_LETTER_U_WITH_DOUBLE_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_OGONEK, ts::LATIN_SMALL_LETTER_U_WITH_OGONEK},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX, ts::LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_ACUTE, ts::LATIN_SMALL_LETTER_Z_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_CARON, ts::LATIN_SMALL_LETTER_Z_WITH_CARON},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_COMMA_BELOW, ts::LATIN_SMALL_LETTER_S_WITH_COMMA_BELOW},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_COMMA_BELOW, ts::LATIN_SMALL_LETTER_T_WITH_COMMA_BELOW},
        {ts::GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS, ts::GREEK_SMALL_LETTER_ALPHA_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_EPSILON_WITH_TONOS, ts::GREEK_SMALL_LETTER_EPSILON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_ETA_WITH_TONOS, ts::GREEK_SMALL_LETTER_ETA_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS, ts::GREEK_SMALL_LETTER_IOTA_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_ALPHA, ts::GREEK_SMALL_LETTER_ALPHA},
        {ts::GREEK_CAPITAL_LETTER_BETA, ts::GREEK_SMALL_LETTER_BETA},
        {ts::GREEK_CAPITAL_LETTER_GAMMA, ts::GREEK_SMALL_LETTER_GAMMA},
        {ts::GREEK_CAPITAL_LETTER_DELTA, ts::GREEK_SMALL_LETTER_DELTA},
        {ts::GREEK_CAPITAL_LETTER_EPSILON, ts::GREEK_SMALL_LETTER_EPSILON},
        {ts::GREEK_CAPITAL_LETTER_ZETA, ts::GREEK_SMALL_LETTER_ZETA},
        {ts::GREEK_CAPITAL_LETTER_ETA, ts::GREEK_SMALL_LETTER_ETA},
        {ts::GREEK_CAPITAL_LETTER_THETA, ts::GREEK_SMALL_LETTER_THETA},
        {ts::GREEK_CAPITAL_LETTER_IOTA, ts::GREEK_SMALL_LETTER_IOTA},
        {ts::GREEK_CAPITAL_LETTER_KAPPA, ts::GREEK_SMALL_LETTER_KAPPA},
        {ts::GREEK_CAPITAL_LETTER_LAMDA, ts::GREEK_SMALL_LETTER_LAMDA},
        {ts::GREEK_CAPITAL_LETTER_MU, ts::GREEK_SMALL_LETTER_MU},
        {ts::GREEK_CAPITAL_LETTER_NU, ts::GREEK_SMALL_LETTER_NU},
        {ts::GREEK_CAPITAL_LETTER_XI, ts::GREEK_SMALL_LETTER_XI},
        {ts::GREEK_CAPITAL_LETTER_OMICRON, ts::GREEK_SMALL_LETTER_OMICRON},
        {ts::GREEK_CAPITAL_LETTER_PI, ts::GREEK_SMALL_LETTER_PI},
        {ts::GREEK_CAPITAL_LETTER_RHO, ts::GREEK_SMALL_LETTER_RHO},
        {ts::GREEK_CAPITAL_LETTER_SIGMA, ts::GREEK_SMALL_LETTER_SIGMA},
        {ts::GREEK_CAPITAL_LETTER_TAU, ts::GREEK_SMALL_LETTER_TAU},
        {ts::GREEK_CAPITAL_LETTER_UPSILON, ts::GREEK_SMALL_LETTER_UPSILON},
        {ts::GREEK_CAPITAL_LETTER_PHI, ts::GREEK_SMALL_LETTER_PHI},
        {ts::GREEK_CAPITAL_LETTER_CHI, ts::GREEK_SMALL_LETTER_CHI},
        {ts::GREEK_CAPITAL_LETTER_PSI, ts::GREEK_SMALL_LETTER_PSI},
        {ts::GREEK_CAPITAL_LETTER_OMEGA, ts::GREEK_SMALL_LETTER_OMEGA},
        {ts::CYRILLIC_CAPITAL_LETTER_A, ts::CYRILLIC_SMALL_LETTER_A},
        {ts::CYRILLIC_CAPITAL_LETTER_BE, ts::CYRILLIC_SMALL_LETTER_BE},
        {ts::CYRILLIC_CAPITAL_LETTER_VE, ts::CYRILLIC_SMALL_LETTER_VE},
        {ts::CYRILLIC_CAPITAL_LETTER_GHE, ts::CYRILLIC_SMALL_LETTER_GHE},
        {ts::CYRILLIC_CAPITAL_LETTER_DE, ts::CYRILLIC_SMALL_LETTER_DE},
        {ts::CYRILLIC_CAPITAL_LETTER_IE, ts::CYRILLIC_SMALL_LETTER_IE},
        {ts::CYRILLIC_CAPITAL_LETTER_ZHE, ts::CYRILLIC_SMALL_LETTER_ZHE},
        {ts::CYRILLIC_CAPITAL_LETTER_ZE, ts::CYRILLIC_SMALL_LETTER_ZE},
        {ts::CYRILLIC_CAPITAL_LETTER_I, ts::CYRILLIC_SMALL_LETTER_I},
        {ts::CYRILLIC_CAPITAL_LETTER_SHORT_I, ts::CYRILLIC_SMALL_LETTER_SHORT_I},
        {ts::CYRILLIC_CAPITAL_LETTER_KA, ts::CYRILLIC_SMALL_LETTER_KA},
        {ts::CYRILLIC_CAPITAL_LETTER_EL, ts::CYRILLIC_SMALL_LETTER_EL},
        {ts::CYRILLIC_CAPITAL_LETTER_EM, ts::CYRILLIC_SMALL_LETTER_EM},
        {ts::CYRILLIC_CAPITAL_LETTER_EN, ts::CYRILLIC_SMALL_LETTER_EN},
        {ts::CYRILLIC_CAPITAL_LETTER_O, ts::CYRILLIC_SMALL_LETTER_O},
        {ts::CYRILLIC_CAPITAL_LETTER_PE, ts::CYRILLIC_SMALL_LETTER_PE},
        {ts::CYRILLIC_CAPITAL_LETTER_ER, ts::CYRILLIC_SMALL_LETTER_ER},
        {ts::CYRILLIC_CAPITAL_LETTER_ES, ts::CYRILLIC_SMALL_LETTER_ES},
        {ts::CYRILLIC_CAPITAL_LETTER_TE, ts::CYRILLIC_SMALL_LETTER_TE},
        {ts::CYRILLIC_CAPITAL_LETTER_U, ts::CYRILLIC_SMALL_LETTER_U},
        {ts::CYRILLIC_CAPITAL_LETTER_EF, ts::CYRILLIC_SMALL_LETTER_EF},
        {ts::CYRILLIC_CAPITAL_LETTER_HA, ts::CYRILLIC_SMALL_LETTER_HA},
        {ts::CYRILLIC_CAPITAL_LETTER_TSE, ts::CYRILLIC_SMALL_LETTER_TSE},
        {ts::CYRILLIC_CAPITAL_LETTER_CHE, ts::CYRILLIC_SMALL_LETTER_CHE},
        {ts::CYRILLIC_CAPITAL_LETTER_SHA, ts::CYRILLIC_SMALL_LETTER_SHA},
        {ts::CYRILLIC_CAPITAL_LETTER_SHCHA, ts::CYRILLIC_SMALL_LETTER_SHCHA},
        {ts::CYRILLIC_CAPITAL_LETTER_HARD_SIGN, ts::CYRILLIC_SMALL_LETTER_HARD_SIGN},
        {ts::CYRILLIC_CAPITAL_LETTER_YERU, ts::CYRILLIC_SMALL_LETTER_YERU},
        {ts::CYRILLIC_CAPITAL_LETTER_SOFT_SIGN, ts::CYRILLIC_SMALL_LETTER_SOFT_SIGN},
        {ts::CYRILLIC_CAPITAL_LETTER_E, ts::CYRILLIC_SMALL_LETTER_E},
        {ts::CYRILLIC_CAPITAL_LETTER_YU, ts::CYRILLIC_SMALL_LETTER_YU},
        {ts::CYRILLIC_CAPITAL_LETTER_YA, ts::CYRILLIC_SMALL_LETTER_YA},
        {ts::LATIN_CAPITAL_LETTER_B_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_B_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_D_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_F_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_F_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_M_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_M_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_P_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_P_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_S_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_DOT_ABOVE, ts::LATIN_SMALL_LETTER_T_WITH_DOT_ABOVE},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_GRAVE, ts::LATIN_SMALL_LETTER_W_WITH_GRAVE},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_ACUTE, ts::LATIN_SMALL_LETTER_W_WITH_ACUTE},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_W_WITH_DIAERESIS},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_GRAVE, ts::LATIN_SMALL_LETTER_Y_WITH_GRAVE},
    };

    const UpperLower UpperLower2[] = {
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_Y_WITH_DIAERESIS},
        {ts::GREEK_CAPITAL_LETTER_OMICRON_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMICRON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_TONOS, ts::GREEK_SMALL_LETTER_UPSILON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_OMEGA_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMEGA_WITH_TONOS},
        {ts::CYRILLIC_CAPITAL_LETTER_IO, ts::CYRILLIC_SMALL_LETTER_IO},
        {ts::CYRILLIC_CAPITAL_LETTER_DJE, ts::CYRILLIC_SMALL_LETTER_DJE},
        {ts::CYRILLIC_CAPITAL_LETTER_GJE, ts::CYRILLIC_SMALL_LETTER_GJE},
        {ts::CYRILLIC_CAPITAL_LETTER_UKRAINIAN_IE, ts::CYRILLIC_SMALL_LETTER_UKRAINIAN_IE},
        {ts::CYRILLIC_CAPITAL_LETTER_DZE, ts::CYRILLIC_SMALL_LETTER_DZE},
        {ts::CYRILLIC_CAPITAL_LETTER_BYELORUSSIAN_UKRAINIAN_I, ts::CYRILLIC_SMALL_LETTER_BYELORUSSIAN_UKRAINIAN_I},
        {ts::CYRILLIC_CAPITAL_LETTER_YI, ts::CYRILLIC_SMALL_LETTER_YI},
        {ts::CYRILLIC_CAPITAL_LETTER_JE, ts::CYRILLIC_SMALL_LETTER_JE},
        {ts::CYRILLIC_CAPITAL_LETTER_LJE, ts::CYRILLIC_SMALL_LETTER_LJE},
        {ts::CYRILLIC_CAPITAL_LETTER_NJE, ts::CYRILLIC_SMALL_LETTER_NJE},
        {ts::CYRILLIC_CAPITAL_LETTER_TSHE, ts::CYRILLIC_SMALL_LETTER_TSHE},
        {ts::CYRILLIC_CAPITAL_LETTER_KJE, ts::CYRILLIC_SMALL_LETTER_KJE},
        {ts::CYRILLIC_CAPITAL_LETTER_SHORT_U, ts::CYRILLIC_SMALL_LETTER_SHORT_U},
        {ts::CYRILLIC_CAPITAL_LETTER_DZHE, ts::CYRILLIC_SMALL_LETTER_DZHE},
    };

    const UpperLower UpperLower3[] = {
        {ts::GREEK_CAPITAL_LETTER_IOTA_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA},
    };

    const UpperLowerTable AllUpperLower[] = {
        {UpperLower1, ElementCount(UpperLower1)},
        {UpperLower2, ElementCount(UpperLower2)},
        {UpperLower3, ElementCount(UpperLower3)},
    };

    //
    // Search a translation based on either upper or lower value.
    //
    const UpperLower* SearchUpperLower(ts::Char key, UpperLowerField field)
    {
        // Loop on all tables.
        for (size_t ti = 0; ti < ElementCount(AllUpperLower); ++ti) {
            // Try a dichotomic search on current table.
            assert(AllUpperLower[ti].size > 0);
            const UpperLower* const tab = AllUpperLower[ti].table;
            size_t first = 0;
            size_t last = AllUpperLower[ti].size - 1;
            if (key >= tab[first].*field && key <= tab[last].*field) {
                do {
                    const size_t mid = first + (last - first) / 2;
                    if (tab[mid].*field == key) {
                        return &tab[mid];
                    }
                    else if (tab[mid].*field > key) {
                        last = mid == 0 ? 0 : mid - 1;
                    }
                    else {
                        first = mid + 1;
                    }
                } while (last > first);
                // Last chance: last == first.
                if (last == first && tab[first].*field == key) {
                    return &tab[first];
                }
            }
        }
        // No translation found.
        return 0;
    }
}


//----------------------------------------------------------------------------
// Char internal self-test procedure.
//----------------------------------------------------------------------------

bool ts::CharSelfTest()
{
    // Check that UpperLowerTable is correctly sorted.
    for (size_t ti = 0; ti < ElementCount(AllUpperLower); ++ti) {
        const UpperLower* const tab = AllUpperLower[ti].table;
        const size_t len = AllUpperLower[ti].size;
        for (size_t i = 1; i < len; ++i) {
            if (tab[i-1].lower >= tab[i].lower || tab[i-1].upper >= tab[i].upper) {
                return false;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Character case conversions.
//----------------------------------------------------------------------------

bool ts::IsLower(Char c)
{
    // If the standard function says not lower, check if it is a known lowercase for us.
    return std::iswlower(wint_t(c)) != 0 || SearchUpperLower(c, &UpperLower::lower) != 0;
}

bool ts::IsUpper(Char c)
{
    // If the standard function says not upper, check if it is a known uppercase for us.
    return std::iswupper(wint_t(c)) != 0 || SearchUpperLower(c, &UpperLower::upper) != 0;
}

ts::Char ts::ToLower(Char c)
{
    const Char result = Char(std::towlower(wint_t(c)));
    if (result != c) {
        // The standard function has found a translation.
        return result;
    }
    else {
        // Search for an additional translation, if any.
        const UpperLower* trans = SearchUpperLower(c, &UpperLower::upper);
        return trans == 0 ? c : trans->lower;
    }
}

ts::Char ts::ToUpper(Char c)
{
    const Char result = Char(std::towupper(wint_t(c)));
    if (result != c) {
        // The standard function has found a translation.
        return result;
    }
    else {
        // Search for an additional translation, if any.
        const UpperLower* trans = SearchUpperLower(c, &UpperLower::lower);
        return trans == 0 ? c : trans->upper;
    }
}
