//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPIDOperator.h"
#include "tsCASFamily.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::PIDOperator::PIDOperator(PID pid_, bool is_emm_, uint16_t cas_id_, uint32_t oper_) :
    pid(pid_),
    is_emm(is_emm_),
    cas_id(cas_id_),
    oper(oper_)
{
}


//----------------------------------------------------------------------------
// Comparison operator.
//----------------------------------------------------------------------------

bool ts::PIDOperator::operator<(const PIDOperator& po) const
{
    if (cas_id != po.cas_id) {
        return cas_id < po.cas_id;
    }
    else if (oper != po.oper) {
        return oper < po.oper;
    }
    else if (pid != po.pid) {
        return pid < po.pid;
    }
    else {
        return int(is_emm) < int(po.is_emm);
    }
}


//----------------------------------------------------------------------------
// Search first ECM/EMM PID for a specific operator, return O if not found
//----------------------------------------------------------------------------

ts::PID ts::PIDOperatorSet::pidForOper(uint32_t oper) const
{
    for (const_iterator it = begin(); it != end(); ++it) {
        if (it->oper == oper) {
            return it->pid;
        }
    }
    return 0;
}


//----------------------------------------------------------------------------
// Add all known operator info from a list of descriptors from a CAT or PMT.
//----------------------------------------------------------------------------

void ts::PIDOperatorSet::addAllOperators(const DescriptorList& dlist, bool is_cat)
{
    if (is_cat) {
        addMediaGuardCAT(dlist);
        addSafeAccessCAT(dlist);
    }
    else {
        addMediaGuardPMT(dlist);
    }
    addViaccess(dlist, is_cat);
}


//----------------------------------------------------------------------------
// Add MediaGuard info from a list of descriptors from a PMT
//----------------------------------------------------------------------------

void ts::PIDOperatorSet::addMediaGuardPMT(const DescriptorList& dlist)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // Ignore descriptor if too short
        if (size < 2) {
            continue;
        }

        // Get CA system id
        const uint16_t sysid = GetUInt16(desc);
        desc += 2; size -= 2;

        // Ignore descriptor if not MediaGuard
        if (CASFamilyOf(sysid) != CAS_MEDIAGUARD) {
            continue;
        }

        // Analyze all ECM streams in the descriptor
        while (size >= 15) {
            // Get PID and OPI
            insert(PIDOperator(GetUInt16(desc) & 0x1FFF, false, sysid, GetUInt16(desc + 2)));
            desc += 15; size -= 15;
        }
    }
}


//----------------------------------------------------------------------------
// Add MediaGuard info from a list of descriptors from a CAT
//----------------------------------------------------------------------------

void ts::PIDOperatorSet::addMediaGuardCAT(const DescriptorList& dlist)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // Ignore descriptor if too short
        if (size < 4) {
            continue;
        }

        // Get CA system id and first EMM PID
        const uint16_t sysid = GetUInt16(desc);
        PID pid = GetUInt16(desc + 2) & 0x1FFF;
        uint16_t oper;
        desc += 4; size -= 4;

        // Ignore descriptor if not MediaGuard
        if (CASFamilyOf(sysid) != CAS_MEDIAGUARD) {
            continue;
        }

        // Analyze all EMM streams in the descriptor
        if (size == 4) {
            // New format: only one EMM PID per descriptor (DVB-compliant)
            oper = GetUInt16(desc + 2);
            insert(PIDOperator(pid, true, sysid, oper));
        }
        else if (size >= 1) {
            // Old format: several EMM PID per descriptor (not DVB-compliant)
            uint8_t nb_opi = *desc;
            desc++; size --;
            // First EMM PID is for individual EMMs: no OPI (set OPI to 0xFFFF)
            insert(PIDOperator(pid, true, sysid, 0xFFFF));
            // Other EMM PID's carry group EMMs for one OPI
            while (nb_opi > 0 && size >= 4) {
                pid = GetUInt16(desc) & 0x1FFF;
                oper = GetUInt16(desc + 2);
                insert(PIDOperator(pid, true, sysid, oper));
                desc += 4; size -= 4; nb_opi--;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Add SafeAccess info from a list of descriptors from a CAT
//----------------------------------------------------------------------------

void ts::PIDOperatorSet::addSafeAccessCAT(const DescriptorList& dlist)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // Ignore descriptor if too short
        if (size < 5) {
            continue;
        }

        // Get CA system id and EMM PID
        uint16_t sysid = GetUInt16(desc);
        PID pid = GetUInt16(desc + 2) & 0x1FFF;
        desc += 5; size -= 5;

        // Ignore descriptor if not SafeAccess
        if (CASFamilyOf(sysid) != CAS_SAFEACCESS) {
            continue;
        }

        // Analyze all PPID in the descriptor
        while (size >= 2) {
            uint16_t oper = GetUInt16(desc);
            insert(PIDOperator(pid, true, sysid, oper));
            desc += 2; size -= 2;
        }
    }
}


//----------------------------------------------------------------------------
// Add Viaccess info from a list of descriptors from a CAT or PMT.
//----------------------------------------------------------------------------

void ts::PIDOperatorSet::addViaccess(const DescriptorList& dlist, bool is_cat)
{
    // Loop on all CA descriptors
    for (size_t index = dlist.search(DID_CA); index < dlist.count(); index = dlist.search(DID_CA, index + 1)) {

        // Descriptor payload
        const uint8_t* desc = dlist[index]->payload();
        size_t size = dlist[index]->payloadSize();

        // Ignore descriptor if too short
        if (size < 4) {
            continue;
        }

        // Get CA system id and EMM PID
        const uint16_t sysid = GetUInt16(desc);
        const PID pid = GetUInt16(desc + 2) & 0x1FFF;
        desc += 4; size -= 4;

        // Ignore descriptor if not Viaccess.
        if (CASFamilyOf(sysid) != CAS_VIACCESS) {
            continue;
        }

        // Analyze all TLV in the descriptor, collecting SOID parameters.
        while (size >= 2) {
            const uint8_t tag = desc[0];
            size_t len = desc[1];
            desc += 2; size -= 2;
            if (len > size) {
                len = size;
            }
            if (tag == 0x14 && len == 3) {
                const uint32_t oper = GetUInt24(desc);
                insert(PIDOperator(pid, is_cat, sysid, oper));
            }
            desc += len; size -= len;
        }
    }
}
