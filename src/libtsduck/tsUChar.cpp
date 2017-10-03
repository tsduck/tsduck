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

#include "tsUChar.h"
#include "tsUString.h"
#include "tsSingletonManager.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// The macro MAP_SINGLETON defines a singleton class which inherits
// from std::map<key_type, value_type>.
// The constructor needs to be separately defined.
//----------------------------------------------------------------------------

#define MAP_SINGLETON(classname, key_type, value_type)      \
    class classname : public std::map<key_type, value_type> \
    {                                                       \
        tsDeclareSingleton(classname);                      \
        typedef std::map<key_type, value_type> SuperClass;  \
    };                                                      \
    tsDefineSingleton(classname)


//----------------------------------------------------------------------------
// Map uppercase => lowercase.
//----------------------------------------------------------------------------

namespace {
    MAP_SINGLETON(UpperLower, ts::UChar, ts::UChar);
    UpperLower::UpperLower() : SuperClass({
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
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS, ts::LATIN_SMALL_LETTER_Y_WITH_DIAERESIS},
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
        {ts::GREEK_CAPITAL_LETTER_OMICRON_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMICRON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_TONOS, ts::GREEK_SMALL_LETTER_UPSILON_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_OMEGA_WITH_TONOS, ts::GREEK_SMALL_LETTER_OMEGA_WITH_TONOS},
        {ts::GREEK_CAPITAL_LETTER_IOTA_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA},
        {ts::GREEK_CAPITAL_LETTER_UPSILON_WITH_DIALYTIKA, ts::GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA},
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
    }) {}
}


//----------------------------------------------------------------------------
// Map lowercase => uppercase.
//----------------------------------------------------------------------------

namespace {
    MAP_SINGLETON(LowerUpper, ts::UChar, ts::UChar);
    LowerUpper::LowerUpper() : SuperClass()
    {
        // Build inversed table from UpperLower.
        const UpperLower* ul = UpperLower::Instance();
        for (UpperLower::const_iterator it = ul->begin(); it != ul->end(); ++it) {
            insert(std::pair<ts::UChar, ts::UChar>(it->second, it->first));
        }
    }
}


//----------------------------------------------------------------------------
// Map accented letter => without accent sequence.
//----------------------------------------------------------------------------

