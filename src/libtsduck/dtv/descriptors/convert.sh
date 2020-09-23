#!/usr/bin/env bash
#
# Temporary conversion script for source code of descriptor classes.
#
# Old descriptor classes directly override serialize() and deserialize(),
# working on raw data. They declare:
#
# public:
#     DeclareLegacyDisplayDescriptor();  <== note the "Legacy"
#     virtual void serialize(DuckContext&, Descriptor&) const override;
#     virtual void deserialize(DuckContext&, const Descriptor&) override;
#
# New and converted classes override serializePayload() and deserializePayload(),
# working on PSIBuffer. They declare:
#
# public:
#     DeclareDisplayDescriptor();
# protected:
#     virtual void serializePayload(PSIBuffer&) const override;
#     virtual void deserializePayload(PSIBuffer&) override;
#
# In the process of conversion from old to new descriptor class, there are two phases:
# - A boring automated phase to modify the declarations both in the .h and .cpp files.
#   This is done by this script.
# - A manual phase to modify the implementation of the three methods in the .cpp file:
#   serializePayload(), deserializePayload() and DisplayDescriptor().
#
# Options: By default, the serialization/deserialization and the display methods
# are converted. The first argument may be "-s" or "-d", in which case only the
# serialization/deserialization (-s) or the display (-d) methods are converted.
# Option "-t" tests the conversion after completion of the code update.
#

SCRIPT=$(basename $0 .sh)
info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Move to same directory as script, where all source files for descriptor classes are.
cd $(dirname $0)

# Used GNU versions on non-Linux OS.
SED=$(which gsed 2>/dev/null)
SED=${SED:-sed}

# Analyze first parameter for partial conversion.
DISPLAY=false
SERIAL=false
TEST=false
case "$1" in
    -s) shift
        SERIAL=true
        ;;
    -d) shift
        DISPLAY=true
        ;;
    -t) shift
        TEST=true
        ;;
    -*) error "invalid parameter $1"
        ;;
    *)  SERIAL=true
        DISPLAY=true
        ;;
esac

# Loop on class names or file names.
for name in $*; do

    # Extract class name if a file name is given.
    name=${name/#ts/}
    name=${name/%./}
    name=${name/%.h/}
    name=${name/%.cpp/}
    header=ts${name}.h
    source=ts${name}.cpp

    echo "---- Converting class $name"

    # Check that source files exist.
    if [[ ! -f $header ]]; then
        info "$header not found"
        continue
    fi
    if [[ ! -f $source ]]; then
        info "$source not found"
        continue
    fi

    # PSIBuffer is always needed in the source.
    if ! grep -q '#include "tsPSIBuffer.h"' $source; then
        $SED -e '/#include "tsPSIRepository.h"/a\#include "tsPSIBuffer.h"' -i $source
    fi

    # Adjust serialization/deserialization methods
    if $SERIAL; then
        # Adjust header file.
        $SED -e '/virtual void serialize(DuckContext/d' \
             -e '/virtual void deserialize(DuckContext/d' \
             -i $header
        if ! grep -q 'virtual void serializePayload(PSIBuffer' $header; then
            $SED -e '/virtual void clearContent()/a\        virtual void serializePayload(PSIBuffer\&) const override;' -i $header
        fi
        if ! grep -q 'virtual void deserializePayload(PSIBuffer' $header; then
            $SED -e '/virtual void serializePayload(PSIBuffer/a\        virtual void deserializePayload(PSIBuffer\&) override;' -i $header
        fi
        # Adjust source file.
        $SED -e 's/::serialize(DuckContext[^,)]*, Descriptor[^,)]*) const/::serializePayload(PSIBuffer\& buf) const/' \
             -e 's/bbp->append/buf.put/g' \
             -e '/ByteBlockPtr bbp(serializeStart());/d' \
             -e '/serializeEnd(desc, bbp);/d' \
             -e 's/::deserialize(DuckContext[^,)]*, const Descriptor[^,)]*)/::deserializePayload(PSIBuffer\& buf)/' \
             -i $source
    fi

    # Adjust display methods
    if $DISPLAY; then
        # Adjust header file.
        $SED -e 's/DeclareLegacyDisplayDescriptor();/DeclareDisplayDescriptor();/' -i $header
        # Adjust source file.
        $SED -e 's/::DisplayDescriptor(TablesDisplay.*)/::DisplayDescriptor(TablesDisplay\& disp, PSIBuffer\& buf, const UString\& margin, DID did, TID tid, PDS pds)/' \
             -e '/disp.displayExtraData(data, size, margin);/d' \
             -i $source
    fi

    # Test using test-suite
    if $TEST; then
        root=$(cd ../../../..; pwd)
        tsuite=$(cd $root/../tsduck-test; pwd)
        make -C $root -j10 || exit
        xmlname=$(grep '#define MY_XML_NAME' $source | head -1 | sed -e 's/^[^"]*"//' -e 's/" *$//')
        if [[ -n "$xmlname" ]]; then
            for xmlfile in $(grep -l "<$xmlname" $tsuite/input/*.xml); do
                $tsuite/tests/$(basename $xmlfile .xml).sh --dev
            done
        fi
    fi
done
