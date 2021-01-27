#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2021, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  TSDuck Python bindings to DuckContext.
#
#-----------------------------------------------------------------------------

from . import lib
from .native import NativeObject
import ctypes

##
# A wrapper class for C++ DuckContext.
# @ingroup python
#
class DuckContext(NativeObject):

    # Bit masks for standards, used to qualify the signalization, same values as C++ counterparts.
    ## No known standard
    NONE  = 0x00;
    ## Defined by MPEG, common to all standards
    MPEG  = 0x01;
    ## Defined by ETSI/DVB.
    DVB   = 0x02;
    ## Defined by ANSI/SCTE.
    SCTE  = 0x04;
    ## Defined by ATSC.
    ATSC  = 0x08;
    ## Defined by ISDB.
    ISDB  = 0x10;
    ## Defined in Japan only (typically in addition to ISDB).
    JAPAN = 0x20;
    ## Defined by ABNT (Brazil, typically in addition to ISDB).
    ABNT  = 0x40;

    ##
    # Constructor.
    # @param report The ts.Report object to use.
    #
    def __init__(self, report):
        super().__init__()
        self._report = report
        self._native_object = lib.tspyNewDuckContext(self._report._native_object)


    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        lib.tspyDeleteDuckContext(self._native_object)
        super().delete()

    ##
    # Set the default character set for strings.
    # The default should be the DVB superset of ISO/IEC 6937 as defined in ETSI EN 300 468.
    # Use another default in the context of an operator using an incorrect signalization,
    # assuming another default character set (usually from its own country) or in the
    # context of mixed standards (ISBD/DVB for instance).
    # @param charset The new default character set name or an empty string to revert to the default.
    # @return True on success, False if @a charset is invalid.
    #
    def setDefaultCharset(self, charset):
        buf = lib.InByteBuffer(charset)
        return lib.tspyDuckContextSetDefaultCharset(self._native_object, buf.data_ptr(), buf.size())

    ##
    # Set the default CAS id to use.
    # @param cas Default CAS id to be used when the CAS is unknown.
    # @return None.
    #
    def setDefaultCASId(self, cas):
        lib.tspyDuckContextSetDefaultCASId(self._native_object, cas)

    ##
    # Set the default private data specifier to use in the absence of explicit private_data_specifier_descriptor.
    # @param pds Default PDS. Use zero to revert to no default.
    # @return None.
    #
    def setDefaultPDS(self, pds):
        lib.tspyDuckContextSetDefaultPDS(self._native_object, pds)

    ##
    # Add a list of standards which are present in the transport stream or context.
    # @param mask A bit mask of standards.
    # @return None.
    #
    def addStandards(self, mask):
        lib.tspyDuckContextAddStandards(self._native_object, mask)

    ##
    # Get the list of standards which are present in the transport stream or context.
    # @return A bit mask of standards.
    #
    def standards(self):
        return int(lib.tspyDuckContextStandards(self._native_object))

    ##
    # Reset the list of standards which are present in the transport stream or context.
    # @param mask A bit mask of standards.
    # @return None.
    #
    def resetStandards(self, mask = NONE):
        lib.tspyDuckContextResetStandards(self._native_object, mask)

    ##
    # Set a non-standard time reference offset.
    # In DVB SI, reference times are UTC. These SI can be reused in non-standard ways
    # where the stored times use another reference. This is the case with ARIB and ABNT
    # variants of ISDB which reuse TOT, TDT and EIT but with another local time reference.
    # @param offset Offset from UTC in milli-seconds. Can be positive or negative.
    # The default offset is zero, meaning plain UTC time.
    # @return None.
    #
    def setTimeReferenceOffset(self, offset):
        lib.tspyDuckContextSetTimeReferenceOffset(self._native_object, offset)

    ##
    # Set a non-standard time reference offset using a name.
    # @param name Time reference name. The non-standard time reference offset is computed
    # from this name which can be "JST" or "UTC[[+|-]hh[:mm]]".
    # @return True on success, False if @a name is invalid.
    #
    def setTimeReference(self, name):
        buf = lib.InByteBuffer(name)
        return lib.tspyDuckContextSetTimeReference(self._native_object, buf.data_ptr(), buf.size())