namespace {
    MAP_SINGLETON(WithoutAccent, ts::UChar, const char*);
    WithoutAccent::WithoutAccent() : SuperClass({
        {ts::LATIN_CAPITAL_LETTER_A_WITH_GRAVE,        "A"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_ACUTE,        "A"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX,   "A"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_TILDE,        "A"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS,    "A"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE,   "A"},
        {ts::LATIN_CAPITAL_LETTER_AE,                  "AE"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CEDILLA,      "C"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_GRAVE,        "E"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_ACUTE,        "E"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX,   "E"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS,    "E"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_GRAVE,        "I"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_ACUTE,        "I"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX,   "I"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS,    "I"},
        {ts::LATIN_CAPITAL_LETTER_ETH,                 "E"},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_TILDE,        "N"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_GRAVE,        "O"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_ACUTE,        "O"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX,   "O"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_TILDE,        "O"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS,    "O"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_STROKE,       "O"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_GRAVE,        "U"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_ACUTE,        "U"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX,   "U"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS,    "U"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_ACUTE,        "Y"},
        {ts::LATIN_CAPITAL_LETTER_THORN,               "T"},
        {ts::LATIN_SMALL_LETTER_SHARP_S,               "ss"},
        {ts::LATIN_SMALL_LETTER_A_WITH_GRAVE,          "a"},
        {ts::LATIN_SMALL_LETTER_A_WITH_ACUTE,          "a"},
        {ts::LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX,     "a"},
        {ts::LATIN_SMALL_LETTER_A_WITH_TILDE,          "a"},
        {ts::LATIN_SMALL_LETTER_A_WITH_DIAERESIS,      "a"},
        {ts::LATIN_SMALL_LETTER_A_WITH_RING_ABOVE,     "a"},
        {ts::LATIN_SMALL_LETTER_AE,                    "ae"},
        {ts::LATIN_SMALL_LETTER_C_WITH_CEDILLA,        "c"},
        {ts::LATIN_SMALL_LETTER_E_WITH_GRAVE,          "e"},
        {ts::LATIN_SMALL_LETTER_E_WITH_ACUTE,          "e"},
        {ts::LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX,     "e"},
        {ts::LATIN_SMALL_LETTER_E_WITH_DIAERESIS,      "e"},
        {ts::LATIN_SMALL_LETTER_I_WITH_GRAVE,          "i"},
        {ts::LATIN_SMALL_LETTER_I_WITH_ACUTE,          "i"},
        {ts::LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX,     "i"},
        {ts::LATIN_SMALL_LETTER_I_WITH_DIAERESIS,      "i"},
        {ts::LATIN_SMALL_LETTER_ETH,                   "e"},
        {ts::LATIN_SMALL_LETTER_N_WITH_TILDE,          "n"},
        {ts::LATIN_SMALL_LETTER_O_WITH_GRAVE,          "o"},
        {ts::LATIN_SMALL_LETTER_O_WITH_ACUTE,          "o"},
        {ts::LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX,     "o"},
        {ts::LATIN_SMALL_LETTER_O_WITH_TILDE,          "o"},
        {ts::LATIN_SMALL_LETTER_O_WITH_DIAERESIS,      "o"},
        {ts::LATIN_SMALL_LETTER_O_WITH_STROKE,         "o"},
        {ts::LATIN_SMALL_LETTER_U_WITH_GRAVE,          "u"},
        {ts::LATIN_SMALL_LETTER_U_WITH_ACUTE,          "u"},
        {ts::LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX,     "u"},
        {ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS,      "u"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_ACUTE,          "y"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_DIAERESIS,      "y"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_MACRON,       "A"},
        {ts::LATIN_SMALL_LETTER_A_WITH_MACRON,         "a"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_BREVE,        "A"},
        {ts::LATIN_SMALL_LETTER_A_WITH_BREVE,          "a"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_OGONEK,       "A"},
        {ts::LATIN_SMALL_LETTER_A_WITH_OGONEK,         "a"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_ACUTE,        "C"},
        {ts::LATIN_SMALL_LETTER_C_WITH_ACUTE,          "c"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CIRCUMFLEX,   "C"},
        {ts::LATIN_SMALL_LETTER_C_WITH_CIRCUMFLEX,     "c"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_DOT_ABOVE,    "C"},
        {ts::LATIN_SMALL_LETTER_C_WITH_DOT_ABOVE,      "c"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CARON,        "C"},
        {ts::LATIN_SMALL_LETTER_C_WITH_CARON,          "c"},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_CARON,        "D"},
        {ts::LATIN_SMALL_LETTER_D_WITH_CARON,          "d"},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_STROKE,       "D"},
        {ts::LATIN_SMALL_LETTER_D_WITH_STROKE,         "d"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_MACRON,       "E"},
        {ts::LATIN_SMALL_LETTER_E_WITH_MACRON,         "e"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_DOT_ABOVE,    "E"},
        {ts::LATIN_SMALL_LETTER_E_WITH_DOT_ABOVE,      "e"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_OGONEK,       "E"},
        {ts::LATIN_SMALL_LETTER_E_WITH_OGONEK,         "e"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_CARON,        "E"},
        {ts::LATIN_SMALL_LETTER_E_WITH_CARON,          "e"},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_CIRCUMFLEX,   "G"},
        {ts::LATIN_SMALL_LETTER_G_WITH_CIRCUMFLEX,     "g"},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_BREVE,        "G"},
        {ts::LATIN_SMALL_LETTER_G_WITH_BREVE,          "g"},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_DOT_ABOVE,    "G"},
        {ts::LATIN_SMALL_LETTER_G_WITH_DOT_ABOVE,      "g"},
        {ts::LATIN_CAPITAL_LETTER_G_WITH_CEDILLA,      "G"},
        {ts::LATIN_SMALL_LETTER_G_WITH_CEDILLA,        "g"},
        {ts::LATIN_CAPITAL_LETTER_H_WITH_CIRCUMFLEX,   "H"},
        {ts::LATIN_SMALL_LETTER_H_WITH_CIRCUMFLEX,     "h"},
        {ts::LATIN_CAPITAL_LETTER_H_WITH_STROKE,       "H"},
        {ts::LATIN_SMALL_LETTER_H_WITH_STROKE,         "h"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_TILDE,        "I"},
        {ts::LATIN_SMALL_LETTER_I_WITH_TILDE,          "i"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_MACRON,       "I"},
        {ts::LATIN_SMALL_LETTER_I_WITH_MACRON,         "i"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_OGONEK,       "I"},
        {ts::LATIN_SMALL_LETTER_I_WITH_OGONEK,         "i"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE,    "I"},
        {ts::LATIN_CAPITAL_LETTER_J_WITH_CIRCUMFLEX,   "J"},
        {ts::LATIN_SMALL_LETTER_J_WITH_CIRCUMFLEX,     "j"},
        {ts::LATIN_CAPITAL_LETTER_K_WITH_CEDILLA,      "K"},
        {ts::LATIN_SMALL_LETTER_K_WITH_CEDILLA,        "k"},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_ACUTE,        "L"},
        {ts::LATIN_SMALL_LETTER_L_WITH_ACUTE,          "l"},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_CEDILLA,      "L"},
        {ts::LATIN_SMALL_LETTER_L_WITH_CEDILLA,        "l"},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_CARON,        "L"},
        {ts::LATIN_SMALL_LETTER_L_WITH_CARON,          "l"},
        {ts::LATIN_CAPITAL_LETTER_L_WITH_STROKE,       "L"},
        {ts::LATIN_SMALL_LETTER_L_WITH_STROKE,         "l"},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_ACUTE,        "N"},
        {ts::LATIN_SMALL_LETTER_N_WITH_ACUTE,          "n"},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_CEDILLA,      "N"},
        {ts::LATIN_SMALL_LETTER_N_WITH_CEDILLA,        "n"},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_CARON,        "N"},
        {ts::LATIN_SMALL_LETTER_N_WITH_CARON,          "n"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_MACRON,       "O"},
        {ts::LATIN_SMALL_LETTER_O_WITH_MACRON,         "o"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_DOUBLE_ACUTE, "O"},
        {ts::LATIN_SMALL_LETTER_O_WITH_DOUBLE_ACUTE,   "o"},
        {ts::LATIN_CAPITAL_LIGATURE_OE,                "OE"},
        {ts::LATIN_SMALL_LIGATURE_OE,                  "oe"},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_ACUTE,        "R"},
        {ts::LATIN_SMALL_LETTER_R_WITH_ACUTE,          "r"},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_CEDILLA,      "R"},
        {ts::LATIN_SMALL_LETTER_R_WITH_CEDILLA,        "r"},
        {ts::LATIN_CAPITAL_LETTER_R_WITH_CARON,        "R"},
        {ts::LATIN_SMALL_LETTER_R_WITH_CARON,          "r"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_ACUTE,        "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_ACUTE,          "s"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CIRCUMFLEX,   "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_CIRCUMFLEX,     "s"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CEDILLA,      "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_CEDILLA,        "s"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CARON,        "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_CARON,          "s"},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_CEDILLA,      "T"},
        {ts::LATIN_SMALL_LETTER_T_WITH_CEDILLA,        "t"},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_CARON,        "T"},
        {ts::LATIN_SMALL_LETTER_T_WITH_CARON,          "t"},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_STROKE,       "T"},
        {ts::LATIN_SMALL_LETTER_T_WITH_STROKE,         "t"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_TILDE,        "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_TILDE,          "u"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_MACRON,       "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_MACRON,         "u"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_BREVE,        "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_BREVE,          "u"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_RING_ABOVE,   "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_RING_ABOVE,     "u"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_DOUBLE_ACUTE, "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_DOUBLE_ACUTE,   "u"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_OGONEK,       "U"},
        {ts::LATIN_SMALL_LETTER_U_WITH_OGONEK,         "u"},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_CIRCUMFLEX,   "W"},
        {ts::LATIN_SMALL_LETTER_W_WITH_CIRCUMFLEX,     "w"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_CIRCUMFLEX,   "Y"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_CIRCUMFLEX,     "y"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS,    "Y"},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_ACUTE,        "Z"},
        {ts::LATIN_SMALL_LETTER_Z_WITH_ACUTE,          "z"},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_DOT_ABOVE,    "Z"},
        {ts::LATIN_SMALL_LETTER_Z_WITH_DOT_ABOVE,      "z"},
        {ts::LATIN_CAPITAL_LETTER_Z_WITH_CARON,        "Z"},
        {ts::LATIN_SMALL_LETTER_Z_WITH_CARON,          "z"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_COMMA_BELOW,  "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_COMMA_BELOW,    "s"},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_COMMA_BELOW,  "T"},
        {ts::LATIN_SMALL_LETTER_T_WITH_COMMA_BELOW,    "t"},
        {ts::LATIN_CAPITAL_LETTER_B_WITH_DOT_ABOVE,    "B"},
        {ts::LATIN_SMALL_LETTER_B_WITH_DOT_ABOVE,      "b"},
        {ts::LATIN_CAPITAL_LETTER_D_WITH_DOT_ABOVE,    "D"},
        {ts::LATIN_SMALL_LETTER_D_WITH_DOT_ABOVE,      "d"},
        {ts::LATIN_CAPITAL_LETTER_F_WITH_DOT_ABOVE,    "F"},
        {ts::LATIN_SMALL_LETTER_F_WITH_DOT_ABOVE,      "f"},
        {ts::LATIN_CAPITAL_LETTER_M_WITH_DOT_ABOVE,    "M"},
        {ts::LATIN_SMALL_LETTER_M_WITH_DOT_ABOVE,      "m"},
        {ts::LATIN_CAPITAL_LETTER_P_WITH_DOT_ABOVE,    "P"},
        {ts::LATIN_SMALL_LETTER_P_WITH_DOT_ABOVE,      "p"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_DOT_ABOVE,    "S"},
        {ts::LATIN_SMALL_LETTER_S_WITH_DOT_ABOVE,      "s"},
        {ts::LATIN_CAPITAL_LETTER_T_WITH_DOT_ABOVE,    "T"},
        {ts::LATIN_SMALL_LETTER_T_WITH_DOT_ABOVE,      "t"},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_GRAVE,        "W"},
        {ts::LATIN_SMALL_LETTER_W_WITH_GRAVE,          "w"},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_ACUTE,        "W"},
        {ts::LATIN_SMALL_LETTER_W_WITH_ACUTE,          "w"},
        {ts::LATIN_CAPITAL_LETTER_W_WITH_DIAERESIS,    "W"},
        {ts::LATIN_SMALL_LETTER_W_WITH_DIAERESIS,      "w"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_GRAVE,        "Y"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_GRAVE,          "y"},
        {ts::LATIN_SMALL_F_WITH_HOOK,                  "f"},
        {ts::BLACKLETTER_CAPITAL_I,                    "I"},
        {ts::SCRIPT_CAPITAL_P,                         "P"},
        {ts::BLACKLETTER_CAPITAL_R,                    "R"},
    }) {}
}


