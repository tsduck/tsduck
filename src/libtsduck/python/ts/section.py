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
#  TSDuck Python bindings to SectionFile.
#
#-----------------------------------------------------------------------------

from . import lib
from .native import NativeObject
import ctypes

##
# A wrapper class for C++ SectionFile.
# @ingroup python
#
class SectionFile(NativeObject):

    ##
    # Constructor.
    # @param duck The ts.DuckContext object to use.
    #
    def __init__(self, duck):
        super().__init__()
        self._duck = duck
        self._native_object = lib.tspyNewSectionFile(self._duck._native_object)

    # Explicitly free the underlying C++ object (inherited).
    def delete(self):
        lib.tspyDeleteSectionFile(self._native_object)
        super().delete()

    ##
    # Clear the content of the SectionFile, erase all sections.
    # @return None.
    #
    def clear(self):
        lib.tspySectionFileClear(self._native_object)

    ##
    # Get the size in bytes of all sections.
    # This would be the size of the corresponding binary file.
    # @return The size in bytes of all sections.
    #
    def binarySize(self):
        return int(lib.tspySectionFileBinarySize(self._native_object))

    ##
    # Get the total number of sections in the file.
    # @return The total number of sections in the file.
    #
    def sectionsCount(self):
        return int(lib.tspySectionFileSectionsCount(self._native_object))

    ##
    # Get the total number of full tables in the file.
    # Orphan sections are not included.
    # @return The total number of full tables in the file.
    #
    def tablesCount(self):
        return int(lib.tspySectionFileTablesCount(self._native_object))

    ##
    # Load a binary section file.
    # The loaded sections are added to the content of this object.
    # @param file Binary file name.
    # If the file name is empty or "-", the standard input is used.
    # @return True on success, False on error.
    #
    def loadBinary(self, file):
        name = lib.InByteBuffer(file)
        return bool(lib.tspySectionFileLoadBinary(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save a binary section file.
    # @param [in] file Binary file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveBinary(self, file):
        name = lib.InByteBuffer(file)
        return bool(lib.tspySectionFileSaveBinary(self._native_object, name.data_ptr(), name.size()))

    ##
    # Load an XML file.
    # The loaded tables are added to the content of this object.
    # @param file XML file name.
    # If the file name starts with "<?xml", this is considered as "inline XML content".
    # If the file name is empty or "-", the standard input is used.
    # @return True on success, False on error.
    #
    def loadXML(self, file):
        name = lib.InByteBuffer(file)
        return bool(lib.tspySectionFileLoadXML(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save an XML file.
    # @param [in] file XML file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveXML(self, file):
        name = lib.InByteBuffer(file)
        return bool(lib.tspySectionFileSaveXML(self._native_object, name.data_ptr(), name.size()))

    ##
    # Save a JSON file after automated XML-to-JSON conversion.
    # @param [in] file JSON file name.
    # If the file name is empty or "-", the standard output is used.
    # @return True on success, False on error.
    #
    def saveJSON(self, file):
        name = lib.InByteBuffer(file)
        return bool(lib.tspySectionFileSaveJSON(self._native_object, name.data_ptr(), name.size()))

    ##
    # Serialize as XML text.
    # @return Complete XML document text, empty on error.
    #
    def toXML(self):
        buf = lib.OutByteBuffer(2048)
        len = lib.tspySectionFileToXML(self._native_object, buf.data_ptr(), buf.size_ptr())
        if len > 2048:
            # First try was too short
            buf = lib.OutByteBuffer(len)
            len = lib.tspySectionFileToXML(self._native_object, buf.data_ptr(), buf.size_ptr())
        return buf.to_string()

    ##
    # Serialize as JSON text.
    # @return Complete JSON document text, empty on error.
    #
    def toJSON(self):
        buf = lib.OutByteBuffer(2048)
        len = lib.tspySectionFileToJSON(self._native_object, buf.data_ptr(), buf.size_ptr())
        if len > 2048:
            # First try was too short
            buf = lib.OutByteBuffer(len)
            len = lib.tspySectionFileToJSON(self._native_object, buf.data_ptr(), buf.size_ptr())
        return buf.to_string()