//----------------------------------------------------------------------------
// Map character => html entity.
// See http://www.w3.org/TR/html4/sgml/entities.html
//----------------------------------------------------------------------------

namespace {
    MAP_SINGLETON(HTMLEntities, ts::UChar, const char*);
    HTMLEntities::HTMLEntities() : SuperClass({
        {ts::QUOTATION_MARK, "quot"},
        {ts::AMPERSAND, "amp"},
        {ts::LESS_THAN_SIGN, "lt"},
        {ts::GREATER_THAN_SIGN, "gt"},
        {ts::NO_BREAK_SPACE, "nbsp"},
        {ts::INVERTED_EXCLAMATION_MARK, "iexcl"},
        {ts::CENT_SIGN, "cent"},
        {ts::POUND_SIGN, "pound"},
        {ts::CURRENCY_SIGN, "curren"},
        {ts::YEN_SIGN, "yen"},
        {ts::BROKEN_BAR, "brvbar"},
        {ts::SECTION_SIGN, "sect"},
        {ts::DIAERESIS, "uml"},
        {ts::COPYRIGHT_SIGN, "copy"},
        {ts::FEMININE_ORDINAL_INDICATOR, "ordf"},
        {ts::LEFT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK, "laquo"},
        {ts::NOT_SIGN, "not"},
        {ts::SOFT_HYPHEN, "shy"},
        {ts::REGISTERED_SIGN, "reg"},
        {ts::MACRON, "macr"},
        {ts::DEGREE_SIGN, "deg"},
        {ts::PLUS_MINUS_SIGN, "plusmn"},
        {ts::SUPERSCRIPT_TWO, "sup2"},
        {ts::SUPERSCRIPT_THREE, "sup3"},
        {ts::ACUTE_ACCENT, "acute"},
        {ts::MICRO_SIGN, "micro"},
        {ts::PILCROW_SIGN, "para"},
        {ts::MIDDLE_DOT, "middot"},
        {ts::CEDILLA, "cedil"},
        {ts::SUPERSCRIPT_ONE, "sup1"},
        {ts::MASCULINE_ORDINAL_INDICATOR, "ordm"},
        {ts::RIGHT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK, "raquo"},
        {ts::VULGAR_FRACTION_ONE_QUARTER, "frac14"},
        {ts::VULGAR_FRACTION_ONE_HALF, "frac12"},
        {ts::VULGAR_FRACTION_THREE_QUARTERS, "frac34"},
        {ts::INVERTED_QUESTION_MARK, "iquest"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_GRAVE, "Agrave"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_ACUTE, "Aacute"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX, "Acirc"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_TILDE, "Atilde"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS, "Auml"},
        {ts::LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE, "Aring"},
        {ts::LATIN_CAPITAL_LETTER_AE, "AElig"},
        {ts::LATIN_CAPITAL_LETTER_C_WITH_CEDILLA, "Ccedil"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_GRAVE, "Egrave"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_ACUTE, "Eacute"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX, "Ecirc"},
        {ts::LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS, "Euml"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_GRAVE, "Igrave"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_ACUTE, "Iacute"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX, "Icirc"},
        {ts::LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS, "Iuml"},
        {ts::LATIN_CAPITAL_LETTER_ETH, "ETH"},
        {ts::LATIN_CAPITAL_LETTER_N_WITH_TILDE, "Ntilde"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_GRAVE, "Ograve"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_ACUTE, "Oacute"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX, "Ocirc"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_TILDE, "Otilde"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS, "Ouml"},
        {ts::MULTIPLICATION_SIGN, "times"},
        {ts::LATIN_CAPITAL_LETTER_O_WITH_STROKE, "Oslash"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_GRAVE, "Ugrave"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_ACUTE, "Uacute"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX, "Ucirc"},
        {ts::LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS, "Uuml"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_ACUTE, "Yacute"},
        {ts::LATIN_CAPITAL_LETTER_THORN, "THORN"},
        {ts::LATIN_SMALL_LETTER_SHARP_S, "szlig"},
        {ts::LATIN_SMALL_LETTER_A_WITH_GRAVE, "agrave"},
        {ts::LATIN_SMALL_LETTER_A_WITH_ACUTE, "aacute"},
        {ts::LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX, "acirc"},
        {ts::LATIN_SMALL_LETTER_A_WITH_TILDE, "atilde"},
        {ts::LATIN_SMALL_LETTER_A_WITH_DIAERESIS, "auml"},
        {ts::LATIN_SMALL_LETTER_A_WITH_RING_ABOVE, "aring"},
        {ts::LATIN_SMALL_LETTER_AE, "aelig"},
        {ts::LATIN_SMALL_LETTER_C_WITH_CEDILLA, "ccedil"},
        {ts::LATIN_SMALL_LETTER_E_WITH_GRAVE, "egrave"},
        {ts::LATIN_SMALL_LETTER_E_WITH_ACUTE, "eacute"},
        {ts::LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX, "ecirc"},
        {ts::LATIN_SMALL_LETTER_E_WITH_DIAERESIS, "euml"},
        {ts::LATIN_SMALL_LETTER_I_WITH_GRAVE, "igrave"},
        {ts::LATIN_SMALL_LETTER_I_WITH_ACUTE, "iacute"},
        {ts::LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX, "icirc"},
        {ts::LATIN_SMALL_LETTER_I_WITH_DIAERESIS, "iuml"},
        {ts::LATIN_SMALL_LETTER_ETH, "eth"},
        {ts::LATIN_SMALL_LETTER_N_WITH_TILDE, "ntilde"},
        {ts::LATIN_SMALL_LETTER_O_WITH_GRAVE, "ograve"},
        {ts::LATIN_SMALL_LETTER_O_WITH_ACUTE, "oacute"},
        {ts::LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX, "ocirc"},
        {ts::LATIN_SMALL_LETTER_O_WITH_TILDE, "otilde"},
        {ts::LATIN_SMALL_LETTER_O_WITH_DIAERESIS, "ouml"},
        {ts::DIVISION_SIGN, "divide"},
        {ts::LATIN_SMALL_LETTER_O_WITH_STROKE, "oslash"},
        {ts::LATIN_SMALL_LETTER_U_WITH_GRAVE, "ugrave"},
        {ts::LATIN_SMALL_LETTER_U_WITH_ACUTE, "uacute"},
        {ts::LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX, "ucirc"},
        {ts::LATIN_SMALL_LETTER_U_WITH_DIAERESIS, "uuml"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_ACUTE, "yacute"},
        {ts::LATIN_SMALL_LETTER_THORN, "thorn"},
        {ts::LATIN_SMALL_LETTER_Y_WITH_DIAERESIS, "yuml"},
        {ts::LATIN_CAPITAL_LIGATURE_OE, "OElig"},
        {ts::LATIN_SMALL_LIGATURE_OE, "oelig"},
        {ts::LATIN_CAPITAL_LETTER_S_WITH_CARON, "Scaron"},
        {ts::LATIN_SMALL_LETTER_S_WITH_CARON, "scaron"},
        {ts::LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS, "Yuml"},
        {ts::LATIN_SMALL_F_WITH_HOOK, "fnof"},
        {ts::MODIFIER_LETTER_CIRCUMFLEX_ACCENT, "circ"},
        {ts::SMALL_TILDE, "tilde"},
        {ts::GREEK_CAPITAL_LETTER_ALPHA, "Alpha"},
        {ts::GREEK_CAPITAL_LETTER_BETA, "Beta"},
        {ts::GREEK_CAPITAL_LETTER_GAMMA, "Gamma"},
        {ts::GREEK_CAPITAL_LETTER_DELTA, "Delta"},
        {ts::GREEK_CAPITAL_LETTER_EPSILON, "Epsilon"},
        {ts::GREEK_CAPITAL_LETTER_ZETA, "Zeta"},
        {ts::GREEK_CAPITAL_LETTER_ETA, "Eta"},
        {ts::GREEK_CAPITAL_LETTER_THETA, "Theta"},
        {ts::GREEK_CAPITAL_LETTER_IOTA, "Iota"},
        {ts::GREEK_CAPITAL_LETTER_KAPPA, "Kappa"},
        {ts::GREEK_CAPITAL_LETTER_LAMDA, "Lambda"},
        {ts::GREEK_CAPITAL_LETTER_MU, "Mu"},
        {ts::GREEK_CAPITAL_LETTER_NU, "Nu"},
        {ts::GREEK_CAPITAL_LETTER_XI, "Xi"},
        {ts::GREEK_CAPITAL_LETTER_OMICRON, "Omicron"},
        {ts::GREEK_CAPITAL_LETTER_PI, "Pi"},
        {ts::GREEK_CAPITAL_LETTER_RHO, "Rho"},
        {ts::GREEK_CAPITAL_LETTER_SIGMA, "Sigma"},
        {ts::GREEK_CAPITAL_LETTER_TAU, "Tau"},
        {ts::GREEK_CAPITAL_LETTER_UPSILON, "Upsilon"},
        {ts::GREEK_CAPITAL_LETTER_PHI, "Phi"},
        {ts::GREEK_CAPITAL_LETTER_CHI, "Chi"},
        {ts::GREEK_CAPITAL_LETTER_PSI, "Psi"},
        {ts::GREEK_CAPITAL_LETTER_OMEGA, "Omega"},
        {ts::GREEK_SMALL_LETTER_ALPHA, "alpha"},
        {ts::GREEK_SMALL_LETTER_BETA, "beta"},
        {ts::GREEK_SMALL_LETTER_GAMMA, "gamma"},
        {ts::GREEK_SMALL_LETTER_DELTA, "delta"},
        {ts::GREEK_SMALL_LETTER_EPSILON, "epsilon"},
        {ts::GREEK_SMALL_LETTER_ZETA, "zeta"},
        {ts::GREEK_SMALL_LETTER_ETA, "eta"},
        {ts::GREEK_SMALL_LETTER_THETA, "theta"},
        {ts::GREEK_SMALL_LETTER_IOTA, "iota"},
        {ts::GREEK_SMALL_LETTER_KAPPA, "kappa"},
        {ts::GREEK_SMALL_LETTER_LAMDA, "lambda"},
        {ts::GREEK_SMALL_LETTER_MU, "mu"},
        {ts::GREEK_SMALL_LETTER_NU, "nu"},
        {ts::GREEK_SMALL_LETTER_XI, "xi"},
        {ts::GREEK_SMALL_LETTER_OMICRON, "omicron"},
        {ts::GREEK_SMALL_LETTER_PI, "pi"},
        {ts::GREEK_SMALL_LETTER_RHO, "rho"},
        {ts::GREEK_SMALL_LETTER_FINAL_SIGMA, "sigmaf"},
        {ts::GREEK_SMALL_LETTER_SIGMA, "sigma"},
        {ts::GREEK_SMALL_LETTER_TAU, "tau"},
        {ts::GREEK_SMALL_LETTER_UPSILON, "upsilon"},
        {ts::GREEK_SMALL_LETTER_PHI, "phi"},
        {ts::GREEK_SMALL_LETTER_CHI, "chi"},
        {ts::GREEK_SMALL_LETTER_PSI, "psi"},
        {ts::GREEK_SMALL_LETTER_OMEGA, "omega"},
        {ts::GREEK_SMALL_LETTER_THETA_SYMBOL, "thetasym"},
        {ts::GREEK_UPSILON_WITH_HOOK_SYMBOL, "upsih"},
        {ts::GREEK_PI_SYMBOL, "piv"},
        {ts::EN_SPACE, "ensp"},
        {ts::EM_SPACE, "emsp"},
        {ts::THIN_SPACE, "thinsp"},
        {ts::ZERO_WIDTH_NON_JOINER, "zwnj"},
        {ts::ZERO_WIDTH_JOINER, "zwj"},
        {ts::LEFT_TO_RIGHT_MARK, "lrm"},
        {ts::RIGHT_TO_LEFT_MARK, "rlm"},
        {ts::EN_DASH, "ndash"},
        {ts::EM_DASH, "mdash"},
        {ts::LEFT_SINGLE_QUOTATION_MARK, "lsquo"},
        {ts::RIGHT_SINGLE_QUOTATION_MARK, "rsquo"},
        {ts::SINGLE_LOW_9_QUOTATION_MARK, "sbquo"},
        {ts::LEFT_DOUBLE_QUOTATION_MARK, "ldquo"},
        {ts::RIGHT_DOUBLE_QUOTATION_MARK, "rdquo"},
        {ts::DOUBLE_LOW_9_QUOTATION_MARK, "bdquo"},
        {ts::DAGGER, "dagger"},
        {ts::DOUBLE_DAGGER, "Dagger"},
        {ts::BULLET, "bull"},
        {ts::HORIZONTAL_ELLIPSIS, "hellip"},
        {ts::PER_MILLE_SIGN, "permil"},
        {ts::PRIME, "prime"},
        {ts::DOUBLE_PRIME, "Prime"},
        {ts::SINGLE_LEFT_POINTING_ANGLE_QUOTATION_MARK, "lsaquo"},
        {ts::SINGLE_RIGHT_POINTING_ANGLE_QUOTATION_MARK, "rsaquo"},
        {ts::OVERLINE, "oline"},
        {ts::FRACTION_SLASH, "frasl"},
        {ts::EURO_SIGN, "euro"},
        {ts::BLACKLETTER_CAPITAL_I, "image"},
        {ts::SCRIPT_CAPITAL_P, "weierp"},
        {ts::BLACKLETTER_CAPITAL_R, "real"},
        {ts::TRADE_MARK_SIGN, "trade"},
        {ts::ALEF_SYMBOL, "alefsym"},
        {ts::LEFTWARDS_ARROW, "larr"},
        {ts::UPWARDS_ARROW, "uarr"},
        {ts::RIGHTWARDS_ARROW, "rarr"},
        {ts::DOWNWARDS_ARROW, "darr"},
        {ts::LEFT_RIGHT_ARROW, "harr"},
        {ts::DOWNWARDS_ARROW_WITH_CORNER_LEFTWARDS, "crarr"},
        {ts::LEFTWARDS_DOUBLE_ARROW, "lArr"},
        {ts::UPWARDS_DOUBLE_ARROW, "uArr"},
        {ts::RIGHTWARDS_DOUBLE_ARROW, "rArr"},
        {ts::DOWNWARDS_DOUBLE_ARROW, "dArr"},
        {ts::LEFT_RIGHT_DOUBLE_ARROW, "hArr"},
        {ts::FOR_ALL, "forall"},
        {ts::PARTIAL_DIFFERENTIAL, "part"},
        {ts::THERE_EXISTS, "exist"},
        {ts::EMPTY_SET, "empty"},
        {ts::NABLA, "nabla"},
        {ts::ELEMENT_OF, "isin"},
        {ts::NOT_AN_ELEMENT_OF, "notin"},
        {ts::CONTAINS_AS_MEMBER, "ni"},
        {ts::N_ARY_PRODUCT, "prod"},
        {ts::N_ARY_SUMATION, "sum"},
        {ts::MINUS_SIGN, "minus"},
        {ts::ASTERISK_OPERATOR, "lowast"},
        {ts::SQUARE_ROOT, "radic"},
        {ts::PROPORTIONAL_TO, "prop"},
        {ts::CHAR_INFINITY, "infin"},
        {ts::ANGLE, "ang"},
        {ts::LOGICAL_AND, "and"},
        {ts::LOGICAL_OR, "or"},
        {ts::INTERSECTION, "cap"},
        {ts::UNION, "cup"},
        {ts::INTEGRAL, "int"},
        {ts::THEREFORE, "there4"},
        {ts::TILDE_OPERATOR, "sim"},
        {ts::APPROXIMATELY_EQUAL_TO, "cong"},
        {ts::ALMOST_EQUAL_TO, "asymp"},
        {ts::NOT_EQUAL_TO, "ne"},
        {ts::IDENTICAL_TO, "equiv"},
        {ts::LESS_THAN_OR_EQUAL_TO, "le"},
        {ts::GREATER_THAN_OR_EQUAL_TO, "ge"},
        {ts::SUBSET_OF, "sub"},
        {ts::SUPERSET_OF, "sup"},
        {ts::NOT_A_SUBSET_OF, "nsub"},
        {ts::SUBSET_OF_OR_EQUAL_TO, "sube"},
        {ts::SUPERSET_OF_OR_EQUAL_TO, "supe"},
        {ts::CIRCLED_PLUS, "oplus"},
        {ts::CIRCLED_TIMES, "otimes"},
        {ts::UP_TACK, "perp"},
        {ts::DOT_OPERATOR, "sdot"},
        {ts::LEFT_CEILING, "lceil"},
        {ts::RIGHT_CEILING, "rceil"},
        {ts::LEFT_FLOOR, "lfloor"},
        {ts::RIGHT_FLOOR, "rfloor"},
        {ts::LEFT_POINTING_ANGLE_BRACKET, "lang"},
        {ts::RIGHT_POINTING_ANGLE_BRACKET, "rang"},
        {ts::LOZENGE, "loz"},
        {ts::BLACK_SPADE_SUIT, "spades"},
        {ts::BLACK_CLUB_SUIT, "clubs"},
        {ts::BLACK_HEART_SUIT, "hearts"},
        {ts::BLACK_DIAMOND_SUIT, "diams"},
    }) {}
}


//----------------------------------------------------------------------------
// Character conversions.
//----------------------------------------------------------------------------

bool ts::IsLower(UChar c)
{
    if (std::iswlower(wint_t(c)) != 0) {
        // The standard function says it is lower.
        return true;
    }
    else {
        // Check if it is a known lowercase for us.
        const LowerUpper* lu = LowerUpper::Instance();
        return lu->find(c) != lu->end();
    }
}

bool ts::IsUpper(UChar c)
{
    if (std::iswupper(wint_t(c)) != 0) {
        // The standard function says it is upper.
        return true;
    }
    else {
        // Check if it is a known uppercase for us.
        const UpperLower* ul = UpperLower::Instance();
        return ul->find(c) != ul->end();
    }
}

ts::UChar ts::ToLower(UChar c)
{
    const UChar result = UChar(std::towlower(wint_t(c)));
    if (result != c) {
        // The standard function has found a translation.
        return result;
    }
    else {
        // Search for an additional translation, if any.
        const UpperLower* ul = UpperLower::Instance();
        const UpperLower::const_iterator it(ul->find(c));
        return it == ul->end() ? c : it->second;
    }
}

ts::UChar ts::ToUpper(UChar c)
{
    const UChar result = UChar(std::towupper(wint_t(c)));
    if (result != c) {
        // The standard function has found a translation.
        return result;
    }
    else {
        // Search for an additional translation, if any.
        const LowerUpper* lu = LowerUpper::Instance();
        const LowerUpper::const_iterator it(lu->find(c));
        return it == lu->end() ? c : it->second;
    }
}

bool ts::IsAccented(UChar c)
{
    const WithoutAccent* wa = WithoutAccent::Instance();
    const WithoutAccent::const_iterator it(wa->find(c));
    return it != wa->end();
}

ts::UString ts::RemoveAccent(UChar c)
{
    const WithoutAccent* wa = WithoutAccent::Instance();
    const WithoutAccent::const_iterator it(wa->find(c));
    return it == wa->end() ? ts::UString(1, c) : ts::UString::FromUTF8(it->second);
}

ts::UString ts::ToHTML(UChar c)
{
    const HTMLEntities* he = HTMLEntities::Instance();
    const HTMLEntities::const_iterator it(he->find(c));
    return it == he->end() ? ts::UString(1, c) : (ts::UChar('&') + ts::UString::FromUTF8(it->second) + ts::UChar(';'));
}

void ts::UString::convertToHTML()
{
    // Should not be there, but this is much faster to do it that way.
    const HTMLEntities* he = HTMLEntities::Instance();
    for (size_type i = 0; i < length(); ) {
        const HTMLEntities::const_iterator it(he->find(at(i)));
        if (it == he->end()) {
            ++i;
        }
        else {
            const UString rep(it->second);
            at(i) = ts::AMPERSAND;
            insert(i + 1, rep);
            insert(i + 1 + rep.length(), 1, ts::SEMICOLON);
            i += rep.length() + 2;
        }
    }
}
